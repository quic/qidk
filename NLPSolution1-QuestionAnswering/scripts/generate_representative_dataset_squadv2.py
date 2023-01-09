from functools import partial
import numpy as np
import tensorflow as tf
from typing import Tuple
import os
import json

from transformers import TensorType
from transformers.modeling_tf_outputs import TFQuestionAnsweringModelOutput
from transformers import AutoTokenizer, TFAutoModelForQuestionAnswering

from tensorflow.python.ops.numpy_ops import np_config
np_config.enable_numpy_behavior()

os.system('mkdir -p input_ids')
os.system('mkdir -p attention_mask')
os.system('mkdir -p token_type_ids')
os.system('mkdir -p golden_model_logits')
os.system('mkdir -p golden_answers')
os.system('mkdir -p golden_questions_n_context')

os.environ['CUDA_VISIBLE_DEVICES'] = '-1'

# @tf.function()
def qa_pipeline(
    model,
    input_ids,
    attention_mask,
    token_type_ids,
    special_tokens_mask,
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
    model_outputs = model(input_ids, attention_mask, token_type_ids)
    assert isinstance(model_outputs, TFQuestionAnsweringModelOutput)

    # print(model_outputs.start_logits)

    # dump logits_from_golden_model
    start_logits = np.array(model_outputs.start_logits)
    end_logits = np.array(model_outputs.end_logits)

    with open("golden_model_logits/"+ idx_suffix +"start_logits.raw", 'w') as f:
        start_logits.astype(np.float32).tofile(f)
    with open("golden_model_logits/"+ idx_suffix +"end_logits.raw", 'w') as f:
        end_logits.astype(np.float32).tofile(f)

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
    start_scores = tf.where(desired_tokens == 0, -10000.0, model_outputs.start_logits)
    end_scores = tf.where(desired_tokens == 0, -10000.0, model_outputs.end_logits)

    # Normalize logits and spans to retrieve the answer
    start_scores = tf.nn.softmax(start_scores, axis=-1, name="start_scores")
    end_scores = tf.nn.softmax(end_scores, axis=-1, name="end_scores")

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

from datasets import load_dataset, load_metric
from tqdm import tqdm
import sys
if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("Usage : python generate_representative_dataset_squadv2.py <model_name> <num_samples> <seq_len>")
        sys.exit(0)
    nb_samples = int(sys.argv[2])
    SEQ_LEN = int(sys.argv[3])
    MODEL_NAME = sys.argv[1]
    print("MODEL = {}".format(MODEL_NAME))
    print(f"Num of samples = {nb_samples}\n")
    print(f"SEQ Len = {SEQ_LEN}\n")

    # Allocate tokenizer and model
    tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME)
    model = TFAutoModelForQuestionAnswering.from_pretrained(MODEL_NAME, from_pt=True)

    # Sample input
    # context = "I'm on highway to hell!"
    # question = "Where am I going to?"

    # load squad-2 samples
    dataset = load_dataset("squad_v2", split="validation")
    dataset = dataset.shuffle(seed=128)

    metric = load_metric("squad_v2")
    references = []
    predictions = []

    for i in tqdm(range(nb_samples)):
        idx_suffix = "_{0:03}_".format(i)  # for suffixing RAW files name
        items = dataset[i]

        # Ask the tokenizer for the special_tokens_mask
        input_encodings = tokenizer(
            items["question"],
            items["context"],
            # return_tensors=TensorType.TENSORFLOW,
            return_tensors="np",
            padding='max_length',
            truncation="longest_first",
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

        # dump context and questions
        qid = {
            "question" : items["question"],
            "context" : items["context"]
        }
        with open("golden_questions_n_context/qid_"+ idx_suffix +".json", 'w') as f:
            f.write(json.dumps(qid))
        
        # dump model input ids
        with open("input_ids/input_ids_"+ idx_suffix +".raw", 'w') as f:
            input_encodings.input_ids.astype(np.float32).tofile(f)

        # dump model attention_mask
        with open("attention_mask/attention_mask_"+ idx_suffix +".raw", 'w') as f:
            input_encodings["attention_mask"].astype(np.float32).tofile(f)

        # dump model token_type_ids
        with open("token_type_ids/token_type_ids_"+ idx_suffix +".raw", 'w') as f:
            input_encodings["token_type_ids"].astype(np.float32).tofile(f)
        
        # dump golden_answers 
        with open("golden_answers/"+ idx_suffix +"processed_answer.json", 'w') as f:
            f.write(json.dumps({"id": items["id"], "answers": items["answers"]}))

        scores, answers_start_end_indexes, answers_probabilities = qa_pipeline(
            model,
            input_encodings.input_ids.astype(np.int32),
            input_encodings.attention_mask.astype(np.int32),
            input_encodings.token_type_ids.astype(np.int32),
            input_encodings.special_tokens_mask.astype(np.int32),
            top_k=1  # Example with the 1 most probable answers
        )

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

    print("\nMetrics report")
    print("===============================================================")
    result = metric.compute(references=references, predictions=predictions)
    for key, value in result.items():
        print(f"{key} = {value}")
    print("===============================================================")
    print()
    # Save model
    # tf.saved_model.save(model, "saved_models/qa_bert")
