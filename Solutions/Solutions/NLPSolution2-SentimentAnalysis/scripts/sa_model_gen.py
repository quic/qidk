import tensorflow as tf

from transformers import TensorType
from transformers import AutoTokenizer, TFAutoModelForSequenceClassification
import sys

bs = 1
SEQ_LEN = 128
MODEL_NAME = "Alireza1044/mobilebert_sst2"

# Allocate tokenizer and model
tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME)
model = TFAutoModelForSequenceClassification.from_pretrained(MODEL_NAME, from_pt=True)
# model = TFAutoModelForQuestionAnswering.from_pretrained(MODEL_NAME)

def model_fn(input_ids, attention_mask):
    output = tf.nn.softmax(model(input_ids, attention_mask).logits, axis=-1)
    return output

model_fn = tf.function(
    model_fn,
    input_signature=[
        tf.TensorSpec(shape=[bs, SEQ_LEN], dtype=tf.int32),
        tf.TensorSpec(shape=[bs, SEQ_LEN], dtype=tf.int32)
    ]
)

# Sample input
context = "It is easy to say but hard to do ..."

input_encodings = tokenizer(
            context,
            return_tensors=TensorType.TENSORFLOW,
            # return_tensors="np",
            padding='max_length',
            return_length=True,
            max_length=SEQ_LEN,
            return_special_tokens_mask=True
        )
# print(input_encodings)

print(f"\nContext = \n{context}")
logits = model_fn(input_encodings.input_ids, input_encodings.attention_mask)
# print(logits)
# print(logits.shape)

positivity = logits[0][1] * 100
negativity = logits[0][0] * 100

print(f"\nPrediction: {positivity:.2f}% positive & {negativity:.2f}% negative\n")

input("Enter to continue ...")
from tensorflow.python.framework.convert_to_constants import convert_variables_to_constants_v2
frozen_func = convert_variables_to_constants_v2(model_fn.get_concrete_function())

layers = [op.name for op in frozen_func.graph.get_operations()]
print("-" * 50)
print("NO. of Frozen model layers: {}".format(len(layers)))

print("-" * 50)
print("Frozen model inputs: ")
print(frozen_func.inputs)
print("Frozen model outputs: ")
print(frozen_func.outputs)

graph_def = frozen_func.graph.as_graph_def()

graph_def = tf.compat.v1.graph_util.remove_training_nodes(graph_def)

tf.io.write_graph(graph_or_graph_def=graph_def,
                  logdir="./frozen_models",
                  name="mobilebert_sst2.pb",
                  as_text=False)

