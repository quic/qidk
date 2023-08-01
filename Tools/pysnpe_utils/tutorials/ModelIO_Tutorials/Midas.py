# get the onnx model from  wget https://github.com/isl-org/MiDaS/releases/download/v2_1/model-f6b98070.onnx

from pysnpe_utils import pysnpe
from pysnpe_utils.modelio import MidasIO
import os
import numpy as np
import cv2

def midas():
    do_transform = (1,2,0) # SNPE expects NHWC
    img_path = "input/ronaldo.jpg" # img_path = composer.clickFromCamera(device_id)
    input_dtype = { "0": "folat32"}
    
    midasio_obj = MidasIO.MidasIO()
        
    preprocess_img = midasio_obj.preprocess(img_path)
    input_tensor_map = {"0":preprocess_img}


    target_device = pysnpe.TargetDevice(target_device_adb_id="1c7dec76")
    
    snpe_context = pysnpe.SnpeContext("model-f6b98070.onnx", "ONNX", "model-f6b98070_Q.dlc", {"0":(1,3,384,384)},["1080"],None, target_device, None)
    
    out_tensor_map = snpe_context.execute_dlc(input_tensor_map, "QUANT", {"0":do_transform}, " --use_dsp ", target_device=target_device)

    midasio_obj.postprocess("input/ronaldo.jpg", out_tensor_map )


# call to midas function
midas()