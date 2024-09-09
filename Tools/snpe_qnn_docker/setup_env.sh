#!/bin/bash

#-----------------------------------------------------------------------------
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. All rights reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc. and/or its subsidiaries.
#
# All data and information contained in or disclosed by this document are
# confidential and proprietary information of Qualcomm Technologies, Inc., and
# all rights therein are expressly reserved. By accepting this material, the
# recipient agrees that this material and the information contained therein
# are held in confidence and in trust and will not be used, copied, reproduced
# in whole or in part, nor its contents revealed in any manner to others
# without the express written permission of Qualcomm Technologies, Inc.
#
# This software may be subject to U.S. and international export, re-export, or
# transfer ("export") laws.  Diversion contrary to U.S. and international law
# is strictly prohibited.
#-----------------------------------------------------------------------------

function downloadndk() {
	wget https://dl.google.com/android/repository/android-ndk-r26c-linux.zip -O /usr/android-ndk-r26c-linux.zip
	unzip /usr/android-ndk-r26c-linux.zip -d /usr
	export ANDROID_NDK_ROOT=/usr/android-ndk-r26c
	export PATH=${ANDROID_NDK_ROOT}:${PATH}
	${QNN_SDK_ROOT}/bin/envcheck -n
}

function predownloadedndk() {
	read -p "Do you have Android NDK (android-ndk-r26c-linux.zip) already downloaded? Select [Y]es or [N]o " downloaded_ndk
	case $downloaded_ndk in
		[Yy]* ) 
			read -p "Please provide the andorid ndk path: " ndk_path
			export ANDROID_NDK_ROOT=${ndk_path}
			export PATH=${ANDROID_NDK_ROOT}:${PATH}
			${QNN_SDK_ROOT}/bin/envcheck -n
			;;
		
		[Nn]* ) 
			echo "Skipping the setup of Android NDK";;
        * ) 
			echo "Please answer yes or no.";;
	esac
}

function setupenv() {
	. /usr/venv/bin/activate
	${QNN_SDK_ROOT}/bin/check-python-dependency
	pip3 install onnxruntime==1.16.1
	. ${QNN_SDK_ROOT}/bin/check-linux-dependency.sh
	. ${QNN_SDK_ROOT}/bin/envsetup.sh
	${QNN_SDK_ROOT}/bin/envcheck -c
	export PATH=/usr/venv/lib/python3.10/site-packages/tensorflow:${PATH}
	export PATH=/usr/venv/lib/python3.10/site-packages/onnx:${PATH}
	export TENSORFLOW_HOME=/usr/venv/lib/python3.10/site-packages/tensorflow
	if [[ -z "${ANDROID_NDK_ROOT}" ]]; then
		read -p "Do you wish to download Android NDK (android-ndk-r26c-linux.zip)? Select [Y]es or [N]o " ndk
		case $ndk in
			[Yy]* ) 
				downloadndk ;;
			[Nn]* ) 
				predownloadedndk;;
			* ) 
				echo "Please answer yes or no.";;
		esac
	else
		echo "Android NDK path is already set to $ANDROID_NDK_ROOT"
	fi
}

function setupoptionalenv() {
	if [[ -z "${HEXAGON_SDK_ROOT}" ]]; then
		read -p "(optional) Do you wish to set up Hexagon toolchain? Select [Y]es or [N]o " hex
		case $hex in
			[Yy]* ) 
				read -p "Please provide the Hexagon toolchain path: " hex_path
				export HEXAGON_SDK_ROOT=${hex_path} ;;
			[Nn]* ) 
				echo "Skipping setting the hexagon SDK path, This is optional setup";;
			* ) 
				echo "Please answer yes or no.";;
		esac
	fi
		
	if [[ -z "${QNX_HOST}" ]]; then
		read -p "(optional) Do you wish to set up QNX toolchain? Select [Y]es or [N]o " qnx
		case $qnx in
			[Yy]* ) 
				read -p "Please provide the QNX toolchain path: " qnx_path
				export QNX_HOST=${qnx_path}/root/host
				export QNX_TARGET=${qnx_path}/root/target
				export PATH=${QNX_HOST}/usr/bin:${PATH} ;;
			[Nn]* ) 
				echo "Skipping setting the QNX toolchian path. This is optional setup";;
			* ) 
				echo "Please answer yes or no.";;
		esac
	fi
	if [[ -z "${QNN_AARCH64_LINUX_OE_GCC_93}" ]]; then
		read -p "(optional) Do you wish to set up OE-Linux toolchain? Select [Y]es or [N]o " oe
		case $oe in
			[Yy]* ) 
				read -p "Please provide the OE-Linux toolchain path: " oe_path
				export QNN_AARCH64_LINUX_OE_GCC_93=${oe_path} ;;
			[Nn]* ) 
				echo "Skipping stetting the OE-Linux toolchain path. This is optional setup";;
			* ) 
				echo "Please answer yes or no.";;
		esac
	fi
}


if [[ -z "${QNN_SDK_ROOT}" ]]; then 
	
	read -p "Enter the path of qnn sdk:" sdk_path #check if it is unziped or zipped

	if [[ ! -d "$sdk_path" ]] && [[ ! -e "$sdk_path" ]]; then
		echo "The entered path for QNN SDK does not exist"
		read -p "Enter the path of qnn sdk:" sdk_path
	fi
	
	if (file ${sdk_path} | grep -o "Zip archive data" ) ; then
		echo "unzipping the file"
		unzip ${sdk_path} -d /usr/qnn_sdk
		export QNN_SDK_ROOT=$(ls -d /usr/qnn_sdk/*)
	else
		export QNN_SDK_ROOT=${sdk_path}
		# export SNPE_ROOT=${sdk_path}  #not_required sdk scripts are doing it already
	fi
	
	setupenv
	# setupoptionalenv
	
	echo "SDK path is set to $QNN_SDK_ROOT"
	sed -i '/[. /setup_env.sh]/d' ~/.bashrc
	echo "export QNN_SDK_ROOT=${QNN_SDK_ROOT}" >> ~/.bashrc
	echo "export ANDROID_NDK_ROOT=${ANDROID_NDK_ROOT}" >> ~/.bashrc
	if [[ "${HEXAGON_SDK_ROOT}" ]]; then
		echo "export HEXAGON_SDK_ROOT=${HEXAGON_SDK_ROOT}" >> ~/.bashrc
	fi
	# if [[ "${QNX_HOST}" ]]; then
	# 	echo "export QNX_HOST=${QNX_HOST}" >> ~/.bashrc
	# 	echo "export QNX_TARGET=${QNX_TARGET}" >> ~/.bashrc
	# fi
	# if [[ "${QNN_AARCH64_LINUX_OE_GCC_93}" ]]; then
	# 	echo "export QNN_AARCH64_LINUX_OE_GCC_93=${QNN_AARCH64_LINUX_OE_GCC_93}" >> ~/.bashrc
	# fi
	echo ". /setup_env.sh" >> ~/.bashrc
else 
	echo "SDK path is already set to $QNN_SDK_ROOT"
	setupenv
fi

