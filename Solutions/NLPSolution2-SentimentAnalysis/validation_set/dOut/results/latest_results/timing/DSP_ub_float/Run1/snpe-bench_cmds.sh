#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

#!/system/bin/sh
export LD_LIBRARY_PATH=/data/local/tmp/8550snpeSA/artifacts/aarch64-android-clang8.0/lib:$LD_LIBRARY_PATH
export ADSP_LIBRARY_PATH="/data/local/tmp/8550snpeSA/artifacts/aarch64-android-clang8.0/lib;/system/lib/rfsa/adsp;/usr/lib/rfsa/adsp;/system/vendor/lib/rfsa/adsp;/dsp;/etc/images/dsp;"
cd /data/local/tmp/8550snpeSA/dOut
rm -rf output
/data/local/tmp/8550snpeSA/artifacts/aarch64-android-clang8.0/bin/snpe-net-run --container mobilebert_sst2_cached.dlc --input_list snpe_raw_list.txt --output_dir output --use_dsp --userbuffer_float --perf_profile burst --profiling_level basic
