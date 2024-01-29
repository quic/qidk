# -*- mode: python -*-
# =============================================================================
#  @@-COPYRIGHT-START-@@
#
#  Copyright (c) 2023 of Qualcomm Innovation Center, Inc. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause
#
#  @@-COPYRIGHT-END-@@
# =============================================================================
import functools
import time
import os

#Copy paste all dlls and so files to this location
os.add_dll_directory(os.getcwd())
import snpehelper #Make sure snpehelper.pyd (Python Extenson Module is in the same directory or in PYTHONPATH)

'''
Description:
        Python wrapper to measure a function E2E execution time

    Returns:
        Total Execution time in milliseconds
'''
def timer(func):
    @functools.wraps(func)
    def wrapper_timer(*args, **kwargs):
        tic = time.perf_counter()
        value = func(*args, **kwargs)
        toc = time.perf_counter()
        elapsed_time = toc - tic
        print(f"{str(func)} : Elapsed time: {elapsed_time * 1000:.2f}ms")
        return value
    return wrapper_timer

'''
Description:
        Available Performance modes for SNPE runtime
'''
class PerfProfile():
    DEFAULT = "DEFAULT"
    # Run in a balanced mode.
    BALANCED = "BALANCED"
    # Run in high performance mode
    HIGH_PERFORMANCE = "HIGH_PERFORMANCE"
    # Run in a power sensitive mode, at the expense of performance.
    POWER_SAVER = "POWER_SAVER"
    # Use system settings.  SNPE makes no calls to any performance related APIs.
    SYSTEM_SETTINGS = "SYSTEM_SETTINGS"
    # Run in sustained high performance mode
    SUSTAINED_HIGH_PERFORMANCE = "SUSTAINED_HIGH_PERFORMANCE"
    # Run in burst mode
    BURST = "BURST"
    # Run in lower clock than POWER_SAVER, at the expense of performance.
    LOW_POWER_SAVER = "LOW_POWER_SAVER"
    # Run in higher clock and provides better performance than POWER_SAVER.
    HIGH_POWER_SAVER = "HIGH_POWER_SAVER"
    # Run in lower balanced mode
    LOW_BALANCED = "LOW_BALANCED"
    # Run in extreme power saver mode 
    EXTREME_POWERSAVER = "EXTREME_POWERSAVER"

'''
Description:
        Available runtimes for model execution on Qualcomm® harwdware
'''
class Runtime():
    CPU = "CPU"
    DSP = "DSP"

class SnpeContext:
    """
    Description:
        Model attributes required for running inferences.
        On instantiation, the buffer allocation, performance level and runtime is selected,
        
        Runtime Selection Order: 
            CPU (Default)
            DSP (If specifed)

        If Runtime == DSP , then need to Push  Hexagon libs into the current directory.

        Args:
            dlc_path : Path to the DLC path on the device including model name like 'c:/Users/SESR_128_512_quantized_cached.dlc'.
            
            input_layers : Input layer name of the DLC. 
            
            output_layers : Output layer/s name of the DLC
            
            output_tensors : Output tensor/s of the DLC
            
            runtime : CPU or DSP runtime. Defaults to CPU

            profile_level : Performance level to run. Defaults to BALANCED

            enable_cache : To save cache data into DLC. Defaults to False
    """
    def __init__(self,dlc_path: str = "None",
                    input_layers : list = [],
                    output_layers : list = [],
                    output_tensors : list = [],
                    runtime : str = Runtime.CPU,
                    profile_level : str = PerfProfile.BALANCED,
                    enable_cache : bool = False):
        self.m_dlcpath = dlc_path 
        self.m_input_layers = input_layers 
        self.m_output_layers  = output_layers
        self.m_output_tensors = output_tensors 
        self.m_runtime = runtime
        self.profiling_level = profile_level
        self.enable_cache = enable_cache
        self.m_context = snpehelper.SnpeContext(self.m_dlcpath,self.m_input_layers,self.m_output_layers,self.m_output_tensors,self.m_runtime,self.profiling_level,self.enable_cache)
    
    """
    Description:
        Intializes Buffers and load network for SNPE execution

    Returns:
        SnpeContext instance
    """
    #@timer
    def Initialize(self):
        return self.m_context.Initialize()
    
    """
    Description:
        Overwrite Input buffer with data provided for input_layer
    """
    # Uncomment to measure time taken for this function 
    # NOTE: Additional time will be added because of "time" module
    
    #@timer
    def SetInputBuffer(self,input_data,input_layer):
        self.m_context.SetInputBuffer(input_data,input_layer)
        return
    
    """
    Description:
        Get inferenced buffers from SNPE based on output tensor

    Returns:
        Output buffer for specifies tensor
    """

    #@timer
    def GetOutputBuffer(self,Tensor):
        return self.m_context.GetOutputBuffer(Tensor)
    
    """
    Description:
        Run inference on the target

    Returns:
        True if execution is success; else False
    """
    #@timer
    def Execute(self):
        # This will be replacement for snpe-net-run
        return self.m_context.Execute()