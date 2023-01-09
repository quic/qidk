from functools import partial

import tensorflow as tf
from typing import Tuple
import numpy as np
from argparse import ArgumentParser

from transformers import TFBertForQuestionAnswering, TensorType, BertTokenizerFast
from transformers.modeling_tf_outputs import TFQuestionAnsweringModelOutput
from transformers import AutoTokenizer, TFAutoModelForQuestionAnswering

# @tf.function()
def qa_pipeline(
    start_logit,
    end_logit,
    input_ids: tf.Tensor,
    attention_mask: tf.Tensor,
    token_type_ids: tf.Tensor,
    special_tokens_mask: tf.Tensor,
    top_k: int = 1,
    max_answer_length: int = 20
) -> Tuple[tf.Tensor, tf.Tensor, tf.Tensor]:
    """

    :param input_ids: Token ids representing the context and question concatenated as (context + pad, question)
    :param attention_mask: Attention mask avoiding attending to padded tokens
    :param token_type_ids: Context (0) and question (1) spans indicators
    :param top_k: int (default = 1) Number of - most probable - candidates answer(s) to return
    :param max_answer_length: int (default = 20) maximum length of the answer(s)
    :return:
     0: tf.Tensor, Score for each (start, end) pair
     1: tf.Tensor, Answer token positions (sorted according to probability)  (batch, k, (batch idx, start idx, end_idx))
     2: tf.Tensor, Probability of each answer (batch, k, 1)
    """
    # model_outputs = model(input_ids, attention_mask, token_type_ids)
    # start_logit = model_outputs.start_logits
    # end_logit = model_outputs.end_logits
    # assert isinstance(model_outputs, TFQuestionAnsweringModelOutput)

    special_tokens_mask = tf.tensor_scatter_nd_update(
        special_tokens_mask,
        tf.repeat(tf.zeros((1, 2), dtype=tf.int32), tf.shape(special_tokens_mask)[0], axis=0),
        tf.zeros((1, ), dtype=tf.int32)
    )

    token_type_ids = tf.tensor_scatter_nd_update(
        token_type_ids,
        tf.repeat(tf.zeros((1, 2), dtype=tf.int32), tf.shape(special_tokens_mask)[0], axis=0),
        tf.ones((1, ), dtype=tf.int32)
    )

    # desired_tokens = ([1, 1, 1, 1, 1, 1, 0, 0, 0] AND [0, 0, 0, 1, 1, 1, 1, 1, 1]) -> Question tokens AND Padded tokens are masked out
    # desired_tokens = [0, 0, 0, 1, 1, 1, 0, 0, 0]  -> This now represents the mask where we want to grab the answer tokens (i.e. in the context tokens)
    desired_tokens = ~special_tokens_mask & token_type_ids

    # Make sure non-context indexes in the tensor cannot contribute to the softmax
    # (-10000.0, might not work for int8 -> need something small enough to have the least contribution on the softmax)
    start_scores = tf.where(desired_tokens == 0, -10000.0, start_logit)
    end_scores = tf.where(desired_tokens == 0, -10000.0, end_logit)

    # Normalize logits and spans to retrieve the answer
    start_scores = tf.nn.softmax(start_scores, axis=-1, name="start_scores")
    end_scores = tf.nn.softmax(end_scores, axis=-1, name="end_scores")

    # All the masking below as been replaced by the use of $special_tokens_mask

    # Mask CLS token (i.e. the first element of each batch tensor).
    # To do this, dropping the first element of the tensor and then left-padding it with 0.0
    # Having 0.0 give 0 probability of CLS token to be sampled for candidate answer(s)
    # start_scores = tf.pad(start_scores[:, 1:], tf.constant([[0, 0], [1, 0]]))
    # end_scores = tf.pad(end_scores[:, 1:], tf.constant([[0, 0], [1, 0]]))

    # Dot product start x end (i.e. importance of each tuple (start, end))
    # start_end_scores: [B, S, S] <= bmm([B, S, 1], [B, S, 1].T)
    start_end_scores = tf.matmul(
        tf.expand_dims(start_scores, -1),
        tf.expand_dims(end_scores, -1),
        transpose_b=True
    )

    # Filter start after end, end before start and answer too long
    candidates_shape = tf.shape(start_end_scores)
    num_upper_candidates = candidates_shape[2] - (max_answer_length - 1)

    # band_part(...) == triu(tril(x), offset=max_len -1) https://www.tensorflow.org/api_docs/python/tf/linalg/band_part
    candidates_scores = tf.linalg.band_part(start_end_scores, 0, num_upper_candidates, "candidate_max_seq_len_filtering")

    # Sort candidates and retrieve most probable one
    # TODO: May be it's possible to not flatten the candidates_scores tensor, thus avoiding costly reshape(s)
    # TODO: top_k gives a quite differ output in the case of 2D tensor, need to investigate if feasible.
    candidates_scores_flat = tf.reshape(candidates_scores, (-1, tf.math.reduce_prod(candidates_shape[1:])))
    best_candidates = tf.math.top_k(candidates_scores_flat, top_k, name="top_k_candidates")

    # Same as map_fn, but vectorizes over batch axis, running func() in //
    # See https://www.tensorflow.org/api_docs/python/tf/vectorized_map
    best_candidates_index = tf.vectorized_map(
        partial(tf.unravel_index, dims=candidates_shape),
        best_candidates.indices
    )

    # Transpose to get (B, K, 3) -> (3 = Batch idx, Start idx, End idx)
    return candidates_scores, tf.transpose(best_candidates_index, (0, 2, 1)), best_candidates.values

def get_args():
    parser = ArgumentParser(
        description="Post-processing Tool to parse output logits and map to SQuAD v2 context answer"
    )
    parser.add_argument(
        "--model_name",
        type=str, default="deepset/bert-base-cased-squad2",
        help="The model to use for generating ouputs, default is deepset/bert-base-cased-squad2",
    )
    parser.add_argument(
        "--sequence_len", type=int, default=384, help="Input sequence length of the model , default=384"
    )
    parser.add_argument(
        "--logits_dir", type=str, default="./tf_out", help="Output Start-End Logits location folder/dir"
    )
    parser.add_argument("--golden_answers_dir", type=str, help="golden_answers location folder/dir")
    parser.add_argument("--top_k", type=str, help="top k predictions")

    return parser.parse_args()                        

from datasets import load_dataset, load_metric
import os
import sys
import json

if __name__ == '__main__':
    args = get_args()
    SEQ_LEN = int(args.sequence_len)
    MODEL_NAME = args.model_name
    print("MODEL = {}".format(MODEL_NAME))

    # Allocate tokenizer and model
    tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME, padding_side = 'right', model_max_length=SEQ_LEN)

    # load squad-2 samples
    dataset = load_dataset("squad_v2", split="validation")
    dataset = dataset.shuffle(seed=128)

    # Loading the metric.
    metric = load_metric("squad_v2")
    references = []
    predictions = []

    # squad golden ans
    answers = sorted([f for f in os.listdir(args.golden_answers_dir) if f.endswith('.json')])

    # get logits
    logits_list = []
    for root, dirs, files in os.walk(args.logits_dir):
        for file in files:
            if file.endswith(".raw"):
                logits_list.append(os.path.join(root,file))
    
    print(f"logits length: {len(logits_list)}\n")
    # print(logits_list)
    # sort the logit => 'output/Result_0/Identity_1_0.raw'
    tmp_list = [None]*len(logits_list)
    for idx in range(len(logits_list)):
        for p in logits_list: 
            if f"Result_{idx}/" in p:
                if "_1:0.raw" in p:
                    tmp_list[2*idx+1] = p   # c style indexing
                else:
                    tmp_list[2*idx] = p
            
    if len(logits_list[0].split('/')) > 2:   # that means that should be from QNN
        logits_list = tmp_list
    else:
        logits_list = sorted(logits_list)

    print(logits_list)
    print("\nVerify the sorted logits order.\n\nPress any key to continue ...")
    input()

    idx = 0 # iter count
    # iterate on outputs
    for start, end in zip(logits_list[::2], logits_list[1::2]):
        items = dataset[idx]
        
        # check golden ans ID with items.id
        with open(os.path.join(args.golden_answers_dir, answers[idx]), 'r') as fd:
            answer_json = json.load(fd)
        if answer_json["id"] != items["id"]:
            raise Exception("Squadv2 Dataset ID mismatch")

        # load np raw buffers
        #start_logit = np.fromfile(os.path.join(args.logits_dir, start), np.float32).reshape(1,SEQ_LEN)
        start_logit = np.fromfile(start, np.float32).reshape(1,SEQ_LEN)
        end_logit = np.fromfile(end, np.float32).reshape(1,SEQ_LEN)

        input_encodings = tokenizer(
            items["question"],
            items["context"],
            return_tensors=TensorType.TENSORFLOW,
            truncation="longest_first",
            padding='max_length',
            return_length=True,
            max_length=SEQ_LEN,
            return_special_tokens_mask=True
        )

        # Print context and question
        context = items["context"]
        question = items["question"]
        print("\nContext = \n{}".format(context))
        print(f"\nQ. > {question}")
        print("\nAns = {}".format(items["answers"]["text"]))

        # Provide the special_tokens_mask only for the postprocessing
        scores, answers_start_end_indexes, answers_probabilities = qa_pipeline(
            start_logit,
            end_logit,
            input_encodings.input_ids,
            input_encodings.attention_mask,
            input_encodings.token_type_ids,
            input_encodings.special_tokens_mask,
            top_k=int(args.top_k)  # Example with the 2 most probable answers
        )

        # Adding the reference for metric computation.
        references.append({"id": items["id"], "answers": items["answers"]})

        is_top_1 = True
        # /!\ Assume batch is 1 here, care if greater, might need an extra outer loop. /!\
        # Retrieve the span in the original text
        for (_, top_i_answer_start, top_i_answer_end), top_i_answer_prob in zip(answers_start_end_indexes[0], answers_probabilities[0]):

            # If start pos == end pos == 0, then it indicates "No Answer in the current document"
            # For more information, see BERT Paper - Section 4.3 (https://arxiv.org/pdf/1810.04805.pdf)
            if top_i_answer_start == top_i_answer_end == 0:
                print(f"\nPrediction = Answer is not present in span (p={top_i_answer_prob})")
                if is_top_1:
                    predictions.append({"id": items["id"], "prediction_text": "", "no_answer_probability": 0.0})
            else:
                # Avoid answer starting at index 0 as it would point to a special token
                top_i_answer_start, top_i_answer_end = max(top_i_answer_start, 1), max(top_i_answer_end, 1)
                char_start = input_encodings.token_to_chars(top_i_answer_start).start
                char_end = input_encodings.token_to_chars(top_i_answer_end).end

                print(f"\nPrediction = {context[char_start: char_end]} (p={top_i_answer_prob})")

                if is_top_1:
                    predictions.append({"id": items["id"], "prediction_text": context[char_start: char_end], "no_answer_probability": 0.})
            
            # Turn off flag, as next iter will be Top-i
            is_top_1 = False

            print("===============================================================")
            print()
        idx = idx + 1
    print("\nMetrics report")
    print("===============================================================")
    result = metric.compute(references=references, predictions=predictions)
    for key, value in result.items():
        print(f"{key} = {value}")
    print("===============================================================")
    print()
    # Save model
    # tf.saved_model.save(model, "saved_models/qa_bert")
