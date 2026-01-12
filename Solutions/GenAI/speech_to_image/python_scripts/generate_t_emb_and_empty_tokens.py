#============================================================================
# Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================


from diffusers import DPMSolverMultistepScheduler
from diffusers.models.embeddings import get_timestep_embedding
from diffusers import UNet2DConditionModel
from transformers import CLIPTokenizerFast
import numpy as np
import torch

user_steps = 20

cache = "./_data_/cache/huggingface/diffusers"
time_embeddings = UNet2DConditionModel.from_pretrained('runwayml/stable-diffusion-v1-5', subfolder='unet', cache_dir=cache).time_embedding

scheduler = DPMSolverMultistepScheduler()
scheduler.set_timesteps(user_steps)

def run_tokenizer(prompt):
    tokens = tokenizer.encode(prompt, truncation=True, padding=True, max_length=tokenizer_max_length, pad_to_multiple_of=tokenizer_max_length)
    token_ids = np.array(tokens, dtype=np.float32)
    return token_ids

tokenizer_max_length = 77
tokenizer = CLIPTokenizerFast.from_pretrained('openai/clip-vit-base-patch32', cache_dir=cache)
tokenizer.model_max_length = tokenizer_max_length
tokens = tokenizer.encode("", truncation=True, padding=True, max_length=tokenizer_max_length, pad_to_multiple_of=tokenizer_max_length)
uncond_tokens = np.array(tokens, dtype=np.float32)
uncond_tokens.tofile("tokens.raw")

def get_time_embedding(step):
    timestep = np.int32(scheduler.timesteps.numpy()[step])
    timestep = torch.tensor([timestep])
    t_emb = get_timestep_embedding(timestep, 320, True, 0)
    emb = time_embeddings(t_emb).detach().numpy()
    return emb
 
for step in range(user_steps):
    time_embed = get_time_embedding(step)
    time_embed.tofile('t_emb_' + str(step) + '.raw')