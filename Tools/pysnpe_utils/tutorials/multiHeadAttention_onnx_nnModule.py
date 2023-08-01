import torch
import torch.nn as nn

class MultiheadAttentionModel(nn.Module):
    def __init__(self, input_size, hidden_size, num_heads):
        super(MultiheadAttentionModel, self).__init__()
        self.multihead_attn = nn.MultiheadAttention(
            embed_dim=input_size,
            kdim=input_size,
            vdim=input_size,
            num_heads=num_heads)
        self.linear = nn.Linear(input_size, hidden_size)
    
    def forward(self, inputs):
        # Q, K, V
        attn_output, _ = self.multihead_attn(inputs, inputs, inputs)
        linear_output = self.linear(attn_output)
        return linear_output

batch_size = 8
seq_len = 10
input_size = 16
inputs = torch.randn(seq_len, batch_size, input_size)

# Create an instance of the model
model = MultiheadAttentionModel(input_size=input_size, hidden_size=32, num_heads=4)

# Infer
pytorch_out = model(inputs)
# output size := torch.Size([10, 8, 32])

# ====================================================================== #
#                        Test OnDevie with PySNPE
# ====================================================================== #
from pysnpe_utils import pysnpe
from pysnpe_utils.pysnpe_enums import *

target_device = pysnpe.TargetDevice(device_host="10.206.64.253", target_device_adb_id="728b7a92")

# Convert Torch model to ONNX => DLC => Generate Cache (offline preparation)
snpe_context = pysnpe.export_to_onnx(model, [InputMap('inp_ids', (seq_len, batch_size, input_size), torch.float32) ],
                                        ["attn_out"], "multi_head_attn.onnx")\
                                            .to_dlc()\
                                                .gen_dsp_graph_cache(DlcType.FLOAT)\
                                                    .set_target_device(target_device=target_device)\
                                                        .profile(runtime=Runtime.DSP)
                                                        
input_tensor_map = {'inp_ids' : inputs.cpu().numpy()}
transpose_order = {'img' : (0,2,1)} # 10,8,16 => 10,16,8

# infer on device : DSP FP16 runtime
snpe_out = snpe_context.execute_dlc(input_tensor_map, transpose_input_order=transpose_order,
                                    target_acclerator=Runtime.DSP )
snpe_prediction = torch.from_numpy(snpe_out["attn_out"])

# check SNPE output vs Pytorch output
output_diff = torch.nn.functional.mse_loss(pytorch_out, snpe_prediction)
print(f"output diff = {output_diff}")

# # visualize dlc
# snpe_context.visualize_dlc()