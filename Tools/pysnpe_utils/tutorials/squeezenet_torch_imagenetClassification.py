import torchvision.models as models
import torch
from torchvision import transforms
import urllib.request
from PIL import Image
from icecream import ic

model = torch.hub.load('pytorch/vision:v0.10.0', 'squeezenet1_0', pretrained=True)
model.eval()

preprocess = transforms.Compose([
    transforms.Resize(256),
    transforms.CenterCrop(224),
    transforms.ToTensor(),
    transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225]),
])

# Download ImageNet labels
url = "https://raw.githubusercontent.com/pytorch/hub/master/imagenet_classes.txt"
urllib.request.urlretrieve(url, "imagenet_classes.txt")

# Download a sample image for inference
url, filename = ("https://github.com/pytorch/hub/raw/master/images/dog.jpg", "dog.jpg")
urllib.request.urlretrieve(url, filename)

# Load input img for inference
input_image = Image.open(filename)

# Apply transforms to the input image.
input_tensor = preprocess(input_image)
input_batch = input_tensor.unsqueeze(0)
ic(input_batch.shape)

# Post process and class prediction
def post_process(output):
    # Get the softmax probabilities.
    probabilities = torch.nn.functional.softmax(output[0], dim=0)
    # Read the categories
    with open("imagenet_classes.txt", "r") as f:
        categories = [s.strip() for s in f.readlines()]
    # Show top categories per image
    top5_prob, top5_catid = torch.topk(probabilities, 5)
    print("\n\nPredictions: \n","="*60)
    for i in range(top5_prob.size(0)):
        print(categories[top5_catid[i]], top5_prob[i].item())

# Run inference
with torch.no_grad():
    output = model(input_batch)

post_process(output)
input("\n\nPress Enter to Continue ...\n")

# ================================================================================
print("="*80 + "\nModel Conversion and Execution via Torchscript path\n" + "="*80)
# ========== Model Conversion and Execution via Torchscript path =================
from pysnpe_utils import pysnpe
from pysnpe_utils.pysnpe_enums import *

# Select target device
target_device = pysnpe.TargetDevice(device_host="10.206.64.253", 
                                    target_device_adb_id="728b7a92")

# Convert Torch model to TorchScript => DLC
pysnpe.export_to_torchscript(model, [(1, 3, 224, 224)], [torch.float32], "squeezenet.pt").to_dlc()

# Get DLC output layer name from Snpe-dlc-info, as output layer name changes & create SnpeConext
snpe_context = pysnpe.SnpeContext("squeezenet.pt", ModelFramework.PYTORCH, 
                                  "squeezenet.dlc", {"x":(1, 3, 224, 224)}, ["reshape_1_0"])\
                                        .gen_dsp_graph_cache(DlcType.FLOAT)\
                                            .set_target_device(target_device=target_device)\
                                                .profile(runtime=Runtime.DSP)

# Prepare inputs as Numpy Nd-Array for inference
input_tensor_map = { 'x' : input_batch.cpu().numpy() }

# Run inference
snpe_out = snpe_context.execute_dlc(input_tensor_map, target_acclerator=Runtime.DSP )

# SNPE predictions
post_process(torch.from_numpy(snpe_out['reshape_1_0']))