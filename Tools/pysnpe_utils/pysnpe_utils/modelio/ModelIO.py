#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

"""
ModelIO abstract class having preprocess and postprocess methods needs to be implement for any new model.
"""
# @Author and Maintainer: Pradeep Pant (ppant)


from abc import ABC, abstractmethod
from pysnpe_utils.logger_config import logger

class ModelIO(ABC):
  
    @abstractmethod
    def preprocess(self,img_path):
        logger.debug("preprocess method")


    @abstractmethod
    def postprocess(self,img_path, output):
        logger.debug("preprocess method")
