#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

import subprocess
import os, platform, sys
from typing import List
from icecream import ic
import multiprocessing
import functools
import time

from .logger_config import logger
from .pysnpe_enums import DeviceType


def exec_shell_cmd(cmd: str, check=False, cwd=None):
    logger.info(f"\nExecuting CMD := {cmd}\n")
    python_version = sys.version_info
    exit_code = -1
    std_out = ""
    std_err = ""
    if (python_version[0] == 3 and python_version[1] >= 8):
        out = subprocess.run(cmd, env=os.environ, cwd=cwd, capture_output=True, shell=True)
        exit_code = out.returncode
        std_out = out.stdout.decode("utf-8")
        std_err = out.stderr.decode("utf-8")
    else:
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True, env=os.environ)
        exit_code = process.wait()
        logger.debug("Exit Code := " + str(exit_code))
        std_out = process.communicate()[0].decode("utf-8")

    if check and exit_code != 0:
        raise RuntimeError(f"Command '{cmd}' failed with return code {exit_code} and error: {std_err}")

    if std_out:
        logger.debug(f"stdout := {std_out}")
    if std_err:
        logger.debug(f"stderr := {std_err}")
    logger.debug("Exit Code := " + str(exit_code))
    return exit_code


def exec_shell_cmd_in_batch(batch_cmd_list: List[str]):
    for cmd in batch_cmd_list:
        exit_code = exec_shell_cmd(cmd)
        
        if exit_code != 0:
            raise RuntimeError("Failed to execute cmd : \n" + cmd)
            

def exec_shell_cmd_multiprocess(cmd: str):
    logger.info(f"\nExecuting CMD in another process := {cmd}\n")
    process = multiprocessing.Process(target=subprocess.call, args=(cmd,), kwargs={'shell': True, 'env': os.environ})
    process.start()
    process.join()
    exit_code = process.exitcode
    logger.debug("Exit Code := " + str(exit_code))
    return exit_code


def get_host_type() -> DeviceType:
    architecture = platform.machine().casefold()
    operating_system = platform.system()
    
    if "aarch64".casefold() in architecture or "arm64".casefold() in architecture:
        if "Windows" in operating_system:
            return DeviceType.ARM64_WINDOWS
        elif "Linux" in operating_system:
            return DeviceType.ARM64_UBUNTU
        else:
            logger.warning("Not able to identify Host Type. Assuming ARM64 UBUNTU")
            return DeviceType.ARM64_UBUNTU
    elif "x86_64".casefold() in architecture or "AMD64".casefold() in architecture:
        if "Windows" in operating_system:
            return DeviceType.X86_64_WINDOWS
        elif "Linux" in operating_system:
            return DeviceType.X86_64_LINUX
        else:
            logger.warning("Not able to identify Host Type. Assuming ARM64 UBUNTU")
            return DeviceType.X86_64_LINUX


def timer(func):
    @functools.wraps(func)
    def wrapper_timer(*args, **kwargs):
        tic = time.perf_counter()
        value = func(*args, **kwargs)
        toc = time.perf_counter()
        elapsed_time = toc - tic
        print(f"Elapsed time: {elapsed_time * 1000:.2f}ms")
        return value
    return wrapper_timer