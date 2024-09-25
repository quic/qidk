#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

from logging import raiseExceptions
import os
import hashlib
import subprocess
from icecream import ic

from .logger_config import logger
from .exec_utils import exec_shell_cmd_in_batch, exec_shell_cmd
from .pysnpe_enums import *
from .env_setup_checker import SnpeEnv

SNPE_ROOT = SnpeEnv().SNPE_ROOT 
ic(SNPE_ROOT)


def get_md5sum(filename):
    hash_md5 = hashlib.md5()
    with open(filename, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return str(hash_md5.hexdigest())


def get_root_access_via_adb(device_id:str = None, 
                            device_host:str = "localhost") -> None:
    if device_id:
        adb_prefix = f"adb -H {device_host} -s {device_id} "
    else:
        adb_prefix =f"adb -H {device_host} "

    ic(adb_prefix)

    push_cmds = [
        f"{adb_prefix} root",
        f"{adb_prefix} remount",
        f"{adb_prefix} shell setenforce 0"
    ]
    for cmd in push_cmds:
        exec_shell_cmd(cmd)


def push_assets_on_target_device_via_adb(device_storage_location: str, 
                                        target_arch:DeviceType=DeviceType.ARM64_ANDROID,
                                        device_id:str = None, 
                                        device_host:str = "localhost",
                                        send_root_access_request:bool = False) -> None:
    if send_root_access_request:
        get_root_access_via_adb(device_id=device_id, device_host=device_host)

    if device_id:
        adb_prefix = f"adb -H {device_host} -s {device_id} "
    else:
        adb_prefix =f"adb -H {device_host} "

    SNPE_TARGET_ARCH=target_arch.value

    logger.debug("Creating dirs on Target Device at :" + device_storage_location)
    mkdir_cmds = [
        f"{adb_prefix} shell \"mkdir -p {device_storage_location}/bin\" ",
        f"{adb_prefix} shell \"mkdir -p {device_storage_location}/lib\" ",
        f"{adb_prefix} shell \"mkdir -p {device_storage_location}/dsp\" ",
    ]
    exec_shell_cmd_in_batch(mkdir_cmds)

    logger.debug(f"Pushing SNPE {SNPE_TARGET_ARCH} Binaries: ")
    # adb -H $HOST push $SNPE_ROOT/bin/$SNPE_TARGET_ARCH/. {device_storage_location}/bin/.
    exec_shell_cmd_in_batch(
        [adb_prefix + f" push {SNPE_ROOT}/bin/{SNPE_TARGET_ARCH}*/* {device_storage_location}/bin/."]
    )

    logger.debug(f"Pushing SNPE {SNPE_TARGET_ARCH} Libraries: ")
    # adb -H $HOST push $SNPE_ROOT/lib/$SNPE_TARGET_ARCH/. {device_storage_location}/lib/.
    exec_shell_cmd_in_batch(
        [adb_prefix + f" push {SNPE_ROOT}/lib/{SNPE_TARGET_ARCH}*/* {device_storage_location}/lib/." ]
    )

    logger.debug(f"Pushing SNPE DSP Libraries: ")
    # adb -H $HOST push $SNPE_ROOT/lib/dsp/. {device_storage_location}/dsp/.
    # adb -H $HOST push $SNPE_ROOT/lib/hexagonV*/unsigned/*kel.so. {device_storage_location}/dsp/.
    status_a = exec_shell_cmd(adb_prefix + f" push {SNPE_ROOT}/lib/dsp/* {device_storage_location}/dsp/.")
    status_b = exec_shell_cmd(adb_prefix + f" push {SNPE_ROOT}/lib/*/*/*kel.so {device_storage_location}/dsp/.")
    if status_a != 0 and status_b != 0 :
        raise RuntimeError(f"Failed to push DSP libs on device")

    logger.debug("Changing File permissions: ")
    # adb -H $HOST shell 'chmod -R 777 /data/local/tmp/pysnpe_bench'
    exec_shell_cmd( adb_prefix + f" shell 'chmod -R 777 {device_storage_location}'" )


def is_file_pushed_via_adb(remote_file_location: str, 
                        file_location: str,
                        adb_prefix: str
                    ) -> bool:

    if exec_shell_cmd(adb_prefix + f" shell ls -lh {remote_file_location}") == 0:
        logger.info(f"File already exists on Device at : {remote_file_location}")
        
        # get file size
        file_size = os.popen(f"{adb_prefix} shell 'du -b {remote_file_location}'").read().split()[0]
        device_file_size = int(file_size)

        # get file size on host
        host_file_size = os.path.getsize(file_location)

        # A. Compare the file sizes
        if device_file_size == host_file_size:
            logger.debug(f"File size is same on both host and device = {device_file_size} ")

            # B. Compare checksum
            # 1. Get device file checksum using adb
            device_checksum_result = subprocess.run(f"adb shell 'md5sum {remote_file_location}'", shell=True, executable='/bin/bash', env=os.environ, stdout=subprocess.PIPE)
            if device_checksum_result.returncode == 0:
                device_checksum = str(device_checksum_result.stdout.decode().strip().split(" ")[0])

                # 2. Get host file checksum using hashlib
                host_checksum = get_md5sum(file_location)

                # 3. Compare checksum
                if host_checksum == device_checksum:
                    logger.debug(f"Files have same Md5sum = {host_checksum}")
                    return True
                else:
                    logger.warning(f"Checksums do not match.\n\
                        Checksum of host file ({file_location}) := {host_checksum} != \
                        Checksum of device file ({remote_file_location}) := {device_checksum}")
                    return False
        else:
            logger.warning(f"Filesize on Target Device: {device_file_size} != filesize on host: {host_file_size}")  
            return False  

    
def push_dlc_on_device_via_adb(device_storage_location: str, 
                        dlc_location: str,
                        device_id:str = None, 
                        device_host:str = "localhost"
                        ) -> str:
    if device_id:
        adb_prefix = f"adb -H {device_host} -s {device_id} "
    else:
        adb_prefix =f"adb -H {device_host} "

    remote_dlc_location = f"{device_storage_location}/{os.path.basename(dlc_location)}"
    
    # check if DLC is already pushed
    if is_file_pushed_via_adb(remote_dlc_location, dlc_location, adb_prefix):
        logger.info(f"DLC already exists on Device at : {remote_dlc_location}")
        return remote_dlc_location

    logger.debug(f"Pushing SNPE DLC: {dlc_location}")
    exec_shell_cmd_in_batch(
        [
            adb_prefix + f" shell 'mkdir -p {device_storage_location}' ",
            adb_prefix + f" push {dlc_location} {device_storage_location}/." 
        ]
    )

    logger.debug("Changing File permissions: ")
    # adb -H $HOST shell 'chmod -R 777 /data/local/tmp/<user>/pysnpe_bench'
    exec_shell_cmd_in_batch(
        [adb_prefix + f" shell 'chmod -R 777 {device_storage_location}'"]
    )

    logger.debug(f"File pushed on Device at : {remote_dlc_location}")
    ic(remote_dlc_location)

    return remote_dlc_location