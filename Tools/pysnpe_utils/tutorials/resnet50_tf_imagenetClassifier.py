#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

import numpy as np
from tensorflow.keras.applications import ResNet50
from tensorflow.keras.preprocessing import image
from tensorflow.keras.applications.resnet50 import preprocess_input, decode_predictions
import tensorflow as tf
from icecream import ic
import urllib.request

# Load the pre-trained ResNet50 model
model = ResNet50(weights='imagenet')

# Download and Load sample image for inference
url, filename = ("https://github.com/pytorch/hub/raw/master/images/dog.jpg", "dog.jpg")
urllib.request.urlretrieve(url, filename)
img = image.load_img(filename, target_size=(224, 224))
x = image.img_to_array(img)
x = np.expand_dims(x, axis=0)
x = preprocess_input(x)
ic(x.shape)

# Perform inference
predictions = model.predict(x)
ic(predictions.shape)
class_predictions = decode_predictions(predictions, top=3)[0]
ic(class_predictions)

input("\nPress Enter to continue ...")

# ====================== OnDevice Inference with ===========================
print("="*80 + "\nModel Conversion and Execution via PySnpe\n" + "="*80)
# ========================= PySnpe Interface ===============================

from pysnpe_utils import pysnpe
from pysnpe_utils.pysnpe_enums import *

# Select target device
target_device = pysnpe.TargetDevice(device_host="10.206.64.253", 
                                    target_device_adb_id="728b7a92")

# Create list of InputMap to specify Model Input layers and dimensions, with datatype
input_map = [ InputMap('input_img', (1, 224, 224, 3), tf.float32) ]

snpe_context = pysnpe.export_to_tf_keras_model(model, input_map, "resnet50_classification",
                                                frozen_graph_path="resnet50_classification.pb"
                                               ).to_dlc()\
                                                    .gen_dsp_graph_cache(DlcType.FLOAT)\
                                                        .set_target_device(target_device=target_device)\
                                                            .profile(runtime=Runtime.DSP)

# Prepare Inputs for Inference in Numpy Array format
input_tensor_map = { "input_img:0": np.array(x, dtype=np.float32) }

# Run Inference on target_device
out_tensor_map = snpe_context.execute_dlc(input_tensor_map, DlcType.FLOAT, 
                                            target_acclerator=Runtime.DSP, target_device=target_device)

ic(out_tensor_map['Identity:0'].shape)

snpe_class_predictions = decode_predictions(out_tensor_map['Identity:0'], top=3)[0]
ic(snpe_class_predictions)