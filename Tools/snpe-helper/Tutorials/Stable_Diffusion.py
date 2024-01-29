# -*- mode: python -*-
# =============================================================================
#  @@-COPYRIGHT-START-@@
#
#  Copyright (c) 2023 of Qualcomm Innovation Center, Inc. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause
#
#  @@-COPYRIGHT-END-@@
# =============================================================================

# Refer to https://docs.qualcomm.com/bundle/publicresource/topics/80-64748-1/model_updates.html for model generation
import numpy as np
import cv2
from tqdm import tqdm
from tokenizers import Tokenizer
from torch import tensor,from_numpy,randn,manual_seed
from diffusers import DPMSolverMultistepScheduler
from diffusers import UNet2DConditionModel
from diffusers.models.embeddings import get_timestep_embedding
from snpehelper_manager import PerfProfile,Runtime,timer,SnpeContext
import time

class TextEncoder(SnpeContext):
        def __init__(self,dlc_path: str = "None",
                    input_layers : list = [],
                    output_layers : list = [],
                    output_tensors : list = [],
                    runtime : str = Runtime.CPU,
                    profile_level : str = PerfProfile.BALANCED,
                    enable_cache : bool = False):
            super().__init__(dlc_path,input_layers,output_layers,output_tensors,runtime,profile_level,enable_cache)

        #@timer
        def run_text_encoder(self,input_data):
            self.SetInputBuffer(input_data.flatten(),"input_1")

            if(model_object_text.Execute() != True):
                    print("!"*50,"Failed to Execute","!"*50)
                    return
            output_data = self.GetOutputBuffer("output_1")
            # Output of Text encoder should be of shape (1, 77, 768)
            output_data = output_data.reshape((1, 77, 768))
            return output_data

class Unet(SnpeContext):
        def __init__(self,dlc_path: str = "None",
                    input_layers : list = [],
                    output_layers : list = [],
                    output_tensors : list = [],
                    runtime : str = Runtime.CPU,
                    profile_level : str = PerfProfile.BALANCED,
                    enable_cache : bool = False):
            super().__init__(dlc_path,input_layers,output_layers,output_tensors,runtime,profile_level,enable_cache)

        #@timer
        def run_unet(self,input_data_1, input_data_2, input_data_3):
            self.SetInputBuffer(input_data_1.flatten(),"input_1")
            self.SetInputBuffer(input_data_2.flatten(),"input_2")
            self.SetInputBuffer(input_data_3.flatten(),"input_3")

            if(self.Execute() != True):
                    print("!"*50,"Failed to Execute","!"*50)
                    return
            output_data = self.GetOutputBuffer("output_1")
            # Output of UNet should be of shape (1, 64, 64, 4)
            output_data = output_data.reshape((1, 64, 64, 4))
            return output_data

class VaeEncoder(SnpeContext):
        def __init__(self,dlc_path: str = "None",
                    input_layers : list = [],
                    output_layers : list = [],
                    output_tensors : list = [],
                    runtime : str = Runtime.CPU,
                    profile_level : str = PerfProfile.BALANCED,
                    enable_cache : bool = False):
            super().__init__(dlc_path,input_layers,output_layers,output_tensors,runtime,profile_level,enable_cache)

        #@timer
        def run_vae(self,input_data):
            self.SetInputBuffer(input_data.flatten(),"input_1")

            if(self.Execute() != True):
                    print("!"*50,"Failed to Execute","!"*50)
                    return
            output_data = self.GetOutputBuffer("output_1")
            # Convert floating point output into 8 bits RGB image
            output_data = np.clip(output_data*255.0, 0.0, 255.0).astype(np.uint8)
            # Output of VAE should be of shape (512, 512, 3)
            output_data = output_data.reshape((512, 512, 3))
            return output_data


def get_time_embedding(timestep):
    timestep = tensor([timestep])
    t_emb = get_timestep_embedding(timestep, 320, True, 0)
    emb = time_embeddings(t_emb).detach().numpy()
    
    return emb 

def run_tokenizer(prompt):
    # Run Tokenizer encoding
    token_ids = tokenizer.encode(prompt).ids
    # Convert tokens list to np.array
    token_ids = np.array(token_ids, dtype=np.float32)

    return token_ids

# Setting up user provided time steps for Scheduler

def run_scheduler(noise_pred_uncond, noise_pred_text, latent_in, timestep):
    # Convert all inputs from NHWC to NCHW
    noise_pred_uncond = np.transpose(noise_pred_uncond, (0,3,1,2)).copy()
    noise_pred_text = np.transpose(noise_pred_text, (0,3,1,2)).copy()
    latent_in = np.transpose(latent_in, (0,3,1,2)).copy()

    # Convert all inputs to torch tensors
    noise_pred_uncond = from_numpy(noise_pred_uncond)
    noise_pred_text = from_numpy(noise_pred_text)
    latent_in = from_numpy(latent_in)

    # Merge noise_pred_uncond and noise_pred_text based on user_text_guidance
    noise_pred = noise_pred_uncond + user_text_guidance * (noise_pred_text - noise_pred_uncond)

    # Run Scheduler step
    latent_out = scheduler.step(noise_pred, timestep, latent_in).prev_sample.numpy()
    
    # Convert latent_out from NCHW to NHWC
    latent_out = np.transpose(latent_out, (0,2,3,1)).copy()
    
    return latent_out

# Function to get timesteps
def get_timestep(step):
    return np.int32(scheduler.timesteps.numpy()[step])

# Any user defined prompt
user_prompt = "decorated modern country house interior, 8 k, light reflections"

# User defined seed value
user_seed = np.int64(1.36477711e+14)

# User defined step value, any integer value in {20, 50}
user_step = 20

# User define text guidance, any float value in [5.0, 15.0]
user_text_guidance = 7.5

# Error checking for user_seed
assert isinstance(user_seed, np.int64) == True,"user_seed should be of type int64"

# Error checking for user_step
assert isinstance(user_step, int) == True,"user_step should be of type int"
assert user_step == 20 or user_step == 50,"user_step should be either 20 or 50"

# Error checking for user_text_guidance
assert isinstance(user_text_guidance, float) == True,"user_text_guidance should be of type float"
assert user_text_guidance >= 5.0 and user_text_guidance <= 15.0,"user_text_guidance should be a float from [5.0, 15.0]"


# pre-load time embedding
time_embeddings = UNet2DConditionModel.from_pretrained('runwayml/stable-diffusion-v1-5',
                                                       subfolder='unet', cache_dir='./cache/diffusers').time_embedding
# Define Tokenizer output max length (must be 77)
tokenizer_max_length = 77

# Initializing the Tokenizer
tokenizer = Tokenizer.from_pretrained("openai/clip-vit-base-patch32")

# Setting max length to tokenizer_max_length
tokenizer.enable_truncation(tokenizer_max_length)
tokenizer.enable_padding(pad_id=49407, length=tokenizer_max_length)

# Initializing the Scheduler
scheduler = DPMSolverMultistepScheduler(num_train_timesteps=1000, beta_start=0.00085,
                                        beta_end=0.012, beta_schedule="scaled_linear")


'''
    Creating model instances
'''

# Instance for TextEncoder 
model_object_text = TextEncoder("text_encoder_quantized.dlc",["input_1"],["layernorm_24"],["output_1"],Runtime.DSP,PerfProfile.BURST,False)
# Intialize required buffers and load network
ret = model_object_text.Initialize()
if(ret != True):
    print("!"*50,"Failed to Initialize","!"*50)
    exit(0)


# Instance for Unet 
model_object_unet = Unet("unet_quantized.dlc",["input_1","input_2","input_3"], ["conv_out"],["output_1"],Runtime.DSP,PerfProfile.BURST,False)
# Intialize required buffers and load network
ret = model_object_unet.Initialize()
if(ret != True):
    print("!"*50,"Failed to Initialize","!"*50)
    exit(0)

# Instance for VaeEncoder 
model_object_vae = VaeEncoder("vae_decoder_quantized.dlc",["input_1"], ["Clip_800"],["output_1"],Runtime.DSP,PerfProfile.BURST,False)
# Intialize required buffers and load network
ret = model_object_vae.Initialize()
if(ret != True):
    print("!"*50,"Failed to Initialize","!"*50)
    exit(0)

uncond_tokens = run_tokenizer("")
uncond_text_embedding = model_object_text.run_text_encoder(uncond_tokens)

# Run Tokenizer
scheduler.set_timesteps(user_step)
cond_tokens = run_tokenizer(user_prompt)

# Run Text Encoder on Tokens 
user_text_embedding = model_object_text.run_text_encoder(cond_tokens)

# Initialize the latent input with random initial latent
random_init_latent = randn((1, 4, 64, 64), generator=manual_seed(user_seed)).numpy()
latent_in = random_init_latent.transpose((0, 2, 3, 1)).copy()

# Run the loop for user_step times
for step in tqdm(range(user_step)):
    
    # Get timestep from step
    timestep = get_timestep(step)

    # Run U-net for const embeddings
    unconditional_noise_pred = model_object_unet.run_unet(latent_in, get_time_embedding(timestep), uncond_text_embedding)

    # Run U-net for user text embeddings
    conditional_noise_pred = model_object_unet.run_unet(latent_in, get_time_embedding(timestep), user_text_embedding)
    
    # Run Scheduler
    latent_in = run_scheduler(unconditional_noise_pred, conditional_noise_pred, latent_in, timestep)

# Run VAE
output_image = model_object_vae.run_vae(latent_in)
output_image = cv2.cvtColor(output_image,cv2.COLOR_RGB2BGR)
cv2.imwrite("stabe_diff.jpg",output_image)