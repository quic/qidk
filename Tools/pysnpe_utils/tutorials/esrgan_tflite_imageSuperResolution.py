import sys, io, os
import numpy as np
import requests
import tensorflow as tf
import tensorflow_hub as hub
from PIL import Image as PILImage
from icecream import ic
os.environ["TFHUB_DOWNLOAD_PROGRESS"] = "True"

# Download the model from Tensorflow Hub and set static input shape
IMAGE_WIDTH = 128 ; IMAGE_HEIGHT = 128
keras_layer = hub.KerasLayer('https://tfhub.dev/captain-pool/esrgan-tf2/1')
model = tf.keras.Sequential([keras_layer])
model.build([1, IMAGE_WIDTH, IMAGE_HEIGHT, 3])
model.summary()

# Download, Resize and Preprocess image.
image_url = "https://lh4.googleusercontent.com/-Anmw5df4gj0/AAAAAAAAAAI/AAAAAAAAAAc/6HxU8XFLnQE/photo.jpg64"
image_response = requests.get(image_url, verify=False)
image = PILImage.open(io.BytesIO(image_response.content)).convert('RGB')

min_dim = min(image.size[0], image.size[1])
image = image.resize((IMAGE_WIDTH * image.size[0] // min_dim,
                      IMAGE_HEIGHT * image.size[1] // min_dim), PILImage.BICUBIC)
image.save("img_before_sr.png")                      
input_data = np.expand_dims(image, axis=0)
input_data = input_data[:, :IMAGE_WIDTH, :IMAGE_HEIGHT, :]
input_data = input_data.astype(np.float32)

def post_process(prediction, out_name):
    sr_image = tf.squeeze(tf.clip_by_value(prediction, 0, 255))
    ic(sr_image.shape)
    sr_image = PILImage.fromarray(tf.cast(sr_image, tf.uint8).numpy())
    sr_image.save(out_name)

# Run Inference 
prediction = model(input_data)

post_process(prediction, "img_after_sr_with_tf.png")

# ====================== OnDevice Inference with ===========================
print("="*80 + "\nModel Conversion and Execution via TFLITE path\n" + "="*80)
# ========================= PySnpe Interface ===============================

from pysnpe_utils import pysnpe
from pysnpe_utils.pysnpe_enums import *

target_device = pysnpe.TargetDevice(device_host="10.206.64.253", 
                                    target_device_adb_id="728b7a92")

snpe_context = pysnpe.export_to_tflite(model, "esrgan.tflite")\
                        .to_dlc().gen_dsp_graph_cache(DlcType.FLOAT)\
                            .set_target_device(target_device=target_device)\
                                .profile(runtime=Runtime.DSP)

# Prepare Inputs for Inference in Numpy Array format
input_tensor_map = { "keras_layer_input": input_data } 

# Run Inference on target_device
out_tensor_map = snpe_context.execute_dlc(input_tensor_map, DlcType.FLOAT, 
                                            target_acclerator=Runtime.DSP)

post_process(out_tensor_map["Identity"], "img_after_sr_with_snpe_dsp.png")
