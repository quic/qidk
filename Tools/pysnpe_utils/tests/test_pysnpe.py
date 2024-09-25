#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

import os
import pytest
from icecream import ic
import torch
import tensorflow as tf
tf.compat.v1.logging.set_verbosity(tf.compat.v1.logging.ERROR)
import numpy as np
from transformers import AutoModelForSequenceClassification, TFAutoModelForSequenceClassification

from pysnpe_utils import pysnpe


class TinyBertSentimentAnalysis:
    def __init__(self, model_framework):
        if model_framework=="torch":
            self.pytorch_model = AutoModelForSequenceClassification.from_pretrained("philschmid/tiny-bert-sst2-distilled")
            self.pytorch_model = self.pytorch_model.eval()
        elif model_framework=="tf":
            self.tf_model = TFAutoModelForSequenceClassification.from_pretrained("philschmid/tiny-bert-sst2-distilled", from_pt=True)
            self.pytorch_model = None
        else:
            raise ValueError(f"Unexpected model framework received = {model_framework}")

    def forward(self, input_ids, attention_mask):
        outputs = self.pytorch_model(input_ids=input_ids, attention_mask=attention_mask)
        logits = outputs.logits
        argmax = torch.argmax(logits, dim=1)
        return logits, argmax

    @tf.function
    def call(self, inputs):
        input_ids = inputs['input_ids']
        attention_mask = inputs['attention_mask']
        outputs = self.tf_model(input_ids=input_ids, attention_mask=attention_mask)
        logits = outputs['logits']
        argmax = tf.argmax(logits, axis=1, output_type=tf.dtypes.int32)
        return logits, argmax

    def predict(self, input_ids, attention_mask):
        logits, argmax = self.forward(input_ids, attention_mask) if isinstance(self.pytorch_model, torch.nn.Module) else self.call({'input_ids':input_ids, 'attention_mask':attention_mask})
        return logits, argmax


@pytest.fixture(scope="module")
def create_pytorch_model():
    # Create test assets
    os.makedirs("test_assets", exist_ok=True)

    model = TinyBertSentimentAnalysis("torch")

    def predict(input_ids, attention_mask):
        with torch.no_grad():
            return model.predict(input_ids, attention_mask)

    # Return the predict function to the test function
    yield predict

    # Clean up model and assets after the test is done
    del model
    if os.path.isdir("test_assets"): os.removedirs("test_assets")


@pytest.fixture(scope="module")
def create_tf_model():
    # Create test assets
    os.makedirs("test_assets", exist_ok=True)

    model = TinyBertSentimentAnalysis("tf")

    def predict(input_ids, attention_mask):
            return model.predict(input_ids, attention_mask)

    # Return the predict function to the test function
    yield predict

    # Clean up model and assets after the test is done
    del model
    if os.path.isdir("test_assets"): os.removedirs("test_assets")


def test_pytorch_model(create_pytorch_model):
    input_ids = torch.tensor([[  101, 11082,  2175,  2046,  1996, 15393,   102]])
    attention_mask = torch.tensor([[1, 1, 1, 1, 1, 1, 1]])
    expected_logits = torch.tensor([[-1.3814,  1.6003]])

    logits, argmax = create_pytorch_model(input_ids, attention_mask)
    assert logits == pytest.approx(expected_logits, abs=1e-2, rel=1e-2)
    assert argmax == 1


def test_tf_model(create_tf_model):
    input_ids = np.array([[  101, 11082,  2175,  2046,  1996, 15393,   102]])
    attention_mask = np.array([[1, 1, 1, 1, 1, 1, 1]])
    expected_logits = np.array([[-1.3814,  1.6003]])

    logits, argmax = create_tf_model(input_ids, attention_mask)
    assert logits.numpy() == pytest.approx(expected_logits, abs=1e-2, rel=1e-2)
    assert argmax.numpy() == 1