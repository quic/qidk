"""
Python helper for misc utility like clickfromcamera using adb
"""
# @Author and Maintainer: Pradeep Pant (ppant)


from pysnpe_utils.exec_utils import exec_shell_cmd
import os
import subprocess
from pysnpe_utils.logger_config import logger

def clickFromCamera(device_id):
    # Open Camera with adb
    exec_shell_cmd('adb -s '+device_id+' shell am start -a android.media.action.STILL_IMAGE_CAMERA')
    exec_shell_cmd('adb -s '+device_id+' shell sleep 1')
    # Take snapshot with adb
    exec_shell_cmd('adb -s '+device_id+' shell "input keyevent KEYCODE_CAMERA"')
    exec_shell_cmd('adb -s '+device_id+' shell sleep 1')
    # Print the file name of last modified file
    arg = "ls /sdcard/Pictures -t | head -n1"
    jpg_file = subprocess.check_output(['adb', '-s', device_id, 'shell', arg])
    jpg_file_name = jpg_file.decode('utf-8')
    exec_shell_cmd ('adb -s '+device_id+' shell sleep 1')
    pull_command = "adb -s "+device_id+" pull /sdcard/Pictures/" + jpg_file_name + " ."
    exec_shell_cmd(pull_command)
    exec_shell_cmd(("chmod -R 777 *"))
    return jpg_file_name.strip()
    