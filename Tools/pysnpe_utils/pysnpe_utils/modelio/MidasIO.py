"""
Python Helper for Midas model pre and post processing
"""
# @Author and Maintainer: Pradeep Pant (ppant)


from pysnpe_utils.logger_config import logger
import numpy as np
from datetime import datetime
import os
import cv2
from .ModelIO import ModelIO

class MidasIO(ModelIO):
	def preprocess(self,imgfile):
		img = cv2.imread(imgfile)
		img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB) / 255.0
		img = cv2.resize(img, (384, 384), interpolation=cv2.INTER_CUBIC)
		img = np.transpose(img, (2, 0, 1))
		img = np.ascontiguousarray(img).astype(np.float32)
		return img
		
	def postprocess(self,img_path, output):
		depth = output["1080"][0]
		depth_min = depth.min()
		depth_max = depth.max()
		bits = 1
		max_val = (2**(8*bits))-1
		if depth_max - depth_min > np.finfo("float").eps:
			out = max_val * (depth - depth_min) / (depth_max - depth_min)
		else:
			out = 0
		if not os.path.exists("midas_output"):
			os.mkdir("midas_output")
		output_name = "midas_output/midas_"+datetime.now().strftime('%Y-%m-%d-%H-%M-%S')+".png"
		if bits == 1:
			cv2.imwrite(output_name, out.astype("uint8"))
		elif bits == 2:
			cv2.imwrite(output_name, out.astype("uint16"))
		elif bits == 3:
			cv2.imwrite(output_name, out.astype("np.float32"))
		logger.debug("output_name : "+output_name)