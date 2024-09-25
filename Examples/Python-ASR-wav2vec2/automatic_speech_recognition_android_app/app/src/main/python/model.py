#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

import os
import torch
import numpy as np
os.environ['CURL_CA_BUNDLE'] =''
from transformers import AutoProcessor

processor = AutoProcessor.from_pretrained("facebook/wav2vec2-base-960h")

def main(inp=None):
    if not inp:
         return ""

    audio=list(inp)
    audio_tensor=torch.tensor(audio).float()
    inputs =processor(audio_tensor, sampling_rate=16000, return_tensors="np")
    print("tfllite Inputs",inputs)
    print("ASRHelper: Updated Tokenized Inputs",inputs['input_values'])
    input_values =inputs.input_values.astype(np.float32)
    print("ASRHelper: Updated float32 Tokenized Inputs",input_values)
    return input_values[0]


def postProcess(logits=None):
    if not logits:
        return ""
    logits=torch.from_numpy(logits.reshape((1,124,32)))
    predicted_ids = torch.argmax(logits, dim=-1)
    transcription=processor.batch_decode(predicted_ids)
    return transcription[0] if transcription[0]!="" else ""

