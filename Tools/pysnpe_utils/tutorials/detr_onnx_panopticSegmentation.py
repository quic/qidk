#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

# Reference:
# https://colab.research.google.com/github/facebookresearch/detr/blob/colab/notebooks/DETR_panoptic.ipynb#scrollTo=-5ytUV_qsVhL

from PIL import Image
import requests
import math
import matplotlib.pyplot as plt
from icecream import ic

import torch
import torchvision.transforms as T
import numpy as np
torch.set_grad_enabled(False);

model = torch.hub.load('facebookresearch/detr', 'detr_resnet101_panoptic', pretrained=True, num_classes=250)
model.eval();

url = "http://images.cocodataset.org/val2017/000000281759.jpg"
im = Image.open(requests.get(url, stream=True).raw)

# mean-std normalize the input image (batch-size: 1)
transform = T.Compose([
    T.Resize(480),
    T.ToTensor(),
    T.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])
])
img = transform(im).unsqueeze(0)
ic(img.shape)

# Run Inference
out = model(img)
ic(out["pred_boxes"].shape)
ic(out["pred_logits"].shape)
ic(out["pred_masks"].shape)
# compute the scores, excluding the "no-object" class (the last one)
scores = out["pred_logits"].softmax(-1)[..., :-1].max(-1)[0]
ic(scores.shape)
# threshold the confidence
keep = scores > 0.85
ic(keep.shape)

# Plot all the remaining masks
ncols = 5
fig, axs = plt.subplots(ncols=ncols, nrows=math.ceil(keep.sum().item() / ncols), figsize=(18, 10))
for i, mask in enumerate(out["pred_masks"][keep]):
    axs[i // ncols, i % ncols].imshow(mask, cmap="cividis")
    axs[i // ncols, i % ncols].axis('off')
for ax in axs.flat[i+1:]: ax.axis('off')

plt.savefig('panoptic_segment_masks.png', bbox_inches='tight')


from pysnpe_utils import pysnpe
from pysnpe_utils.pysnpe_enums import *

# Convert Torch model to ONNX => DLC => Generate Cache (offline preparation)
pysnpe.export_to_onnx(model, [InputMap('input_img', img.shape, torch.float32) ],
                                        ["pred_logits"], "detr_v2_panoptic_seg.onnx", opset_version=12)
