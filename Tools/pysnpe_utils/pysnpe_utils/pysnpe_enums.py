from enum import Enum
from typing import Tuple, Union
import numpy as np
from .logger_config import logger

# Give Warnings if TF, Torch and ONNX imports are not availble to their respective func
try:
    import torch
    import tensorflow as tf
except ImportError as ie:
    logger.warning(f"Not able to import package : {ie}")
    logger.warning("Some of the functionalities will be affected \n")


class Runtime(Enum):
    """
    Runtime:
        SNPE Runtimes for model accleration on Target IP Cores : DSP, GPU, CPU, AIP
    """
    DSP = " --use_dsp "
    CPU = " --use_cpu "
    GPU = " --use_gpu "
    GPU_FP16 = " --use_gpu_fp16 "
    AIP = " --use_aip "


class QuantScheme(Enum):
    """
    QuantScheme:
        SNPE Quantization Schemes for generating Weights and Act. Min-Max encodings
    """
    AXIS_QUANT = " --axis_quant "
    ENH_QUANT = " --use_enhanced_quantizer "
    ADJ_WIEGHT_QUANT = " --use_adjusted_weights_quantizer "
    SYMMETRIC_WEIGHT_QUANT = " --use_symmetric_quantize_weights "
    CLE = " --optimizations cle "


class WeightBw(Enum):
    """
    WeightBw:
        Weight Bitwidth to be used during Quantization: INT4, INT8, FP16
    """
    INT4 = " --weights_bitwidth 4 "
    INT8 = " --weights_bitwidth 8 "
    FP16 = " --float_bitwidth 16 "


class ActBw(Enum):
    """
    ActBw:
        Activaiton Bitwidth to be used during Quantization: INT8, INT16, FP16
    """
    INT8 = " --act_bitwidth 8 "
    INT16 = " --act_bitwidth 16 "


class DlcType(Enum):
    """
    DlcType:
        Type of Dlc : "FLOAT", "QUANT" to determine if the DLC has been Quantized or not

        SNPE Converter generate FLOAT DLC
        SNPE Quantize generates QUANT DLC
    """
    FLOAT = "FLOAT"
    QUANT = "QUANT"


class ModelFramework(Enum):
    """
    ModelType:
        Tells the Framework to be used for exporting the model
    """
    TF = "TF"
    ONNX = "ONNX" 
    CAFFE = "CAFFE"
    PYTORCH = "PYTORCH"
    TFLITE = "TFLITE"


class DeviceProtocol(Enum):
    """
    DeviceProtocol:
        Type of Protocol to be used for communicating with Target device
    """
    ADB = "ADB"
    PYBIND = "PYBIND"
    GRPC = "GRPC"
    TSHELL = "TSHELL"
    NATIVE_BINARY = "NATIVE_BINARY"


class DeviceType(Enum):
    """
    DeviceType:
        Type of Device Architecture and OS
    """
    ARM64_ANDROID = "aarch64-android-clang8.0"
    ARM64_UBUNTU = "aarch64-ubuntu-gcc7.5"
    ARM64_OELINUX = "aarch64-oe-linux-gcc9.3"
    ARM64_WINDOWS = "aarch64-windows-vc19"
    X86_64_LINUX = "x86_64-linux-clang"
    X86_64_WINDOWS = "x86_64-windows-vc19"


def check_dtype(dtype):
    if isinstance(dtype, tf.dtypes.DType):
        # TensorFlow data type
        itemsize = dtype.size
    elif isinstance(dtype, torch.dtype):
        # PyTorch data type
        if dtype.is_floating_point:
            itemsize = torch.finfo(dtype).bits / 8
        else:
            itemsize = torch.iinfo(dtype).bits / 8
    elif isinstance(dtype, np.dtype) or isinstance(np.dtype(dtype), np.dtype):
        # NumPy data type
        itemsize = np.dtype(dtype).itemsize
    else:
        raise TypeError("Datatype should be a NumPy, PyTorch, or TensorFlow data type object")

    if int(itemsize) > 4:
        # raise ValueError("The size of datatype should not be greater than 32 bits")
        logger.warning("The size of datatype should not be greater than 32 bits, else it may cause conversion issue with SNPE converters.")


class InputMap:
    def __init__(self, name: str, shape: Tuple, datatype: Union[np.dtype, torch.dtype, tf.dtypes.DType]):
        self.name = name
        self.shape = shape
        self.dtype = datatype

        check_dtype(self.dtype)

    def __repr__(self):
        return f"InputMap({self.name}, shape={self.shape}, datatype={self.dtype})"