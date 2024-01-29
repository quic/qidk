# -*- mode: python -*-
# =============================================================================
#  @@-COPYRIGHT-START-@@
#
#  Copyright (c) 2023 of Qualcomm Innovation Center, Inc. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause
#
#  @@-COPYRIGHT-END-@@
# =============================================================================

import cv2
import numpy as np
from tqdm import tqdm
import time
from snpehelper_manager import PerfProfile,Runtime,timer,SnpeContext

class SESR(SnpeContext):
    def __init__(self,dlc_path: str = "None",
                    input_layers : list = [],
                    output_layers : list = [],
                    output_tensors : list = [],
                    runtime : str = Runtime.CPU,
                    profile_level : str = PerfProfile.BALANCED,
                    enable_cache : bool = False):
        super().__init__(dlc_path,input_layers,output_layers,output_tensors,runtime,profile_level,enable_cache)
    
    """
    Description:
        Preprocess on input data and SetInputBuffer

        Specify layer name to overwrite input buffer which is further provided to model
        
        Repeat SetInputBuffer if there are multiple input nodes
        
        This will be replacement for saving *.raw file
    """
    #@timer
    def preprocess(self,images):
        preprocess_image = [image.flatten() / 255.0 for image in images]
        self.SetInputBuffer(preprocess_image,"lr")
        return  

    """
    Description:
        Postprocess on inferenced data
        
        Specify output tensor to get model inferenced data for this tensor -> which returns 1D array
        
        Repeat GetOutputBuffer if there are multiple output nodes
        
        This will be replacement for opening result *.raw file
    """
    #@timer
    def postprocess(self):
        OutputBuffer = self.GetOutputBuffer("sr") 
        OutputBuffer = OutputBuffer.reshape(512,512,3)
        OutputBuffer = OutputBuffer * 255.0
        #To save output
        cv2.imwrite(f"super_resolution.jpg",OutputBuffer) 
        return 
        
# Instance for SnpeContext 
model_object = SESR(dlc_path="SESR_quantized.dlc",input_layers=["lr"], output_layers=["DepthToSpace_52"],output_tensors=["sr"],runtime=Runtime.DSP,profile_level=PerfProfile.BURST,enable_cache=False)

# Intialize required buffers and load network
ret = model_object.Initialize()
if(ret != True):
    print("!"*50,"Failed to Initialize","!"*50)
    exit(0)

image = cv2.imread("Sample1.jpg")
model_object.preprocess([image,image])
if(model_object.Execute() != True):
    print("!"*50,"Failed to Execute","!"*50)
    exit(0)
model_object.postprocess()