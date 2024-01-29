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
import functools
import torch
import torchvision.transforms as T
torch.set_grad_enabled(False);
from PIL import Image
import matplotlib.pyplot as plt
from snpehelper_manager import PerfProfile,Runtime,timer,SnpeContext

class DETR(SnpeContext):
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
    def preprocess(self,image):
        transform = T.Compose([
                                T.Resize(800),
                                T.ToTensor(),
                                T.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])
                            ])
        img = transform(image).unsqueeze(0)
        out = torch.nn.functional.interpolate(img, size=(800, 1066), mode='bicubic', align_corners=False)
        input_image = out.numpy().transpose(0,2,3,1).astype(np.float32)
        input_image = input_image[0].flatten()
        self.SetInputBuffer(input_image,"samples")
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
        prob = self.GetOutputBuffer("5859")
        prob = prob.reshape(1,100,92)
        tensor_prob = torch.from_numpy(prob)
        
        boxes = self.GetOutputBuffer("5860")
        boxes = boxes.reshape(1,100,4)
        tensor_boxes = torch.from_numpy(boxes)

        probas = tensor_prob.softmax(-1)[0, :, :-1]
        keep = probas.max(-1).values > 0.9
        bboxes_scaled = self.rescale_bboxes(tensor_boxes[0, keep], image.size)
        self.plot_results(image, probas[keep], bboxes_scaled,"detection.jpg")
        return 

    # For output bounding box post-processing
    def box_cxcywh_to_xyxy(self,x):
        x_c, y_c, w, h = x.unbind(1)
        b = [(x_c - 0.5 * w), (y_c - 0.5 * h),
            (x_c + 0.5 * w), (y_c + 0.5 * h)]
        return torch.stack(b, dim=1)

    def rescale_bboxes(self,out_bbox, size):
        img_w, img_h = size
        b = self.box_cxcywh_to_xyxy(out_bbox)
        b = b * torch.tensor([img_w, img_h, img_w, img_h], dtype=torch.float32)
        return b

    def plot_results(self,pil_img, prob, boxes,output_image):
        # COCO classes
        CLASSES = [
            'N/A', 'person', 'bicycle', 'car', 'motorcycle', 'airplane', 'bus',
            'train', 'truck', 'boat', 'traffic light', 'fire hydrant', 'N/A',
            'stop sign', 'parking meter', 'bench', 'bird', 'cat', 'dog', 'horse',
            'sheep', 'cow', 'elephant', 'bear', 'zebra', 'giraffe', 'N/A', 'backpack',
            'umbrella', 'N/A', 'N/A', 'handbag', 'tie', 'suitcase', 'frisbee', 'skis',
            'snowboard', 'sports ball', 'kite', 'baseball bat', 'baseball glove',
            'skateboard', 'surfboard', 'tennis racket', 'bottle', 'N/A', 'wine glass',
            'cup', 'fork', 'knife', 'spoon', 'bowl', 'banana', 'apple', 'sandwich',
            'orange', 'broccoli', 'carrot', 'hot dog', 'pizza', 'donut', 'cake',
            'chair', 'couch', 'potted plant', 'bed', 'N/A', 'dining table', 'N/A',
            'N/A', 'toilet', 'N/A', 'tv', 'laptop', 'mouse', 'remote', 'keyboard',
            'cell phone', 'microwave', 'oven', 'toaster', 'sink', 'refrigerator', 'N/A',
            'book', 'clock', 'vase', 'scissors', 'teddy bear', 'hair drier',
            'toothbrush'
        ]
        # colors for visualization
        COLORS = [[0.000, 0.447, 0.741], [0.850, 0.325, 0.098], [0.929, 0.694, 0.125],
                [0.494, 0.184, 0.556], [0.466, 0.674, 0.188], [0.301, 0.745, 0.933]]
        
        fig=plt.figure(figsize=(8,8))
        ax1=fig.add_subplot(1,1,1)
        ax1.imshow(pil_img)
        ax = plt.gca()
        colors = COLORS * 100
        for p, (xmin, ymin, xmax, ymax), c in zip(prob, boxes.tolist(), colors):
            ax.add_patch(plt.Rectangle((xmin, ymin), xmax - xmin, ymax - ymin,
                                    fill=False, color=c, linewidth=3))
            cl = p.argmax()
            text = f'{CLASSES[cl]}: {p[cl]:0.2f}'
            ax.text(xmin, ymin, text, fontsize=15,
                    bbox=dict(facecolor='yellow', alpha=0.5))
        plt.savefig(str(output_image))
        #plt.show()

# Instance for SnpeContext 
model_object = DETR(dlc_path="detr_resnet101_quantized.dlc",input_layers=["samples"], output_layers=["Reshape_post_/Gather_2","Reshape_post_/Gather_3"],output_tensors=["5859","5860"],runtime=Runtime.DSP,profile_level=PerfProfile.BURST,enable_cache=False)
# Intialize required buffers and load network
ret = model_object.Initialize()
if(ret != True):
    print("!"*50,"Failed to Initialize","!"*50)
    exit(0)

image = Image.open("plane.jpg")
model_object.preprocess(image)
if(model_object.Execute() != True):
    print("!"*50,"Failed to Execute","!"*50)
    exit(0)
model_object.postprocess(image)