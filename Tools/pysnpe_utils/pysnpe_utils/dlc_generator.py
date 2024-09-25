#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

"""
Python Helper Snippets over SNPE-CONVERTER, QUANTIZER and GRAPH-PREPARE Tool and APIs for Auto DLC Generation, Quantization and DSP caching
"""
# @Author and Maintainer for this file : Shubham Patel (shubpate)


from enum import Flag
from typing import Dict, List, Tuple
from icecream import ic
import numpy as np
from pathlib import Path
import os

from .logger_config import logger
from .exec_utils import exec_shell_cmd, exec_shell_cmd_multiprocess, get_host_type
from .env_setup_checker import SnpeEnv


SNPE_ROOT = SnpeEnv().SNPE_ROOT 
ic(SNPE_ROOT)


def dimension_as_str(dimension_list: Tuple[int]) -> str:
    # Replace None shape with 1
    dimension_list = [dimension or 1 for dimension in dimension_list]
    # Get first shape
    dimension_str = str(dimension_list[0])
    # Get rest of the shape : comma sep
    for idx in range(1, len(dimension_list)):
        dimension_str += f",{dimension_list[idx]}"
    return dimension_str


def snpe_dlc_quant(dlc_path, quant_schemes, output_dlc_name, act_bw, weight_bw, override_quant_params):
    quant_input_folder = "input_for_quantization_" + Path(dlc_path).stem
    quant_input_list = quant_input_folder + ".txt"

    if os.path.exists(quant_input_list):
        quant_cmd = f"  {SNPE_ROOT}/bin/{get_host_type().value}/snpe-dlc-quant --input_dlc {dlc_path} --input_list {quant_input_list} --output_dlc {output_dlc_name} --bias_bitwidth 32 --act_bitwidth {act_bw} --weights_bitwidth {weight_bw} "

        if override_quant_params:
            quant_cmd += " --override_params "

        for scheme in quant_schemes:
            quant_cmd += f" {scheme.value} "

        return exec_shell_cmd( quant_cmd )
    else:
        logger.warning(f"Quant Input List => {quant_input_list} do not exist. Please run some inferences with 'SnpeContext.execute_dlc()' API first and then run this function")
        return -1


def snpe_throughput_net_run_on_host(dlc_path:str, runtime:str, duration:int = 2, 
                                    num_threads:int = 1, cpu_fallback:bool = True):
    throughput_cmd = f" {SNPE_ROOT}/bin/{get_host_type().value}/snpe-throughput-net-run --duration {duration} "
    for idx in range(num_threads):
        throughput_cmd += f" --perf_profile burst --container {dlc_path} {runtime} "

    exec_shell_cmd( throughput_cmd )


def snpe_throughput_net_run_via_adb(remote_dlc_path:str, device_storage_location:str,
                            runtime:str, duration:int, 
                            num_threads:int, cpu_fallback:bool = True,
                            device_id:str=None, device_host:str="localhost",
                            target_arch:str="aarch64-android-clang8.0"):
    if device_id:
        adb_prefix = f"adb -H {device_host} -s {device_id} "
    else:
        adb_prefix =f"adb -H {device_host} "

    # eg cmd:  adb -H $HOST shell "export SNPE_TARGET_ARCH=aarch64-android-clang8.0 && export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/pysnpe_bench/$SNPE_TARGET_ARCH/lib && export ADSP_LIBRARY_PATH=/data/local/tmp/pysnpe_bench/dsp && /data/local/tmp/pysnpe_bench/snpe-throughput-net-run --duration 2 --perf_profile burst --container /data/local/tmp/pysnpe_bench/${DLC} ${RUNTIME}"

    throughput_cmd = f" {device_storage_location}/bin/snpe-throughput-net-run --duration {duration} "
    for idx in range(num_threads):
        throughput_cmd += f" --perf_profile burst --container {remote_dlc_path} {runtime} "
    
    exec_shell_cmd(adb_prefix + f" shell 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:{device_storage_location}/lib && export ADSP_LIBRARY_PATH={device_storage_location}/dsp && {throughput_cmd} '")


def snpe_dlc_graph_prepare(dlc_path:str, chipsets:List, out_dlc_path:str=None, 
                            output_tensor_names:List=None, 
                            is_fp16:bool=False,
                            overwrite_cache_records:bool=True) -> int:
    global SNPE_ROOT

    snpe_cmd = f"{SNPE_ROOT}/bin/x86_64-linux-clang/snpe-dlc-graph-prepare \
             --input_dlc {dlc_path} "
    
    # Prepare chipset list
    htp_socs_list = " --htp_socs " + ','.join(chipsets)
    snpe_cmd += htp_socs_list
    

    if out_dlc_path:
        snpe_cmd += f" --output_dlc {out_dlc_path} "

    # Prepare output tensor list
    if output_tensor_names:
        out_tensor_list = " --set_output_tensors " + ','.join(output_tensor_names)
    
    if is_fp16:
        snpe_cmd += " --use_float_io "

    if overwrite_cache_records:
        snpe_cmd += " --overwrite_cache_records "

    return exec_shell_cmd(snpe_cmd)


def snpe_onnx_to_dlc(model_path, dlc_path, input_tensor_map: Dict[str, Tuple[int]], 
                    output_names: List, quant_encodings_path: str = None) -> int:
    global SNPE_ROOT
    
    input_dims = ""
    # ic(input_tensor_map)
    for name in input_tensor_map:
        input_dims += f" --input_dim {name} {dimension_as_str(input_tensor_map[name])} " 
    # ic(input_dims)

    out_nodes = ""
    for name in output_names: out_nodes += f" --out_node {name} "

    snpe_cmd = f"python {SNPE_ROOT}/bin/x86_64-linux-clang/snpe-onnx-to-dlc \
            -i {model_path} \
            {input_dims} \
            {out_nodes} \
            -o {dlc_path} "
    if quant_encodings_path:
        snpe_cmd += f" --quantization_overrides {quant_encodings_path} "

    return exec_shell_cmd(snpe_cmd)


def snpe_tensorflow_to_dlc(model_path, dlc_path, input_tensor_map: Dict[str, List[int]], 
                            output_names: List, quant_encodings_path: str = None):
    global SNPE_ROOT

    input_dims = ""
    # ic(input_tensor_map)
    for name in input_tensor_map:
        input_dims += f" --input_dim {name.rsplit(':', 1)[0]} {dimension_as_str(input_tensor_map[name])} " 
    # ic(input_dims)

    out_nodes = ""
    for name in output_names: out_nodes += f" --out_node {name.rsplit(':', 1)[0]} "

    snpe_cmd = f"python {SNPE_ROOT}/bin/x86_64-linux-clang/snpe-tensorflow-to-dlc \
                -i {model_path} \
                {input_dims} \
                {out_nodes} \
                -o {dlc_path} \
                --show_unconsumed_nodes "
    if quant_encodings_path:
        snpe_cmd += f" --quantization_overrides {quant_encodings_path} "

    return exec_shell_cmd_multiprocess(snpe_cmd)


def snpe_pytorch_to_dlc(model_path, dlc_path, input_tensor_map: Dict[str, List[int]], 
                            output_names: List, quant_encodings_path: str = None):
    global SNPE_ROOT

    input_dims = ""
    # ic(input_tensor_map)
    for name in input_tensor_map:
        input_dims += f" --input_dim {name} {dimension_as_str(input_tensor_map[name])} " 
    # ic(input_dims)

    out_nodes = ""
    for name in output_names: out_nodes += f" --out_node {name} "

    snpe_cmd = f"python {SNPE_ROOT}/bin/x86_64-linux-clang/snpe-pytorch-to-dlc \
                -i {model_path} \
                {input_dims} \
                {out_nodes} \
                -o {dlc_path} "

    if quant_encodings_path:
        snpe_cmd += f" --quantization_overrides {quant_encodings_path} "

    return exec_shell_cmd(snpe_cmd)


def snpe_tflite_to_dlc(model_path, dlc_path, input_tensor_map: Dict[str, List[int]], 
                            output_names: List, quant_encodings_path: str = None):
    global SNPE_ROOT

    input_dims = ""
    # ic(input_tensor_map)
    for name in input_tensor_map:
        input_dims += f" --input_dim {name} {dimension_as_str(input_tensor_map[name])} " 
    # ic(input_dims)

    out_nodes = ""
    for name in output_names: out_nodes += f" --out_node {name} "

    snpe_cmd = f"python {SNPE_ROOT}/bin/x86_64-linux-clang/snpe-tflite-to-dlc \
                -i {model_path} \
                {input_dims} \
                {out_nodes} \
                -o {dlc_path} "
                
    if quant_encodings_path:
        snpe_cmd += f" --quantization_overrides {quant_encodings_path} "

    return exec_shell_cmd(snpe_cmd)


def snpe_dlc_viewer(dlc_path:str, save_path:str=None) -> int:
    if save_path == None:
        save_path = dlc_path.replace(".dlc", "_graph_visual.html")
    snpe_cmd = f"python {SNPE_ROOT}/bin/x86_64-linux-clang/snpe-dlc-viewer \
                 -i {dlc_path} \
                     -s {save_path} "

    return exec_shell_cmd_multiprocess(snpe_cmd)