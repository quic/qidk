#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

"""
Python Helper for Mobilenet model pre and post processing
"""
# @Author and Maintainer: Pradeep Pant (ppant)


from pysnpe_utils.logger_config import logger
import cv2
import numpy as np
import os
from datetime import datetime
from .ModelIO import ModelIO

class MobilenetIO(ModelIO):
    dict_class_mapping={}
    dict_class_mapping[0]= "background"
    dict_class_mapping[1]= "aeroplane"
    dict_class_mapping[2]= "bicycle"
    dict_class_mapping[3]= "bird"
    dict_class_mapping[4]= "boat"
    dict_class_mapping[5]= "bottle"
    dict_class_mapping[6]= "bus"
    dict_class_mapping[7]= "car"
    dict_class_mapping[8]= "cat"
    dict_class_mapping[9]= "chair"
    dict_class_mapping[10]= "cow"
    dict_class_mapping[11]= "diningtable"
    dict_class_mapping[12]= "dog"
    dict_class_mapping[13]= "horse"
    dict_class_mapping[14]= "motorbike"
    dict_class_mapping[15]= "person"
    dict_class_mapping[16]= "pottedplant"
    dict_class_mapping[17]= "sheep"
    dict_class_mapping[18]= "sofa"
    dict_class_mapping[19]= "train"
    dict_class_mapping[20]= "tvmonitor"
    
    def preprocess(self, imgfile):
        origimg = cv2.imread(imgfile)
        img = cv2.resize(origimg, (300,300))
        img = img - 127.5
        img = img * 0.007843
        img = img.astype(np.float32)
        # img.tofile("raw/"+filenames[i].split(".")[0]+".raw")
        return img
    def postprocess(self, actual_img, output):
        img = cv2.imread(actual_img)
        for i in range(0,int(output["detection_out_num_detections"][0])):
            x0_y0 = (int(output["detection_out_boxes"][0][i][0]*img.shape[1]), int(output["detection_out_boxes"][0][i][1]*img.shape[0]))
            x1_y1 = (int(output["detection_out_boxes"][0][i][2]*img.shape[1]), int(output["detection_out_boxes"][0][i][3]*img.shape[0]))
            cv2.rectangle(img, x0_y0, x1_y1, color=(255), thickness=2)
            cv2.putText(img, str(dict_class_mapping[int(output["detection_out_classes"][0][i])]), (x0_y0[0], x0_y0[1]+15), cv2.FONT_HERSHEY_SIMPLEX, 0.9,  (255,0,230), 2)
        if not os.path.exists("mobilenet_output"):
            os.mkdir("mobilenet_output")
        output_name = "mobilenet_output/mobilenet_"+datetime.now().strftime('%Y-%m-%d-%H-%M-%S')+".jpg"
        cv2.imwrite(output_name, img)
        logger.debug("output_name"+{output_name})