"""
Python API wrapper over SNPE Tools and APIs for Auto DLC Generation, its Execution, Easy Integration and On-Device Prototyping of your DNN project.
"""
# @Author and Maintainer for this file : Shubham Patel (shubpate)


from logging import log
import os
from datetime import datetime
from typing import List, Dict, Tuple
from icecream import ic
import copy
import getpass

# Pysnpe Module imports
from .logger_config import logger, set_logging_level
from .dlc_generator import snpe_onnx_to_dlc, snpe_tensorflow_to_dlc, snpe_dlc_graph_prepare
from .dlc_generator import snpe_pytorch_to_dlc, snpe_tflite_to_dlc, snpe_dlc_quant
from .dlc_generator import snpe_dlc_viewer, snpe_throughput_net_run_via_adb, snpe_throughput_net_run_on_host
from .dlc_executor import inference_on_device
from .pysnpe_enums import *
from .exec_utils import exec_shell_cmd, get_host_type
from .env_setup_checker import get_snpe_version
from .asset_manager import push_assets_on_target_device_via_adb, push_dlc_on_device_via_adb

set_logging_level("DEBUG")

# Give Warnings if TF, Torch and ONNX imports are not availble to their respective func
try:
    import numpy as np
    import tensorflow as tf
    from tensorflow.python.framework.convert_to_constants import convert_variables_to_constants_v2
    import torch
    import onnx
    from onnx import version_converter
    import onnxruntime
    from ppadb.client import Client as AdbClient
except ImportError as ie:
    logger.warning(f"Not able to import package : {ie}")
    logger.warning("Some of the functionalities will be affected \n")


class TargetDevice:

    def __init__(self, target_device_type: DeviceType = DeviceType.ARM64_ANDROID,
                    device_host: str = "localhost",
                    target_device_adb_id: str = None,
                    target_device_ip: str = None,
                    send_root_access_request:bool = True):
        """
        Description:
            Target Device attributes required to prepare it for running inferences.
            On instantiation, the DeviceProtocol is selected based on target-device type (architecture + os),
            and subsequently the artifacts(binaries and libraries) are pushed onto the device.

        Protocol Selection Order: 
            ARM64_ANDROID = ADB                                 <br>
            ARM64_UBUNTU = ADB | NATIVE_BINARY | PYBIND         <br>
            ARM64_OELINUX = ADB                                 <br>
            ARM64_WINDOWS = NATIVE_BINARY | PYBIND | TSHELL     <br>
            X86_64_LINUX = NATIVE_BINARY | PYBIND               <br>
            X86_64_WINDOWS = NATIVE_BINARY | PYBIND             <br>

        If Protocol == NATIVE_BINARY | PYBIND , then no need for Pushing artifacts onto the device.
        Artifacts will be fetched from "SNPE_ROOT" on device itself.

        Args:
            target_device_type (DeviceType, optional): Target device architecture and OS info. Defaults to DeviceType.ARM64_ANDROID.
            
            device_host (str, optional): Host Name/IP on which target device is connected. Defaults to "localhost".
            
            target_device_adb_id (str, optional): ADB Serail ID of target device to uniquely identify when multiple devices are connected on Host machine. Serial ID can be found using `adb devices -l` command. Defaults to None.
            
            target_device_ip (str, optional): IP address of target device. If provided, this is help to make a wireless TCP/IP connection to the device using ADB or GRPC protocol. Defaults to None.
            
            send_root_access_request (bool, optional): Requests target device to run commands with root access.
        """
        self.target_device_type = target_device_type
        self.device_host = device_host
        self.target_device_adb_id = target_device_adb_id
        self.target_device_ip = target_device_ip

        # Select Target Device communication protocol
        self.setDeviceProtocol()

        # Push artifacts (SNPE libs and bins) if needed:
        self.device_storage_loc = self.prepareArtifactsOnsDevice(send_root_access_request=send_root_access_request)


    def setDeviceProtocol(self, device_protocol:DeviceProtocol = None):
        """
        Description:
            Sets Protocol to be used for communication with Target device.
        """
        if device_protocol:
            self.device_protocol = device_protocol
        else:
            if self.target_device_type == DeviceType.ARM64_ANDROID:
                self.device_protocol = DeviceProtocol.ADB
            elif self.target_device_type == DeviceType.ARM64_UBUNTU:
                self.device_protocol = DeviceProtocol.ADB
            elif self.target_device_type == DeviceType.ARM64_OELINUX:
                self.device_protocol = DeviceProtocol.ADB
            elif self.target_device_type == DeviceType.ARM64_WINDOWS:
                self.device_protocol = DeviceProtocol.NATIVE_BINARY
            elif self.target_device_type == DeviceType.X86_64_LINUX:
                self.device_protocol = DeviceProtocol.NATIVE_BINARY
            elif self.target_device_type == DeviceType.X86_64_WINDOWS:
                self.device_protocol = DeviceProtocol.NATIVE_BINARY
            else:
                logger.critical(f"Unsupported Target Device type = {self.target_device_type}")


    def prepareArtifactsOnsDevice(self, location_to_store:str = None,
                                    send_root_access_request:bool = False) -> str:
        """
        Description:
            Push artifacts (SNPE Libs and Bins) onto the Target Device, based on DeviceProtocol. 

        Returns:
            Storage location of the assets of Target Device.
        """
        if location_to_store:
            device_storage_loc = location_to_store
        else:
            # eg: /data/local/tmp/shubpate/v2.7.0.2048/ => bin ; lib ; dsp ; dlc
            device_storage_loc = f"/data/local/tmp/{getpass.getuser()}/{get_snpe_version()}" 

        if self.device_protocol == DeviceProtocol.ADB:
            logger.debug("Querying for ADB devices:")
            client = AdbClient(host=self.device_host, port=5037)
            adb_devices = client.devices()
            logger.debug(f"Got {len(adb_devices)} ADB devices connected on {self.device_host}")
            
            if len(adb_devices) == 0:
                self.device_protocol = DeviceProtocol.NATIVE_BINARY
                self.target_device_type = get_host_type()
                logger.warning("No ADB devices found. Will do Profiling and Execution on this Native machine")
                return os.getcwd()

            if not self.target_device_adb_id:
                logger.debug(f"Fetching Serial ID of Target Device")
                for device_obj in adb_devices:
                    self.target_device_adb_id = device_obj.get_serial_no()
                    logger.debug(f"Selected Device with serial id = {self.target_device_adb_id}")
                    break
                    
                if not self.target_device_adb_id:
                    raise RuntimeError("Not able to fetch Serail ID of device")

            logger.debug(f"Pushing assets on Device using {self.device_protocol}")
            push_assets_on_target_device_via_adb(device_storage_loc, 
                                        target_arch=self.target_device_type,
                                        device_id=self.target_device_adb_id,
                                        device_host=self.device_host, 
                                        send_root_access_request=send_root_access_request)

        elif self.device_protocol == DeviceProtocol.NATIVE_BINARY:
            logger.info(f"DeviceProtocol.NATIVE_BINARY searches for SNPE assets using $PATH and $LD_LIBRARY_PATH")
            logger.info("It is assumed at SNPE SDK is available on this device")
            device_storage_loc = os.getcwd()

        return device_storage_loc


    __pdoc__ = {'__repr__': False}
    def __repr__(self):
        return f"""TargetDevice(target_device_type={self.target_device_type}, 
                        device_protocol={self.device_protocol},
                        device_host='{self.device_host}', 
                        target_device_adb_id='{self.target_device_adb_id}', 
                        target_device_ip='{self.target_device_ip}')"""

class SnpeContext:

    def __init__(self, model_path:str, 
                    model_framework:ModelFramework,
                    dlc_path:str,
                    input_tensor_map: Dict[str, List[int]], 
                    output_tensor_names: List[str],
                    quant_encodings_path: str = None,
                    target_device: TargetDevice = None,
                    remote_session_name: str = None):
        """
        Description:
            Stores metadata needed to generate a DLC and for other DLC operations

        Args:
            model_path (str): Path of freezed graph which is to be converted to DLC
            model_framework (ModelFramework): Specifies the Model framework : TF, ONNX, CAFFE, PYTORCH, TFLITE
            dlc_path (str): Path to save generated DLC
            input_tensor_map (Dict[str, List]): Dict of the model's input names and their shape
            output_tensor_names (List[str]): List of the model's output names
            quant_encodings_path (str): Path to quantization encodings file (mostly generate using AIMET)
            target_device (TargetDevice): Target Device on which inference has to be executed.
            remote_session_name (str): Represents path at Target Device, where DLC and input tensors will be pushed.
        """
        self.model_path = model_path
        self.model_framework = model_framework
        self.dlc_path = dlc_path
        self.quant_dlc_path = None
        self.input_tensor_map = input_tensor_map
        self.output_tensor_names = output_tensor_names
        self.quant_encodings_path = quant_encodings_path
        self.target_device = target_device
        self.remote_session_name = remote_session_name

        if not self.target_device:
            self.target_device = self.set_target_device()


    def set_target_device(self, target_device: TargetDevice = None):
        """
        Description:
            Adds Target Device into current SnpeContext, on which inference has to be done
        """
        if target_device:
            self.target_device = target_device
        else:
            logger.debug("Querying for ADB devices:")
            client = AdbClient()
            adb_devices = client.devices()
            logger.debug(f"Got {len(adb_devices)} ADB devices")
            
            if len(adb_devices) == 0:
                self.target_device = TargetDevice(target_device_type=get_host_type())
            else:
                self.target_device = TargetDevice()

        return self


    def to_dlc(self):
        """
        Description:
            Converts freezed graph into DLC by invoking `SNPE Converter`
        """
        if self.model_framework == ModelFramework.ONNX:
            exit_code = snpe_onnx_to_dlc(self.model_path, self.dlc_path, self.input_tensor_map, 
                            self.output_tensor_names, self.quant_encodings_path)
        elif self.model_framework == ModelFramework.TF:
            exit_code = snpe_tensorflow_to_dlc(self.model_path, self.dlc_path, self.input_tensor_map, 
                            self.output_tensor_names, self.quant_encodings_path)
        elif self.model_framework == ModelFramework.TFLITE:
            exit_code = snpe_tflite_to_dlc(self.model_path, self.dlc_path, self.input_tensor_map, 
                            self.output_tensor_names, self.quant_encodings_path)
        elif self.model_framework == ModelFramework.PYTORCH:
            exit_code = snpe_pytorch_to_dlc(self.model_path, self.dlc_path, self.input_tensor_map, 
                            self.output_tensor_names, self.quant_encodings_path)
        elif self.model_framework == ModelFramework.CAFFE:
            raise NotImplementedError(f'Support for CAFFE models is getting deprecated from SNPE')
        else:
            raise RuntimeError(f'Got Unsupported Model Type: {self.model_type}')
        
        if exit_code != 0:
            raise Exception(f"SNPE Converter returned non-zero Exit Code = {exit_code}")

        return self


    def gen_dsp_graph_cache(self, dlc_type:DlcType,  chipsets:List[str] = ["sm8550"], 
                            overwrite_cache_records:bool=True):
        """
        Description:
            Generates DSP Offline Cache (serialized graph) to save model initialization time, by invoking `snpe-dlc-graph-prepare` tool

            On success, the `self.dlc_path` or `self.quant_dlc_path` will get updated with newly generated cached dlc path

        Args:
            dlc_type (DlcType): Type of DLC : FLOAT | QUANT . QUANT dlc is generated by SNPE Quantizer.
            chipset (List[str], optional): List of chipsets for which Graph cache needs to generated [sm8350,sm8450,sm8550]. Defaults to ["sm8550"].
            overwrite_cache_records (bool, optional): Overwrite previously generated Graph Cache. Defaults to True.
        """
        is_fp16 = False
        if dlc_type == DlcType.FLOAT:
            is_fp16 = True
            out_dlc_path_name = self.dlc_path.replace(".dlc", "_dsp_fp16_cached.dlc")
        else:
            out_dlc_path_name = self.dlc_path.replace(".dlc", "_dsp_cached.dlc")

        exit_code = snpe_dlc_graph_prepare(self.dlc_path, chipsets, 
                                out_dlc_path=out_dlc_path_name,
                                output_tensor_names=self.output_tensor_names,
                                is_fp16=is_fp16, overwrite_cache_records=overwrite_cache_records)

        # Overwrites DLC name with newly generated DLC
        if exit_code == 0:
            if dlc_type == DlcType.FLOAT:
                self.dlc_path = out_dlc_path_name
            else:
                self.quant_dlc_path = out_dlc_path_name

        return self


    def visualize_dlc(self, save_path: str = None):
        """
        Description:
            Saves DLC graph structure in HTML and Tabular textual format by invoking `snpe-dlc-viewer` and `snpe-dlc-info` tools

        Args:
            save_path (str, optional): Path to save visualization output. If None, then output is save with save DLC name as prefix and ".html" and ".txt" as suffix
        """
        if not save_path:
            save_path = self.dlc_path.replace(".dlc", ".html")
        snpe_dlc_viewer(self.dlc_path, save_path=save_path)
        return self


    __pdoc__ = {'__set_remote_session_name': False}   
    def __set_remote_session_name(self, dlc_path:str, target_device:TargetDevice):
        if not self.remote_session_name:
            timestamp = datetime.now()
            unique_id = f"{timestamp.strftime('%d_%B')}"
            # unique_id = f"{timestamp.strftime('%d_%B')}_{timestamp.strftime('%I_%M_%S')}"
            self.remote_session_name = f"{target_device.device_storage_loc}/{dlc_path.replace('.dlc','')}_{unique_id}"
            logger.debug(f"Remote session name = {self.remote_session_name}")


    def profile(self, runtime: Runtime = Runtime.CPU, 
                        dlc_type: DlcType = DlcType.FLOAT,
                        target_device: TargetDevice = None,
                        num_threads: int = 1, 
                        duration: int = 2, 
                        cpu_fallback: bool = True):
        """
        Description:
            Profiles model execution time on provided runtime and gives metrics: Inference per second, Model Init and DeInit times

        Args:
            runtime (Runtime, optional): Runtime on which DLC will be executed [CPU, GPU, GPU_FP16, DSP, AIP]. Defaults to CPU runtime
            target_device (TargetDevice, optional): Target device on which profiling for DLC is to be done. Defaults to None.
            num_threads (int, optional): Number of threads to be used for DLC execution. Defaults to 1
            duration (int, optional): Duration of time (in seconds) to run network execution. Defaults to 2 seconds.
            cpu_fallback (bool, optional): Fallback unsupported layer to CPU, if any. Defaults to True.
        """

        if target_device is None:
            target_device = self.target_device
        logger.debug(f"Profiling will done on device : {target_device}")

        if dlc_type == DlcType.QUANT:
            dlc_path = self.quant_dlc_path
        else:
            dlc_path = self.dlc_path

        # if remote_session_name is not provided, create a unique name
        self.__set_remote_session_name(dlc_path, target_device)

        # push DLC at session_name path (push will be skipped, if file exists at loc)
        if target_device.device_protocol == DeviceProtocol.ADB:
            if self.quant_dlc_path:
                remote_dlc_path = push_dlc_on_device_via_adb(self.remote_session_name, self.quant_dlc_path, target_device.target_device_adb_id, target_device.device_host)
            else:
                remote_dlc_path = push_dlc_on_device_via_adb(self.remote_session_name, self.dlc_path, target_device.target_device_adb_id, target_device.device_host)
            
            print("="*35 + " Througput Test " + "="*35 + "\n")
            snpe_throughput_net_run_via_adb(remote_dlc_path, target_device.device_storage_loc, 
                                    runtime.value, duration, num_threads, 
                                    cpu_fallback=cpu_fallback, 
                                    device_id=target_device.target_device_adb_id,
                                    device_host=target_device.device_host, 
                                    target_arch=target_device.target_device_type)
            print("\n" + "="*80 + "\n")
        
        elif target_device.device_protocol == DeviceProtocol.NATIVE_BINARY:
            snpe_throughput_net_run_on_host(dlc_path, runtime.value, duration, num_threads, cpu_fallback)

        else:
            logger.critical("\n\nUnimplement Protocol : ", target_device.device_protocol)
            
        return self


    def quantize(self, quant_scheme: List[QuantScheme]=[QuantScheme.AXIS_QUANT], 
                    act_bw:ActBw=ActBw.INT8, 
                    weight_bw:WeightBw=WeightBw.INT8, 
                    override_quant_params=False):
        """
        Description:
            Quantizes DLC using provided quant_scheme, activation bitwidth and weight bitwidth

        Args:
            quant_scheme (List[QuantScheme]): List of SNPE provided quant schemes. Defaults to Axis Quantization
            act_bw (ActBw, optional): Activation bitwidth (16 or 8). Defaults to 8.
            weight_bw (int, optional): Weight bitwidth (8 or 4). Defaults to 8.
            override_quant_params (bool, optional): Use quant encodings provided from encodings file. Defaults to False.
        """
        quant_dlc_name = self.dlc_path.replace(".dlc", "_quantized.dlc")
        exit_code = snpe_dlc_quant(self.dlc_path, quant_scheme, quant_dlc_name, act_bw, weight_bw, override_quant_params)

        if exit_code == 0:
            self.quant_dlc_path = quant_dlc_name
        else:
            logger.warning(f"DLC Quantization returned Non Zero exit code = {exit_code}")
        
        return self


    def execute_dlc(self, 
                    input_tensor_map: Dict[str, np.ndarray],
                    dlc_type: DlcType = DlcType.FLOAT,
                    transpose_input_order: Dict[str, Tuple] = None,
                    target_acclerator: Runtime = Runtime.CPU,
                    target_device: TargetDevice = None,
                    ) -> Dict[str, np.ndarray]:
        """
        Description:
            Executes DLC on target device or x86/ARM64 host machine using 'snpe-net-run' for ADB protocol
            and using 'snpe python bindings' for PYBIND and GRPC protocol.

        Args:
            input_tensor_map (Dict[str, np.ndarray]): Input tensor name and its tensor data in Numpy Nd-Array FP32 format.

            dlc_type (DlcType): Whether the DLC is FLOAT type or QUANT type.

            
            transpose_input_order (Dict[str, Tuple]): SNPE expects the input tensor to be NHWC (Batch x Height x Width x Channel) format, whereas DNN Frameworks like Pytorch, ONNX uses NCHW (Batch x Channel x Height x Width) format. Providing transpose order will make the input tensor in format acceptable to SNPE. For example, ONNX input dim = (1,3,224,224) and SNPE DLC input dim = (1,224,224,3), then providing Dictionary {'layer_name': (0,2,3,1)} will do the needful format conversion.
            
            target_acclerator (Runtime): Snapdragon DSP, CPU, GPU, GPU-FP16 and legacy AIP acclerator.

            target_device (TargetDevice): Target device on which DLC is to be executed. Defaults to None.

        Returns:
            Dict[str, np.ndarray]: Output Tensor Name : its Output Tensor, generated after inference
        """

        if not target_device:
            target_device = self.target_device

        if dlc_type == DlcType.QUANT:
            dlc_path = self.quant_dlc_path
        else:
            dlc_path = self.dlc_path

        # if remote_session_name is not provided/ not set, create a unique name
        self.__set_remote_session_name(dlc_path, target_device)

        # push DLC at remote_session_name path (push will be skipped, if file already exists at loc)
        if target_device.device_protocol == DeviceProtocol.ADB:
            if self.quant_dlc_path:
                remote_dlc_path = push_dlc_on_device_via_adb(self.remote_session_name, self.quant_dlc_path, target_device.target_device_adb_id, target_device.device_host)
            else:
                remote_dlc_path = push_dlc_on_device_via_adb(self.remote_session_name, self.dlc_path, target_device.target_device_adb_id, target_device.device_host)
        
        elif target_device.device_protocol == DeviceProtocol.NATIVE_BINARY:
            remote_dlc_path = dlc_path
        else:
            raise Exception("\n\nUnimplement Protocol : ", target_device.device_protocol)        
        
        # Run inference
        return inference_on_device(dlc_path, remote_dlc_path, target_device.device_storage_loc, input_tensor_map, target_device,  output_layers=self.output_tensor_names, runtime=target_acclerator.value, do_transpose=transpose_input_order)

        # [TODO]: For ONNX : Instead of returning Dict of output tensors, validate if shapes are same,
        # else Raise a warning that Transpose is needed


    __pdoc__ = {'__repr__': False}
    def __repr__(self):
        return f"""SnpeContext(model_path='{self.model_path}', 
                model_framework={self.model_framework}, 
                dlc_path='{self.dlc_path}', 
                input_tensor_map={self.input_tensor_map}, 
                output_tensor_names={self.output_tensor_names}, 
                quant_encodings_path='{self.quant_encodings_path}', 
                remote_session_name='{self.remote_session_name}'
                target_device={self.target_device})"""


def export_to_onnx(model: torch.nn.Module,
                    input_tensor_map: List[InputMap], 
                    output_tensor_names: List[str], 
                    onnx_file_name: str,
                    opset_version: int = 11,
                    optimization_level: int = 1,
                    keep_flexible_opset: bool = True ) -> SnpeContext:
    """
    Description:
        Exports Pytorch model (nn.Module) to ONNX format and saves it on disk.

    Args:
        model (torch.nn.Module): A PyTorch model (nn.Module) to export to ONNX format.
        input_tensor_map (List[InputMap]): A list of `InputMap` object: (input_name, shape, dtype)
        output_tensor_names (List[str]): List of output names for the ONNX model.
        onnx_file_name (str): The filename for the exported ONNX model.
        opset_version (int, optional): ONNX opset version to use. Defaults to 11.
        optimization_level (int, optional): ONNX Graph Optimization {0 = ORT_DISABLE_ALL; 1 = ORT_ENABLE_BASIC; 2 = ORT_ENABLE_EXTENDED ; 3 = ORT_ENABLE_ALL }
        keep_flexible_opset (bool, optional): Let Onnx Runtime decide the best suitable opset version

    Returns:
        SnpeContext Class instance required to generate DLC and perform other DLC operations

    Raises:
        RuntimeError: If an error occurs during the export process.

    Example:
        ```python
        from pysnpe_utils import pysnpe

        # Create a PyTorch model and specify the input shape
        model = MyModel()

        input_map = InputMap("img_1", (1, 3, 224, 224), torch.float32)
        output_tensor_names = ["out_1"]
        onnx_file_name = 'model.onnx'
        
        pysnpe.export_to_onnx(model, [input_map], output_tensor_names, onnx_file_name).to_dlc()
        ```
    """
    logger.debug("Set the Pytorch model to evaluation mode")
    model.eval()

    logger.debug("Push the Pytorch model to CPU")
    model.to('cpu')

    input_names = [input_map.name for input_map in input_tensor_map]
    input_shapes = [input_map.shape for input_map in input_tensor_map]

    logger.debug("Create dummy input tensors for each input shape")
    dummy_inputs = []
    for input_map in input_tensor_map:
        dummy_inputs.append(torch.zeros(input_map.shape, dtype=input_map.dtype))

    logger.debug("Export the model to ONNX format")
    with torch.no_grad():
        try:
            torch.onnx.export(model, 
                            tuple(dummy_inputs), 
                            onnx_file_name, 
                            input_names=input_names, 
                            output_names=output_tensor_names,
                            opset_version=opset_version)
        except Exception as e:
            raise RuntimeError(f"Error occurred during export: {e}") 

    logger.info("Output ONNX model saved at : " + onnx_file_name)
    
    logger.debug("Load the exported model and check its correctness with ONNX Checker")
    onnx_model = onnx.load(onnx_file_name)
    onnx.checker.check_model(onnx_model)

    logger.debug("Simplifying/Optimizing ONNX model for inference with Static Shapes")
    try:
        from onnxsim import simplify
        sim_onnx_model, check = simplify(onnx_model)
        sim_onnx_file_name = onnx_file_name.replace(".onnx", "-simplified.onnx")
        logger.debug(f"Saving simplified ONNX model : {onnx_file_name}")
        onnx.save(copy.deepcopy(sim_onnx_model), sim_onnx_file_name)
    except ImportError as ie:
        logger.warning("Not able to Simplify ONNX model with ONNXSIM pkg.")

    logger.debug(f"Create ONNX Runtime Session with optimization level = {optimization_level}")
    sess_options = onnxruntime.SessionOptions()
    
    ort_optimization_levels = {
        0: onnxruntime.GraphOptimizationLevel.ORT_DISABLE_ALL,
        1: onnxruntime.GraphOptimizationLevel.ORT_ENABLE_BASIC,
        2: onnxruntime.GraphOptimizationLevel.ORT_ENABLE_EXTENDED,
        3: onnxruntime.GraphOptimizationLevel.ORT_ENABLE_ALL
    }
    # Set graph optimization level
    sess_options.graph_optimization_level = ort_optimization_levels.get(optimization_level, 1)

    opt_onnx_file_name = sim_onnx_file_name.replace(
                                            ".onnx", 
                                            f"-opt_{optimization_level}.onnx"
                                        )
    sess_options.optimized_model_filepath = opt_onnx_file_name
    logger.info(f"Saving optimized ONNX model : {opt_onnx_file_name}")
    
    ort_session = onnxruntime.InferenceSession(sim_onnx_model.SerializeToString(), sess_options)

    logger.debug("Verify ONNX model functional correctness with dummy inputs")
    ort_inputs = {input_names[i] : dummy_inputs[i].numpy() for i in range(len(input_names))}
    ort_outputs = ort_session.run(None, ort_inputs)

    logger.debug("ONNX model's input shapes")
    for i in range(len(input_names)):
        logger.debug(f"Input {i} := {input_names[i]} : {input_shapes[i]}")

    logger.debug("ONNX model's output shapes")
    for i, output_name in enumerate(output_tensor_names):
        logger.debug(f"Output {i} := {output_name} - Shape: {ort_outputs[i].shape}")

    # if optimized onnx file exists else 
    if os.path.isfile(opt_onnx_file_name):
        if os.path.getsize(opt_onnx_file_name) > 0:
            onnx_file_reference = opt_onnx_file_name

            # Make Opset Corrections, if not flexible_opset:
            if not keep_flexible_opset:
                opt_onnx_model = onnx.load(onnx_file_reference)
                converted_model = version_converter.convert_version(opt_onnx_model, opset_version)
                onnx_file_reference = onnx_file_reference.replace(".onnx", 
                                                            f"-opset_{opset_version}.onnx")
                onnx.save(copy.deepcopy(converted_model), onnx_file_reference)

    elif os.path.isfile(sim_onnx_file_name):
        if os.path.getsize(sim_onnx_file_name) > 0:
            onnx_file_reference = sim_onnx_file_name
    else:
        onnx_file_reference = onnx_file_name

    logger.info(f"the ONNX model := {onnx_file_reference}, can be used for exporting to SNPE DLC")
    return SnpeContext(onnx_file_reference, 
                    ModelFramework.ONNX,
                    onnx_file_name.replace(".onnx", ".dlc"),
                    {input_names[i] : input_shapes[i] for i in range(len(input_names))}, 
                    output_tensor_names)


def export_to_torchscript(model: torch.nn.Module, 
                            input_shapes: List[Tuple[int]], 
                            input_dtypes: List[torch.dtype], 
                            output_file_path: str,
                            enable_optimizations: bool = True,
                            strict_tracing: bool = True) -> SnpeContext:
    """
    Exports a PyTorch model to TorchScript format and saves it to disk.

    Args:
        model (torch.nn.Module): A PyTorch model (nn.Module) to export to TorchScript format.
        input_shape (List[Tuple[int]]): A tuple specifying the shape of the input tensor for the model.
        input_dtypes (List[torch.dtype]) : A tuple specifying the datatype of the input tensor for the model.
        output_file_path (str): A string specifying the file path to which the exported TorchScript model will be saved.
        enable_optimizations (bool): Flag to enable Graph optimizations, which are helpful for inference
        strict_tracing (bool): Use 'strict' model while tracing torch module

    Returns:
        None

    Raises:
        RuntimeError: If an error occurs during the export process.

    Example:
        ```python
        from pysnpe_utils import pysnpe

        # Create a PyTorch model and specify the input shape
        model = MyModel()
        input_shapes = [(3, 224, 224)]
        input_dtypes = [torch.float32]

        # Export the model to TorchScript format
        output_file_path = 'model.pt'
        pysnpe.export_to_torchscript(model, input_shape,input_dtypes, output_file_path).to_dlc()
        ```
    """
    
    logger.debug("Set the Pytorch model to evaluation mode")
    model.eval()

    logger.debug("Pushing the Pytorch model to CPU")
    model.to('cpu')

    logger.debug("Create an dummy input tensor")
    example_inputs = []
    for shape, dtype in zip(input_shapes, input_dtypes):
        example_inputs.append(torch.zeros(shape, dtype=dtype))

    logger.debug("Wrap the export process inside torch.no_grad context manager")
    with torch.no_grad():
        with torch.jit.optimized_execution(enable_optimizations):
            logger.debug("Trace the model to create a TorchScript module")
            traced_module = torch.jit.trace(model, tuple(example_inputs), strict=strict_tracing)

        if enable_optimizations:
            try:    
                logger.debug("Optimizing the traced module using graph mode optimization for inference")
                traced_module, _ = torch.jit.optimize_for_inference(traced_module)
            except Exception as e:
                logger.warning(f"Error occurred during Model Optimization: {e}.\n Skipping optimizations...")

    torch.jit.save(traced_module, output_file_path)
    logger.debug(f"Torchscript module saved at {output_file_path}")

    import inspect
    input_names = inspect.signature(model.forward).parameters.keys()

    logger.debug("Fetch input names and dimensions")
    input_dict = {}
    for name, shape in zip(input_names, input_shapes):
        input_dict[name] = shape
    ic(input_dict)

    output_list = []

    logger.info("Export the Torchscript model to SNPE DLC")
    return SnpeContext(output_file_path, 
                    ModelFramework.PYTORCH,
                    output_file_path.replace(".pt", ".dlc"),
                    input_dict, 
                    output_list)


def export_to_tflite(keras_model: tf.keras.Model, tflite_file_name: str) -> None:
    """
    Description:
        Converts a TensorFlow Keras model to TFLite format.

    Args:
        keras_model (tf.keras.Model): A TensorFlow Keras model.
        tflite_file_name (str): A string indicating the name of the output TFLite file to be saved.

    Returns:
        SnpeContext Class instance required to generate DLC and perform other DLC operations

    Raises:
        RuntimeError: If an error occurs during the export process.

    Example:
        ```python
            model = tf.keras.Sequential([
                tf.keras.layers.Conv2D(32, (3,3), activation='relu', input_shape=(28,28,1)),
                tf.keras.layers.MaxPooling2D((2,2)),
                tf.keras.layers.Flatten(),
                tf.keras.layers.Dense(10, activation='softmax')
            ])
            model.build()
            model.summary()

            # Convert and save the model
            export_to_tflite(model, 'test_model.tflite').to_dlc()
        ```
    """
    logger.debug("Creating a TFLITE converter object")
    converter = tf.lite.TFLiteConverter.from_keras_model(keras_model)

    # logger.debug("Setting input and output shapes of TFLITE model")
    # for i, input_node in enumerate(keras_model.inputs):
    #     converter.optimizations = [tf.lite.Optimize.DEFAULT]
    #     input_shape = input_node.shape.as_list()[1:]
    #     converter.inference_input_type(i, tf.float32, input_shape)

    # for i, output_node in enumerate(keras_model.outputs):
    #     output_shape = output_node.shape.as_list()[1:]
    #     converter.inference_output_type(i, tf.float32, output_shape)

    logger.debug("Converting the model into TFLITE")
    tflite_model = converter.convert()

    logger.debug(f"Saving the TFLite model with name : {tflite_file_name}")
    with open(tflite_file_name, 'wb') as f:
        f.write(tflite_model)

    logger.debug(f"Loading and Validate the TFLite model: {tflite_file_name}")
    interpreter = tf.lite.Interpreter(model_path=tflite_file_name)
    interpreter.allocate_tensors()

    logger.debug("Fetching input and output tensors details")
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    # Loop through input details and get input names and shapes.
    input_dict = {}
    for input_tensor in input_details:
        input_dict[input_tensor['name']] = input_tensor['shape']
    ic(input_dict)

    # Loop through output details and print names and shapes.
    output_list = []
    for output_tensor in output_details:
        output_list.append(output_tensor['name'])
    ic(output_list)

    logger.info("Export the TFLITE model to SNPE DLC")
    return SnpeContext(tflite_file_name, 
                    ModelFramework.TFLITE,
                    tflite_file_name.replace(".tflite", ".dlc"),
                    input_dict, 
                    output_list)


def export_to_tf_keras_model(model: Union[tf.keras.Model, tf.function],
                             input_tensor_map: List[InputMap],
                             keras_model_path: str,
                             skip_model_optimizations: bool = False,
                             frozen_graph_path: str = None,
                             tflite_path: str = None) -> SnpeContext:
    """
    Description:
        Optimizes and Saves a TensorFlow Keras model to disk, with fixed input shapes for each input layer.

    Args:
        model (tf.keras.Model | tf.function): A TensorFlow model to export, either a TensorFlow Keras model or a TensorFlow function.
        input_tensor_map (List[InputMap]): A list of `InputMap` object: (input_name, shape, dtype)
        keras_model_path (str): Path to save keras model.
        skip_model_optimizations (bool, optional): Whether to skip optimization passes in the TensorFlow graph. Default is False.
        frozen_graph_path (str, optional): Path to save optimized frozen graph. Defaults to None.
        tflite_path: If provided, the function will convert the model to a TensorFlow Lite model and save it to the specified tflite_path.

    Returns:
        SnpeContext Class instance required to generate DLC and perform other DLC operations

    Raises:
        RuntimeError: If an error occurs during the export process.

    Example:
        ```python
            model = tf.keras.Sequential([
                tf.keras.layers.Conv2D(32, (3,3), activation='relu', input_shape=(28,28,1)),
                tf.keras.layers.MaxPooling2D((2,2)),
                tf.keras.layers.Flatten(),
                tf.keras.layers.Dense(10, activation='softmax')
            ])
            model.build()
            model.summary()

            # Define the input tensor shapes
            input_map = [ InputMap('input_1', (1,28,28,1), tf.float32) ]

            # Optimize and save the model
            export_to_tf_keras_model(model, 
                                    input_map, 
                                    'test_keras_model',
                                    skip_model_optimizations=False, 
                                    frozen_graph_path='test_frozen_graph.pb')
        ```
    """

    logger.debug("Create the input signature from the input tensor map")
    ic(input_tensor_map)

    input_signature = []
    dummy_input_tensors = []
    keras_input_layers = []
    for input_map in input_tensor_map:
        input_signature.append(tf.TensorSpec(shape=input_map.shape, dtype=input_map.dtype,
                             name=input_map.name.rsplit(':', 1)[0]))
        
        keras_input_layers.append(tf.keras.Input(batch_shape=input_map.shape,
                                  name=input_map.name, dtype=input_map.dtype))
        
        dummy_input_tensors.append(tf.random.uniform(shape=input_map.shape, dtype=input_map.dtype, maxval=5))
    ic(input_signature)
    ic(keras_input_layers)

    if isinstance(model, tf.keras.Model):
        logger.debug(f"Model of type {type(model)} is a tf.keras.Model instance")
        outputs = model(*keras_input_layers)
        keras_model = tf.keras.Model(inputs=keras_input_layers, outputs=outputs)
        keras_model.summary()
        tf.keras.models.save_model(keras_model, keras_model_path) #, signatures=input_signature)
        logger.info(f"Model sucessfully saved in tf keras format at: {keras_model_path}")
        del keras_model

        logger.info("Warping tf.keras.Model in tf.function with input signature")
        concrete_function = tf.function(model, input_signature=input_signature).get_concrete_function()
    else:
        # Assume model is a function / tf.function, add the input signature to the function
        concrete_function = model.get_concrete_function(*input_signature)
        # tf.keras.models.save_model(concrete_function, keras_model_path, signatures=input_signature)
        # logger.info(f"Concrete Function saved in tf keras format at: {keras_model_path}")

    logger.info("Ensuring that the given Model is compatible with given Input shapes")
    # Build the Keras model to ensure all layers are built
    concrete_function(*dummy_input_tensors)  
    
    # ================================TF FROZEN GRAPH GEN ================================ #
    try:
        logger.debug("Convert the ConcreteFunction to a TensorFlow graph")
        from tensorflow.python.framework.convert_to_constants import convert_variables_to_constants_v2
    except ImportError as ie:
        logger.warning(
            f"Failed to find necessary package for TF graph optimization: {str(ie)}")
        logger.warning(
            "Skipping TF Frozen graph generation and optimizations ...")

        # Fallback to TF Keras Saved Model for DLC generation and return
        keras_model = tf.saved_model.load(keras_model_path)

        # get first signatures of the model
        input_signature = list(keras_model.signatures.keys())[0]

        # input layer names and shapes
        input_signature_dict = keras_model.signatures[input_signature].structured_input_signature[1]
        input_layers_dict = {spec.name: spec.shape for spec in input_signature_dict.values()}
        ic(input_layers_dict)

        # output layer names
        output_layers_name = list(keras_model.signatures[input_signature].output_dtypes.keys())
        ic(output_layers_name)
        
        del keras_model
        return SnpeContext(keras_model_path,
                           ModelFramework.TF,
                           keras_model_path + ".dlc",
                           input_layers_dict,
                           output_layers_name)

    frozen_func = convert_variables_to_constants_v2(concrete_function)
    graph_def = frozen_func.graph.as_graph_def()
    graph_def = tf.compat.v1.graph_util.remove_training_nodes(graph_def)

    layers = [op.name for op in frozen_func.graph.get_operations()]
    logger.debug("-" * 50)
    logger.debug("NO. of Frozen model layers: {}".format(len(layers)))
    logger.debug("-" * 50)
    logger.debug(f"Frozen model inputs: \n{frozen_func.inputs}")
    logger.debug(f"Frozen model outputs: \n{frozen_func.outputs}")

    # get inputs for snpe-tensorflow-to-dlc (name: shape)
    input_layers_dict = {}
    for input_tensor in frozen_func.inputs:
        if input_tensor.dtype != tf.resource:
            input_layers_dict[input_tensor.name] = input_tensor.shape.as_list()
    ic(input_layers_dict)

    # get outputs for snpe-tensorflow-to-dlc
    output_layers_name = [output_tensor.name for output_tensor in frozen_func.outputs]
    ic(output_layers_name)

    if frozen_graph_path:
        logger.debug(f"Set Save path for TensorFlow frozen graph = {frozen_graph_path}")
    else:
        frozen_graph_path = keras_model_path + ".pb"
        logger.debug(f"Save path for TensorFlow frozen graph = {frozen_graph_path}")

    if skip_model_optimizations:
        logger.info("Skipping optimizations as requested")
        with tf.io.gfile.GFile(frozen_graph_path, 'wb') as f:
            f.write(graph_def.SerializeToString())

        return SnpeContext(frozen_graph_path,
                           ModelFramework.TF,
                           frozen_graph_path.replace(".pb", ".dlc"),
                           input_layers_dict,
                           output_layers_name)
    else:
        with tf.io.gfile.GFile("pre_optimization_" + frozen_graph_path, 'wb') as f:
            f.write(graph_def.SerializeToString())

    # ===================================TF GRAPH OPTIMIZATIONS ================================ #
    try:
        logger.debug("Optimize using Tensorflow Grappler")
        from tensorflow.lite.python.util import run_graph_optimizations, get_grappler_config
        from tensorflow.python.tools.optimize_for_inference_lib import optimize_for_inference
    except ImportError as ie:
        logger.warning(f"Failed to find necessary package for TF graph optimization: {str(ie)}")

        logger.warning("Returning and saving TF graph without optimizations ...")
        with tf.io.gfile.GFile(frozen_graph_path, 'wb') as f:
            f.write(graph_def.SerializeToString())

        return SnpeContext(frozen_graph_path,
                           ModelFramework.TF,
                           frozen_graph_path.replace(".pb", ".dlc"),
                           input_layers_dict,
                           output_layers_name)

    # tsr: short of "tensor" (list of inputs type)
    input_tensors = [tsr for tsr in frozen_func.inputs if tsr.dtype != tf.resource]
    ic(input_tensors)

    output_tensors = frozen_func.outputs
    
    input_tsr_names = [tsr.name for tsr in input_tensors] # name only
    output_tsr_names = [tsr.name for tsr in output_tensors]
    input_node_names = list(set([tsr_name.rsplit(':', 1)[0] for tsr_name in input_tsr_names]))
    output_node_names = list(set([tsr_name.rsplit(':', 1)[0] for tsr_name in output_tsr_names]))

    input_types = [spec.dtype.as_datatype_enum for spec in input_signature] # dtypes only

    # this from TF Graph Transform
    graph_def = optimize_for_inference(
        graph_def, input_node_names, output_node_names, input_types)

    # this is from TF-MOT
    graph_def = run_graph_optimizations(graph_def, input_tensors, output_tensors,
                                        config=get_grappler_config(
                                            [
                                                'function',
                                                'inline',
                                                'arithmetic',
                                                'dependency',
                                                'constfold',
                                                # 'layout',
                                                # 'remap',
                                                # 'loop_opt',
                                                # 'memory',
                                                # 'common_subgraph_elimination'
                                            ]),
                                        graph=frozen_func.graph)

    with tf.io.gfile.GFile(frozen_graph_path, 'wb') as f:
        f.write(graph_def.SerializeToString())
    logger.info(f"Model successfully exoprted to TF-Frozen Graph format at : {frozen_graph_path}")

    # Saves as tF-Lite if requested
    if tflite_path:
        converter = tf.lite.TFLiteConverter.from_concrete_functions(
            [concrete_function])
        tflite_model = converter.convert()
        with open(tflite_path, 'wb') as f:
            f.write(tflite_model)
        logger.info(f"Model successfully exoprted to TF-Lite format at : {tflite_model}")

    logger.debug("Deleting Model and Graph Def instance")
    del model
    del graph_def

    logger.debug("Clearing Keras backend Session")
    tf.keras.backend.clear_session()

    return SnpeContext(frozen_graph_path,
                           ModelFramework.TF,
                           frozen_graph_path.replace(".pb", ".dlc"),
                           input_layers_dict,
                           output_layers_name)


def export_to_tf_frozen_graph(session: tf.compat.v1.Session,
                                input_tensor_map: Dict[str, List],
                                output_tensor_names: List,
                                output_model_path: str) -> SnpeContext:
    """Exports TF GraphDef to Frozen Graph (.pb) format

    Args:
        session (tf.compat.v1.Session): The TensorFlow Session containing the trained model.
        input_tensor_map (Dict[str, List]): A dictionary of model input layer names with their dimensions. It can be found out by visualization of model.
        output_tensor_names (List): A list of model output layer names. It can be found out by visualization of model.
        output_model_path (str): The name and path of the model for saving as TF frozen graph with ".pb" extension.
    """
    # Create a graph definition for the current session
    graph_def = session.graph_def

    # Create a list of input tensors from the input tensor map
    input_tensors = {}
    for input_name, input_shape in input_tensor_map.items():
        input_tensor = tf.compat.v1.placeholder(dtype=tf.float32, shape=input_shape, name=input_name)
        input_tensors[input_name] = input_tensor
    ic(input_tensors)

    # Create a list of output tensors from the output tensor names
    output_tensors = []
    for output_name in output_tensor_names:
        output_tensor = session.graph.get_tensor_by_name(output_name + ':0')
        output_tensors.append(output_tensor)
    ic(output_tensors)

    # Freeze the graph
    frozen_graph_def = tf.compat.v1.graph_util.convert_variables_to_constants(session, graph_def, [output.name[:-2] for output in output_tensors])

    # Save the frozen graph to a file
    with tf.io.gfile.GFile(output_model_path, 'wb') as f:
        f.write(frozen_graph_def.SerializeToString())

    logger.info(f'MOdel saved as frozen graph at {output_model_path}')


def visualize_tf_session_graph(session: tf.compat.v1.Session,
                                model_name: str,
                                output_dir: str):
    """
    Description:
        For visualization of TF Session graph with Netron Viewer. It is helpful for identifying model input and output layer names and understanding model topology.

    Args:
        session (tf.compat.v1.Session): The TF Session containing Graph.
        model_name (str): Name for frozen graph to be saved with ".pb" extension.
        output_dir (str): Directory where to write the graph.
    """
    graph_def = session.graph_def
    tf.io.write_graph(graph_or_graph_def=graph_def,
                  logdir=output_dir,
                  name=model_name,
                  as_text=False)


def visualize_tf_keras_model(model: tf.keras.Model, output_file_path: str):
    """
    Description:
        Visualizes a TensorFlow Keras model and saves the visualization to a file.

    Args:
        model: A TensorFlow Keras model or a TensorFlow function.
        output_file_path: The path to save the visualization file.
    """
    if isinstance(model, tf.keras.Model):
        logger.warning(f"Model must be a TensorFlow Keras model")
        logger.warning(f"Got Unexpected Model instance type : {type(model)}")
        logger.warning("Visualization of graph may get failed.")
        
    tf.keras.utils.plot_model(model, to_file=output_file_path, 
                                  show_shapes=True, show_dtype=True)