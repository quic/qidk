"""
Python Helper Snippets over SNPE-NET_RUN Tool and APIs for Auto DLC Execution
"""
# @Author and Maintainer for this file : Pradeep Pant


import os
import numpy as np
import sys
import shutil
from typing import Dict, List, Tuple
from icecream import ic
from datetime import datetime
from pathlib import Path
from .logger_config import logger
from .exec_utils import exec_shell_cmd, exec_shell_cmd_in_batch, get_host_type
from .pysnpe_enums import *

try:
    from qti.aisw.dlc_utils import snpe_dlc_utils
    from qti.aisw.dlc_utils import modeltools
except ImportError as ie:
    logger.error("Failed to find necessary package:")
    logger.error(str(ie))
    logger.error("Please ensure that $SNPE_ROOT/lib/python is in your PYTHONPATH")


from .env_setup_checker import SnpeEnv

SNPE_ROOT = SnpeEnv().SNPE_ROOT 
ic(SNPE_ROOT)


def snpe_net_run_via_adb(remote_asset_loc, remote_dlc_path, input_list, dlc_folder, raw_folder, runtime, device_id, device_host="localhost"):
    logger.debug(f"Pushing SNPE Input List: {input_list} and Raws: {raw_folder}")
    
    if device_id:
        adb_prefix = f"adb -H {device_host} -s {device_id} "
    else:
        adb_prefix =f"adb -H {device_host} "
    ic(adb_prefix)

    exec_shell_cmd_in_batch(
        [
            adb_prefix + f" push {input_list} {dlc_folder}/." ,
            adb_prefix + f" push {raw_folder} {dlc_folder}/." ,

        ]
    )

    if "--use_cpu" in runtime:
        runtime = ""

    if "--use_gpu_fp16" in runtime:
        runtime = " --use_gpu --gpu_mode float16 "

    ic(runtime)

    # eg cmd:   adb -H localhost -s 1c7dec76  shell  cd /data/local/tmp/root/v2.7.0.4264/model_20_April_06_19_14  && export PATH=$PATH:/data/local/tmp/root/v2.7.0.4264/bin && export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/root/v2.7.0.4264/lib && export ADSP_LIBRARY_PATH=/data/local/tmp/root/v2.7.0.4264/dsp  &&  rm -rf ./output  &&  snpe-net-run --container model.dlc --input_list list.txt   --perf_profile burst --enable_cpu_fallback

    net_run_cmd = f" {remote_asset_loc}/bin/snpe-net-run --container {remote_dlc_path} --input_list {input_list} {runtime}  --perf_profile burst --profiling_level basic  --output_dir ./pysnpe_out --enable_cpu_fallback && ls -la"
    
    env_cmd = f" export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:{remote_asset_loc}/lib && export ADSP_LIBRARY_PATH={remote_asset_loc}/dsp "

    cd_cmd = f" cd {dlc_folder} "

    rm_cmd = f" rm -rf ./output  && ls -la "

    logger.debug(f"########   Running SNPE-Net-Run On-Device  ############")
    exec_shell_cmd_in_batch([ adb_prefix + " shell '" + cd_cmd + " && " + env_cmd + " && " + rm_cmd + " && " + net_run_cmd + "'" ])

    # delete any older dir, before pulling contents from device
    exec_shell_cmd("rm -rf ./pysnpe_out")

    # adb pull inference device output
    exec_shell_cmd_in_batch([ adb_prefix + f" pull {dlc_folder}" + '/pysnpe_out ./pysnpe_out' ])


def get_dlc_layer_details(dlc_path, input_layers:List[str], output_layers:List[str]):
    """
    {dlc_layer_name : dims}
    """
    ic(dlc_path)

    snpe_dlc_utils.setUpLogger(True)
    model_reader = modeltools.IrDlcReader()
    model_reader.open(dlc_path)

    graph = model_reader.get_ir_graph()
    
    # [TODO]: search with input layer name
    logger.debug("*********** INPUT DETAILS ***********")
    dict_input = {}

    # Fetch DLC Input layer names and shapes
    
    # [TODO]: Find API which prints input table / and also check in snpe-bench
    if len(input_layers)==0:   
        # Assume first layer is input layer
        for i, input_param in enumerate(graph.get_ops()[0].inputs()):
            in_name = input_param.name()
            if "weight" not in in_name and "bias" not in in_name and in_name not in dict_input:
                dict_input[in_name]= graph.get_ops()[0].get_input_shapes()[i]
    else:
        for op in graph.get_ops():
            for input_tensor in op.inputs():
                if input_tensor.name() in input_layers:
                    dict_input[input_tensor.name()]= input_tensor.dims()
            
    ic(dict_input)
        
    logger.debug("*********** OUTPUT DETAILS ***********")
    dict_output = {}
    
    # [TODO]: Update output layer detection logic for IF condition
    
    # Fetch DLC Output layer names and shapes
    if len(output_layers)==0:
        # Assume that last layer is output layer
        for output_param in graph.get_ops()[len(graph.get_ops())-1].outputs():
            if output_param.name() not in dict_output:
                dict_output[output_param.name()]= output_param.dims()
    else:
        for op in graph.get_ops():
            for output_tensor in op.outputs():
                if output_tensor.name() in output_layers:
                    dict_output[output_tensor.name()]= output_tensor.dims()
       
    ic(dict_output)    
    return dict_input, dict_output


def save_input_raw(path, file_content):
    # Save input tensor as Numpy RAW data file
    if not isinstance(file_content, np.ndarray):
        logger.warning(f"Expecting Numpy ND-ARRAY, got = {type(file_content)}. This may result in error ...")
    file_content.tofile(path)

    
def save_raw_for_Quantization(quant_input_folder, layer_name, raw_path):
    # ==== Backup_for_ Post Inference model Quantization ==== # 
    # Save raws which may be needed later for Model Quantization purpose
    os.makedirs(quant_input_folder, exist_ok=True)

    key_folder = ''.join(ch for ch in layer_name if ch.isalnum())   # modify non compatible filenames characters

    # dir to store raw for each input layer
    os.makedirs(quant_input_folder+"/"+key_folder, exist_ok=True)

    # copy raw file
    shutil.copy(raw_path, quant_input_folder+"/"+key_folder)
    
    # make raw name unique for Quantization folder
    raw_file_name = quant_input_folder+ "/" +key_folder+ "/" +str(layer_name)+ datetime.now().strftime('%Y-%m-%d-%H-%M-%S')+".raw"
    os.rename(quant_input_folder+"/"+key_folder+"/"+str(layer_name)+".raw", raw_file_name )

    return raw_file_name


def inference_on_device(dlc_path:str, remote_dlc_path:str, remote_asset_loc:str, inputs:Dict[str, np.ndarray], 
                        target_device, output_layers:List[str] = [], runtime="--use_dsp",  do_transpose=None):
    
    if len(inputs.keys()) == 0 or len(output_layers) == 0:
        raise RuntimeError(f"Inputs Tensor Map / Output Layers cannot be Empty.\
                            Got len. of Input Tensor Map = {len(inputs.keys())} \
                            Got len. of Output Layers = {len(output_layers)} ")
        
    # Get Target Device folder where DLC is kept
    device_folder = "/".join(remote_dlc_path.split("/")[:-1])
    ic(device_folder)
    
    ## get DLC IO layers Shape and DType
    dict_input, dict_output = get_dlc_layer_details(dlc_path, inputs.keys(), output_layers)
    
    # [TODO]: In future, check ndarray dtype with SNPE input layer dtype

    ## Input_raw_prepare
    shutil.rmtree("raw", ignore_errors=True, onerror=None)
    os.makedirs("raw", exist_ok=True)
        
    # TODO: Instead of relying on User, use output from "dict_output"
    inp_list_entry = "" 
    quant_inp_list_entry = ""

    # if there is only one output layer than no need to add it in InputList.txt
    if len(output_layers) > 1:
        inp_list_entry = "%"+" ".join(output_layers)+"\n"
        quant_inp_list_entry = "%"+" ".join(output_layers)+"\n"
    
    # Save Input Tensor in RAW buffer and Create Input list
    # by looping through User Provided Inputs
    for key, value in inputs.items():    
        if do_transpose:
            # Transpose for Data/Memory Format conversion
            if key in do_transpose.keys() and len(do_transpose[key]) != 0:
                inputs[key] = value.transpose(do_transpose[key]).astype(np.float32)
            else:
                inputs[key] = value.astype(np.float32) # explicit conversion to FLOAT32 dtype
        
        ## Checking if all input layer supplied are found in model
        if dict_input.get(key)==None:
            raise RuntimeError(f"Input Layer {key} is not found/doesn't match with name in DLC")
        
        ## Compare dict_input vs inputs[key].shape
        if list(inputs[key].shape) != dict_input[key]:
            logger.warning(f"Input layer: '{key}' dimension := {inputs[key].shape} mismatched with model input dimension := {dict_input[key]} \
            Please ignore if batch dimension with value 1 is mis-matching like [1,384,384] != [384,384]")
                    
        # User have to validate the shape for correctness
        logger.info(f"** DLC_Input['{str(key)}'] = {inputs[key].shape} **")

        # [TODO]: Compare the RAW-BUFFER/File size to be equal to DIMS*Dtype

        # Save input tensor as Numpy RAW data file
        raw_path = "raw/"+str(key)+".raw"
        save_input_raw(raw_path, inputs[key])

        # Prepare Input_list.txt
        inp_list_entry += key + ":=" + raw_path + " "

        # ==== Backup_for Post Inference DLC Quantization ==== # 
        quant_input_folder = "input_for_quantization_" + Path(dlc_path).stem
        quant_input_list = quant_input_folder + ".txt"
        quant_raw_path = save_raw_for_Quantization(quant_input_folder, key, raw_path)
        quant_inp_list_entry += key + ":=" + quant_raw_path + " "
    
    # Save inp_list_entry in pysnpe_inp_list.txt for snpe-net-run
    with open("pysnpe_inp_list.txt","w") as file:
        file.write(inp_list_entry.strip())
    
    # Save quant_inp_list_enty in quant_raw_path for snpe-dlc-quantize
    with open(quant_input_list, "a") as quant_file:
        quant_file.write(quant_inp_list_entry.strip() + "\n")

    # Run inference with snpe_net_run
    if target_device.device_protocol == DeviceProtocol.ADB:
        device_id = target_device.target_device_adb_id
        device_host = target_device.device_host
        snpe_net_run_via_adb(remote_asset_loc, remote_dlc_path, "pysnpe_inp_list.txt", device_folder, "raw", runtime, device_id, device_host)
    
    elif target_device.device_protocol == DeviceProtocol.NATIVE_BINARY:
        # delete any older dir, before pulling contents from device
        exec_shell_cmd("rm -rf ./pysnpe_out")

        net_run_cmd = f" {SNPE_ROOT}/bin/{get_host_type().value}/snpe-net-run --container {dlc_path} --input_list pysnpe_inp_list.txt --perf_profile burst --profiling_level basic --output_dir ./pysnpe_out "
        logger.debug("===== Executing Snpe-Net-Run on Native =====")
        exec_shell_cmd_in_batch([ net_run_cmd ])

    else:
        raise Exception("\n\nUnimplemented Protocol : ", target_device.device_protocol)

    # arranging output
    snpe_output_map = {}
    for file_name in os.listdir("./pysnpe_out/Result_0/"):
        out_tensor_name = file_name.replace(".raw","")
        snpe_output_map[out_tensor_name] = np.fromfile("./pysnpe_out/Result_0/" + file_name, dtype=np.float32)\
                                                .reshape(dict_output[out_tensor_name])
        logger.info(f"DLC Output['{file_name.replace('.raw','')}'] : {snpe_output_map[out_tensor_name].shape}")
        
    return snpe_output_map