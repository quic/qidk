import subprocess
import os, platform
from typing import List
from icecream import ic
import getpass
import multiprocessing

from .logger_config import logger
from .pysnpe_enums import DeviceType


def exec_shell_cmd(cmd: str):
    logger.info(f"\nExecuting CMD := {cmd}\n")
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True, executable='/bin/bash', env=os.environ)
    exit_code = process.wait()
    logger.debug("Exit Code := " + str(exit_code))

    output = process.communicate()[0]
    for line in output.splitlines():
        print(line)

    return exit_code


def exec_shell_cmd_in_batch(batch_cmd_list: List[str]):
    for cmd in batch_cmd_list:
        exit_code = exec_shell_cmd(cmd)
        
        if exit_code != 0:
            raise RuntimeError("Failed to execute cmd : \n" + cmd)
            

def exec_shell_cmd_multiprocess(cmd: str):
    logger.info(f"\nExecuting CMD in another process := {cmd}\n")
    process = multiprocessing.Process(target=subprocess.call, args=(cmd,), kwargs={'shell': True, 'executable': '/bin/bash', 'env': os.environ})
    process.start()
    process.join()
    exit_code = process.exitcode
    logger.debug("Exit Code := " + str(exit_code))
    return exit_code


def get_host_type() -> DeviceType:
    architecture = platform.machine()
    operating_system = platform.system()
    
    if "aarch64" in architecture or "arm64" in architecture:
        if "Windows" in operating_system:
            return DeviceType.ARM64_WINDOWS
        elif "Linux" in operating_system:
            return DeviceType.ARM64_UBUNTU
        else:
            logger.warning("Not able to identify Host Type. Assuming ARM64 UBUNTU")
            return DeviceType.ARM64_UBUNTU
    elif "x86_64" in architecture:
        if "Windows" in operating_system:
            return DeviceType.X86_64_WINDOWS
        elif "Linux" in operating_system:
            return DeviceType.X86_64_LINUX
        else:
            logger.warning("Not able to identify Host Type. Assuming ARM64 UBUNTU")
            return DeviceType.X86_64_LINUX

