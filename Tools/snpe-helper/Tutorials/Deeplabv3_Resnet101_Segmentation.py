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
from torch import from_numpy,argmax
import torchvision.transforms as T
from PIL import Image
from snpehelper_manager import PerfProfile,Runtime,timer,SnpeContext

class DeeplabV3_Resnet101(SnpeContext):
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
        transform = T.Compose([
                T.Resize((513,513)),
                
                T.ToTensor(),
                T.Normalize(mean=[0.485, 0.456, 0.406],
                                std=[0.229, 0.224, 0.225]),
            ])
        preprocess_image = []
        for image in images:
            img = transform(image).unsqueeze(0)
            input_image = img.numpy().transpose(0,2,3,1).astype(np.float32)
            preprocess_image.append(input_image[0].flatten())
        self.SetInputBuffer(preprocess_image,"input.1")
        return  

    """
    Description:
        Postprocess on inferenced data
        
        Specify output tensor to get model inferenced data for this tensor -> which returns 1D array
        
        Repeat GetOutputBuffer if there are multiple output nodes
        
        This will be replacement for opening result *.raw file
    """

    #@timer
    def postprocess(self,image):
        OutputBuffer = self.GetOutputBuffer("1089").reshape((1,513,513,21)).astype(np.float32)
        OutputBuffer = np.transpose(OutputBuffer,(0,3,1,2))
        image  = image.resize((513,513))

        model_img = from_numpy(OutputBuffer)

        segmented_image = self.draw_segmentation_map(model_img)
        final_image = self.image_overlay(image, segmented_image)
        cv2.imwrite("segmented_image.jpg", final_image)
        return 

    '''
    Additional functions required for postprocess
    '''

    def draw_segmentation_map(self, outputs):
        label_map = [
                    (0, 0, 0),  # background
                    (128, 0, 0), # aeroplane
                    (0, 128, 0), # bicycle
                    (128, 128, 0), # bird
                    (0, 0, 128), # boat
                    (128, 0, 128), # bottle
                    (0, 128, 128), # bus 
                    (128, 128, 128), # car
                    (64, 0, 0), # cat
                    (192, 0, 0), # chair
                    (64, 128, 0), # cow
                    (192, 128, 0), # dining table
                    (64, 0, 128), # dog
                    (192, 0, 128), # horse
                    (64, 128, 128), # motorbike
                    (192, 128, 128), # person
                    (0, 64, 0), # potted plant
                    (128, 64, 0), # sheep
                    (0, 192, 0), # sofa
                    (128, 192, 0), # train
                    (0, 64, 128) # tv/monitor
                ]
        labels = argmax(outputs.squeeze(), dim=0).detach().cpu().numpy()
        # create Numpy arrays containing zeros
        # later to be used to fill them with respective red, green, and blue pixels
        red_map = np.zeros_like(labels).astype(np.uint8)
        green_map = np.zeros_like(labels).astype(np.uint8)
        blue_map = np.zeros_like(labels).astype(np.uint8)
        
        for label_num in range(0, len(label_map)):
            index = labels == label_num
            red_map[index] = np.array(label_map)[label_num, 0]
            green_map[index] = np.array(label_map)[label_num, 1]
            blue_map[index] = np.array(label_map)[label_num, 2]
            
        segmentation_map = np.stack([red_map, green_map, blue_map], axis=2)
        return segmentation_map

    def image_overlay(self, image, segmented_image):
        alpha = 1 # transparency for the original image
        beta = 0.8 # transparency for the segmentation map
        gamma = 0 # scalar added to each sum
        segmented_image = cv2.cvtColor(segmented_image, cv2.COLOR_RGB2BGR)
        image = np.array(image)
        image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
        cv2.addWeighted(image, alpha, segmented_image, beta, gamma, image)
        return image

# Instance for SnpeContext 
model_object = DeeplabV3_Resnet101(dlc_path="deeplabv3_resnet101_quantized.dlc",input_layers=["input.1"], output_layers=["/Resize_1"],output_tensors=["1089"],runtime=Runtime.DSP,profile_level=PerfProfile.BURST,enable_cache=False)

# Intialize required buffers and load network
ret = model_object.Initialize()
if(ret != True):
    print("!"*50,"Failed to Initialize","!"*50)
    exit(0)

image = Image.open("plane.jpg").convert('RGB')
model_object.preprocess([image])
if(model_object.Execute() != True):
    print("!"*50,"Failed to Execute","!"*50)
    exit(0)
model_object.postprocess(image)