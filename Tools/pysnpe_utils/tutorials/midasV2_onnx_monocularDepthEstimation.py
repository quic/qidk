# !pip install timm
import cv2
import torch
import urllib.request
from icecream import ic
import matplotlib.pyplot as plt

# Download a sample image for inference
url, filename = ("https://github.com/pytorch/hub/raw/master/images/dog.jpg", "dog.jpg")
urllib.request.urlretrieve(url, filename)

model_type = "MiDaS_small" 

# Download/Load MiDas model from Torch Hub
midas = torch.hub.load("intel-isl/MiDaS", model_type)
device = torch.device("cpu") #if torch.cuda.is_available() else torch.device("cpu")
midas.to(device)
midas.eval()

# Download/Load MiDas Transforms (for pre-processing) from Torch Hub
midas_transforms = torch.hub.load("intel-isl/MiDaS", "transforms")
transform = midas_transforms.small_transform

# Load input img for inference
img = cv2.imread(filename)
img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

# Preprocess input img
preprocessed_img = transform(img).to(device)

with torch.no_grad():
    print("Running inference on input with shape : ", str(preprocessed_img.shape))
    torch_prediction = midas(preprocessed_img)
    print(f"Prediction has shape = {torch_prediction.shape}")

    # Post Process output
    torch_prediction = torch.nn.functional.interpolate(
        torch_prediction.unsqueeze(1),
        size=img.shape[:2],
        mode="bicubic",
        align_corners=False,
    ).squeeze()

output = torch_prediction.cpu().numpy()
# Save output as img
plt.imsave('pytorch_dog_depth_prediction.jpg', output)

# ====================== OnDevice Inference with ===========================
print("="*80 + "\nModel Conversion and Execution via ONNX path\n" + "="*80)
# ========================= PySnpe Interface ===============================
from pysnpe_utils import pysnpe
from pysnpe_utils.pysnpe_enums import *

# Select target device
target_device = pysnpe.TargetDevice(device_host="localhost", 
                                    target_device_adb_id="1c7dec76")

# Convert Torch model to ONNX => DLC => Generate Cache (offline preparation)
snpe_context = pysnpe.export_to_onnx(midas, [ InputMap('img', (1, 3, 192, 256), torch.float32) ],
                                        ["out_depth"], "midas_v2.onnx")\
                                            .to_dlc()\
                                                .gen_dsp_graph_cache(DlcType.FLOAT)\
                                                    .set_target_device(target_device=target_device)\
                                                        .profile(runtime=Runtime.DSP)\
                                                            .visualize_dlc()

## Use following if ONNX model is available or DLC is already generated
# input_map = {'img': (1, 3, 192, 256)}
# snpe_context = pysnpe.SnpeContext("midas_v2-opt.onnx", ModelFramework.ONNX, "midas_v2-opt_dsp_fp16_cached.dlc",
#                                        input_map, ["out_depth"], target_device=target_device )\
#                                            .profile(runtime=Runtime.DSP)


# Prepare inputs as Numpy Nd-Array for inference
input_tensor_map = {'img' : preprocessed_img.cpu().numpy()}
transpose_order = {'img' : (0,2,3,1)}

# Run inference
snpe_out = snpe_context.execute_dlc(input_tensor_map, transpose_input_order=transpose_order,
                                    target_acclerator=Runtime.DSP )

# Check if received output has proper shape
ic(snpe_out['out_depth'].shape)

# Convert to Torch tensor format
snpe_prediction = torch.from_numpy(snpe_out['out_depth'])

# Post Process
snpe_prediction = torch.nn.functional.interpolate(
        snpe_prediction.unsqueeze(1),
        size=img.shape[:2],
        mode="bicubic",
        align_corners=False,
    ).squeeze()

output = snpe_prediction.cpu().numpy()

# Save output as img
plt.imsave('snpe_dog_depth_prediction.jpg', output)
