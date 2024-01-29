# snpehelper for LE
## Table of Contents

- [Table of Contents](#table-of-contents)
- [LE Build setup](#le-build-setup)
- [Generating snpe-helper library](#generating-snpe-helper-library)
- [Running sample application](#running-sample-application)

## LE Build setup

1. Follow "00067.1 Release Note for QCS8550.LE.1.0" to Setup "qti-distro-rb-debug" LE.1.0 build server for QCS8550
2. Make sure "bitbake qti-robotics-image" is successful 
3. Verify the "qti-distro-rb-debug" build by flashing on target using "fastboot". Commands to flash:

    ```
    cd build-qti-distro-rb-debug/tmp-glibc/deploy/images/kalama/qti-robotics-image/
    adb root
    adb reboot bootloader
        
    fastboot flash abl_a abl.elf
    fastboot flash abl_b abl.elf
    fastboot flash dtbo_a dtbo.img
    fastboot flash dtbo_b dtbo.img
    fastboot flash boot_a boot.img
    fastboot flash boot_b boot.img
    fastboot flash system_a system.img
    fastboot flash system_b system.img
    fastboot flash userdata userdata.img
    fastboot flash persist persist.img

    fastboot reboot
    ```

## Additional packages

### PIP
1. Make below change in <APPS_ROOT>/LE.PRODUCT.2.1.r1/apps_proc/poky/meta-qti-bsp/recipes-products/images/qti-robotics-image.bb
    ```
    CORE_IMAGE_EXTRA_INSTALL += "python3-pip" 
    ```

2.  Run the following commands to build with "pip" package and flash after build is successful.
    ```
    cd <APPS_ROOT>/LE.PRODUCT.2.1.r1/apps_proc/poky
    export MACHINE=kalama DISTRO=qti-distro-rb-debug
    source qti-conf/set_bb_env.sh
    export PREBUILT_SRC_DIR="<APPS_ROOT>/prebuilt_HY11"
    bitbake qti-robotics-image
    ```
### Setup aarch64-oe-linux-gcc Toolchain
1. To generate SDK
    ```
    bitbake qti-robotics-image -c populate_sdk
    ```
2. Extract sdk
    ```
    cd  build-qti-distro-rb-debug/tmp-glibc/deploy/sdk/
    mkdir test
    cd test 
    umask 022 
    ./../rb-debug-x86_64-qti-robotics-image-aarch64-kalama-toolchain-2357871.sh <Enter target directory for SDK : input the absolute test work path>
    ```
3. Setup environment
    ```
    . environment-setup-aarch64-oe-linux
    ```
    #### Check CC
    If above steps are successful
    ```
    which $CC   # prints CC path for aarch64-oe-linux-gcc like "<Absolute-test-path>/sysroots/x86_64-qtisdk-linux/usr/bin/aarch64-oe-linux/aarch64-oe-linux-gcc"
    ```

## Generating snpe-helper library
1. Clone snpe-helper repo from qidk
    ```
    git clone https://github.com/quic/qidk.git
    ```
2. Copy CMakeLists_LE1.0.txt and apply patch
    ```
    cd qidk/Tools/snpe-helper
    git apply LE_changes.patch
    ```
3. Download lastest Qualcomm® Neural Processing SDK from https://qpm.qualcomm.com/main/tools/details/qualcomm_neural_processing_sdk
4. Setup python 3.10 environment
5. Copy CMakeLists_LE1.0.txt to CMakeLists.txt
    ```
    cp CMakeLists_LE1.0.txt CMakeLists.txt
    ```
6. Update "SNPE_ROOT" and "PYTHON_BASE_DIR" in CMakeLists.txt
    ```
    set (SNPE_ROOT "<SNPE SDK Path>")
    set (PYTHON_BASE_DIR "<Python 3.10 env path>")
    ```
7. compile snpe-helper 
    ```
    mkdir build
    cd build
    cmake ..
    make
    ```
8. Check if libsnpehelper.so is built under the <build> directory

## Running sample application
1. Execute the following commands to remount the target
    ```
    adb root
    adb disable-verity
    adb reboot
    adb root
    adb shell "mount -o remount,rw /"
    ```
2. Push " libsnpehelper.so", "SNPE SDK", dlc and input image onto the device
    ```
    adb push <file> <path_on_target>
    ```
3. Execute the following commands to setup snpe on target
    ```
    adb shell
    cd <path_on_target>/<SNPE_ROOT>
    cp -r lib/aarch64-oe-linux-gcc11.2/lib* /usr/lib/
    cp bin/aarch64-oe-linux-gcc11.2/snpe-net-run /usr/bin/
    cp -r lib/hexagon-v73/unsigned/lib* /usr/lib/rfsa/adsp/
    chmod +x /usr/bin/snpe-net-run
    snpe-net-run --version
    ```
    Expected output: SNPE v2.17.0.231124161510_65373

4. Install required python packeges
    ```
    export PIP_TARGET=/etc/pip
    export PYTHONPATH=$PYTHONPATH:/etc/pip
    unset PIP_TARGET
    python -m pip install --trusted-host pypi.python.org --trusted-host pypi.org --trusted-host files.pythonhosted.org --upgrade pip
    python -m pip install --trusted-host pypi.python.org --trusted-host pypi.org --trusted-host files.pythonhosted.org torch torchvision numpy opencv-python-headless tqdm pillow matplotlib tokenizers diffusers==0.22.3 accelerate
    ``` 
    ``` 
    date --set="2024-01-01 10:38:00" # set current date and time if there is any issue with date
    ``` 
5. Run sample application 
    ```
    adb shell
    cd <path_on_target>
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:<path_on_target>
    python Detr_Object_Detection.py
    ```