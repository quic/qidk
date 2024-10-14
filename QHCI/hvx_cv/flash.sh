#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

adb -s ae25c132 shell mkdir -p /data/local/tmp/qing/qhci_test/
adb -s ae25c132 push android_Release_aarch64/ship/qhci_test /data/local/tmp/qing/qhci_test/
adb -s ae25c132 push android_Release_aarch64/ship/libqhci.so /data/local/tmp/qing/qhci_test/
adb -s ae25c132 push hexagon_Release_toolv86_v68/ship/libqhci_skel.so /data/local/tmp/qing/qhci_test/
adb -s ae25c132 shell chmod 777 /data/local/tmp/qing/qhci_test/qhci_test
adb -s ae25c132 shell "cd /data/local/tmp/qing/qhci_test/ &&
    export LD_LIBRARY_PATH=/data/local/tmp/qing/qhci_test:$LD_LIBRARY_PATH &&
    export ADSP_LIBRARY_PATH=\"/data/local/tmp/qing/qhci_test;/vendor/dsp/cdsp;/vendor/lib/rfsa/adsp;/system/lib/rfsa/adsp;/dsp\" &&
    ./qhci_test all"
