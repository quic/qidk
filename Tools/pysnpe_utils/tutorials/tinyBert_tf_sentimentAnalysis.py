#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

import os
os.environ["TOKENIZERS_PARALLELISM"] = "false"

import numpy as np
from icecream import ic
import tensorflow as tf
from transformers import AutoTokenizer, TFAutoModelForSequenceClassification

bs = 1
SEQ_LEN = 128
MODEL_NAME = "philschmid/tiny-bert-sst2-distilled"

# Create tokenizer and TF Keras model instance
tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME)
model = TFAutoModelForSequenceClassification.from_pretrained(MODEL_NAME, from_pt=True)

# Sample input
context = "It is easy to say but hard to do ..."
print(f"\nContext = \n{context}")

# Convert text to input vectors
input_encodings = tokenizer(
            context,
            return_tensors="np",
            padding='max_length',
            return_length=True,
            max_length=SEQ_LEN,
            return_special_tokens_mask=True
        )

# Run Inference
logits = model(input_encodings.input_ids, input_encodings.attention_mask)

# Post process model output
logits = tf.nn.softmax(logits[0], axis=-1)
positivity = logits[0][1] * 100
negativity = logits[0][0] * 100

# Print results
print(f"\nModel Prediction: {positivity:.2f}% positive & {negativity:.2f}% negative\n")

input("Press Enter key to Continue ...")

# =======================================================================
from pysnpe_utils import pysnpe
from pysnpe_utils.pysnpe_enums import *

pysnpe.set_logging_level("INFO")

print("="*70)
print("Convert Model to SNPE DLC:")
print("="*70 + "\n")

# Create target device instance for deploying model
target_device = pysnpe.TargetDevice(target_device_adb_id="1c7dec76")

# Create list of InputMap to specify Model Input layers and dimensions, with datatype
input_map = [ InputMap('input_ids', (1,128), tf.int32), 
              InputMap('attention_mask', (1,128), tf.int32) ]

# Export model in TF Saved-Model/Frozen Graph format and 
  # 1. Convert to FP32 DLC using `to_dlc()`
    # 2. Generate DSP FP16 Cache, by mentioning DLC type, using `gen_dsp_graph_cache()`
      # 3. Visualize the generated DLC, using `visualize_dlc()
snpe_context = pysnpe.export_to_tf_keras_model(model, input_map, "sentiment_analysis_tiny_bert",
                                                 skip_model_optimizations=False, frozen_graph_path="sentiment_analysis_tiny_bert_frozen_graph.pb"
                                               ).to_dlc()\
                                                    .gen_dsp_graph_cache(DlcType.FLOAT)\
                                                        .visualize_dlc()

# If Model or DLC is available, the use following to create SnpeContext

# snpe_context = pysnpe.SnpeContext("sentiment_analysis_tiny_bert.pb",
#                                     ModelFramework.TF, 
#                                     "sentiment_analysis_tiny_bert_dsp_fp16_cached.dlc",
#                                     input_dict, ["Identity:0"])

# Prepare Inputs for Inference in Numpy Array format
input_tensor_map = { "input_ids:0": np.array(input_encodings.input_ids, dtype=np.float32), 
                     "attention_mask:0": np.array(input_encodings.attention_mask, dtype=np.float32) }

# Run Inference on target_device
out_tensor_map = snpe_context.execute_dlc(input_tensor_map, DlcType.FLOAT, 
                                            target_acclerator=Runtime.DSP, target_device=target_device)

ic(out_tensor_map)

# Post process the output
logits = tf.nn.softmax(out_tensor_map["Identity:0"], axis=-1)
positivity = logits[0][1] * 100
negativity = logits[0][0] * 100

# Print results
print(f"\nSNPE DLC Prediction: {positivity:.2f}% positive & {negativity:.2f}% negative\n")