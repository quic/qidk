#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

import numpy as np
import os
import sys

if len(sys.argv) != 4 :
    print("Usage : python qc_verify_accuracy_pred.py <K-samples> <tf | snpe> <logits_dir>")
    sys.exit()

# total_samples = 1820
total_samples = int(sys.argv[1])
print("total_samples = {}".format(total_samples))

mode = sys.argv[2]
print(f"Mode = {mode}")

logits_dir = sys.argv[3]
print(f"logits_dir = {logits_dir}")

dataset_logits_argmax = np.fromfile("dataset_logits_argmax_score/dataset_logits_predicted_labels.raw", np.float32)

match_index = 0

for i in range(total_samples):
    if "tf" in mode:
        pred_file_nm = f"{logits_dir}/infer_{i}___Identity.raw"
    else:
        pred_file_nm = "{}/Result_{}/Identity:0.raw".format(logits_dir, i)
    
    pred_score = np.argmax(np.fromfile(pred_file_nm, np.float32))
    print(f"pred = {pred_score} ; ans = {dataset_logits_argmax[i]}")
    if pred_score == dataset_logits_argmax[i]:
        match_index = match_index + 1

print("\n\nAccuracy = {} %".format((match_index*100)/total_samples))
