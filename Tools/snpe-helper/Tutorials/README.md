# Introduction
Deploying DNN model on Snapdragon is a 4 step process with SNPE:
1. Exporting model in SNPE supported format (ONNX, TF Keras-Saved Model/Frozen Graph, TF-Lite, Torchscript)
2. Converting exported model into SNPE DLC (Deep Learning Container)
3. Optional DLC quantization and DSP Offline Graph Preparation (Caching)
4. DLC execution on Target Device

# SnpeContext API usage

The SnpeContext class object stores the basic metadata required for Model execution. SnpeContext class has to be inherited based on respective usecase and perform operations independently. SnpeContext class uses "snpehelper" module which acts as an interface between Python and C++.

## SnpeContext Inheritance
1. dlc_path : Path to the DLC path on the device including model name like 'c:/Users/SESR_128_512_quantized_cached.dlc'.
2. input_layers : Input layer name of the DLC. 
3. output_layers : Output layer/s name of the DLC
4. output_tensors : Output tensor/s of the DLC
5. runtime : CPU or DSP runtime. Defaults to CPU.
6. profile_level : Performance level to run. Defaults to BALANCED
7. enable_cache : To save cache data into DLC. Defaults to False

### Example
```
class DeeplabV3_Resnet101(SnpeContext):
    def __init__(self,dlc_path: str = "None",
                    input_layers : list = [],
                    output_layers : list = [],
                    output_tensors : list = [],
                    runtime : str = Runtime.CPU,
                    profile_level : str = PerfProfile.BALANCED,
                    enable_cache : bool = False):
        super().__init__(dlc_path,input_layers,output_layers,output_tensors,runtime,profile_level,enable_cache)
```

#### Creating instance :
```
snpe_context = DeeplabV3_Resnet101(dlc_path="deeplabv3_resnet101_quantized.dlc",input_layers=["input.1"], output_layers=["/ Resize_1"],output_tensors=["1089"],runtime=Runtime.DSP,profile_level=PerfProfile.BURST,enable_cache=False)                              
```

From SnpeContext object, you can initialize bufferes using `Initialize()` API, Update input buffers using `SetInputBuffer()`, execute dlc using `execute()` API, retrieve inferenced buffer using `GetOutputBuffer()` measure Inference time using `@timer` API

## SetInputBuffer

### Python API
```
    def SetInputBuffer(self,input_data,input_layer):
        self.m_context.SetInputBuffer(input_data,input_layer)
        return
```
```json
input_data     -> preprocessed 1-D array data 
input_layer    -> layer name for which input_data is to be updated
```

#### Example
##### For Single input layer
```
    def run_vae(self,input_data):
        self.SetInputBuffer(input_data.flatten(),"input_1")
```

##### For Multiple input layer
```
    def run_unet(self,input_data_1, input_data_2, input_data_3):
        self.SetInputBuffer(input_data_1.flatten(),"input_1")
        self.SetInputBuffer(input_data_2.flatten(),"input_2")
        self.SetInputBuffer(input_data_3.flatten(),"input_3")
```
## GetOutputBuffer

### Python API
```
    def GetOutputBuffer(self,Tensor):
        return self.m_context.GetOutputBuffer(Tensor)
```
```json
Tensor     -> Output node for reading inferenced data 
returns    -> 1-D array 
```
##### For Single output tensor
```
    def postprocess(self):
        OutputBuffer = self.GetOutputBuffer("sr") 
```

##### For Multiple output tensor
```
    def postprocess(self,image):
        prob = self.GetOutputBuffer("5859")
        prob = prob.reshape(1,100,92)
        tensor_prob = torch.from_numpy(prob)
        
        boxes = self.GetOutputBuffer("5860")
        boxes = boxes.reshape(1,100,4)
        tensor_boxes = torch.from_numpy(boxes)
```

## Execute
```
    def Execute(self):
        return self.m_context.Execute()
```
```json
returns    -> True if success;False otherwise
```

Each SnpeContext can refer to a Floating Point DLC or a Quantized DLC.
If there are multiple DLCs or same DLC with multiple Quantization schemes or Mixed Precision DLCs, then it is suggested to create multiple SnpeContexts for each of them.

#### Note: The DLC generated from ONNX has shape in `Batch x Height x Width x Channel` (NHWC - Channel last) format as it is the preferred Memory format of SNPE, whereas ONNX model has shape `Batch x Channel x Height x Width` (NCHW - Channel first) tensor Memory format. So its user responsibility to provide input-transpose-order, as mentioned in below example usage.

Example Usage:
```
# SNPE Context from segmentation example

def preprocess(self,images):
        transform = T.Compose([
                T.Resize((513,513)),
                
                T.ToTensor(),
                T.Normalize(mean=[0.485, 0.456, 0.406],
                                std=[0.229, 0.224, 0.225]),
            ])
        preprocess_image = []
        for image in images:
            img = transform(image).unsqueeze(0)
            input_image = img.numpy().transpose(0,2,3,1).astype(np.float32)
            preprocess_image.append(input_image[0].flatten())
        self.SetInputBuffer(preprocess_image,"input.1")
        return  

```
# NOTE: Place all DLLs and .so files in this Tutorials directory from below path
#### DLL path : "C:\Qualcomm\AIStack\SNPE\2.15.1.230926\lib\arm64x-windows-msvc"
#### so path : "C:\Qualcomm\AIStack\SNPE\2.15.1.230926\lib\hexagon-v73\unsigned"

###### *Qualcomm® Neural Processing SDK is a product of Qualcomm Technologies, Inc. and/or its subsidiaries.*
