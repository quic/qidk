import os
import sys
import torch
import torchvision.models as models

model = models.mobilenet_v3_large(pretrained=True)

model.load_state_dict(torch.load('mobilenet_v3_large-8738ca79.pth'))
model.eval()

os.makedirs("./ONNX/", exist_ok=True)
os.makedirs("./Pytorch/", exist_ok=True)
dummy_input = torch.randn(1, 3, 224, 224).type(torch.FloatTensor).to('cpu')
torch.onnx.export(model, dummy_input, "./ONNX/mobilenet_v3.onnx",opset_version=14)


traced_model = torch.jit.trace(model, dummy_input)
traced_model.save("./Pytorch/mobilenet_v3.pt")
