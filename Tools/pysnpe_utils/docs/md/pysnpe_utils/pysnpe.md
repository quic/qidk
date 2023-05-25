# Table of Contents

* [pysnpe\_utils.pysnpe](#pysnpe_utils.pysnpe)
  * [TargetDevice](#pysnpe_utils.pysnpe.TargetDevice)
    * [\_\_init\_\_](#pysnpe_utils.pysnpe.TargetDevice.__init__)
    * [setDeviceProtocol](#pysnpe_utils.pysnpe.TargetDevice.setDeviceProtocol)
    * [prepareArtifactsOnsDevice](#pysnpe_utils.pysnpe.TargetDevice.prepareArtifactsOnsDevice)
  * [SnpeContext](#pysnpe_utils.pysnpe.SnpeContext)
    * [\_\_init\_\_](#pysnpe_utils.pysnpe.SnpeContext.__init__)
    * [set\_target\_device](#pysnpe_utils.pysnpe.SnpeContext.set_target_device)
    * [to\_dlc](#pysnpe_utils.pysnpe.SnpeContext.to_dlc)
    * [gen\_dsp\_graph\_cache](#pysnpe_utils.pysnpe.SnpeContext.gen_dsp_graph_cache)
    * [visualize\_dlc](#pysnpe_utils.pysnpe.SnpeContext.visualize_dlc)
    * [profile](#pysnpe_utils.pysnpe.SnpeContext.profile)
    * [quantize](#pysnpe_utils.pysnpe.SnpeContext.quantize)
    * [execute\_dlc](#pysnpe_utils.pysnpe.SnpeContext.execute_dlc)
  * [export\_to\_onnx](#pysnpe_utils.pysnpe.export_to_onnx)
  * [export\_to\_torchscript](#pysnpe_utils.pysnpe.export_to_torchscript)
  * [export\_to\_tflite](#pysnpe_utils.pysnpe.export_to_tflite)
  * [export\_to\_tf\_keras\_model](#pysnpe_utils.pysnpe.export_to_tf_keras_model)
  * [export\_to\_tf\_frozen\_graph](#pysnpe_utils.pysnpe.export_to_tf_frozen_graph)
  * [visualize\_tf\_session\_graph](#pysnpe_utils.pysnpe.visualize_tf_session_graph)
  * [visualize\_tf\_keras\_model](#pysnpe_utils.pysnpe.visualize_tf_keras_model)

<a id="pysnpe_utils.pysnpe"></a>

# pysnpe\_utils.pysnpe

Python API wrapper over SNPE Tools and APIs for Auto DLC Generation, its Execution, Easy Integration and On-Device Prototyping of your DNN project.

<a id="pysnpe_utils.pysnpe.TargetDevice"></a>

## TargetDevice Objects

```python
class TargetDevice()
```

<a id="pysnpe_utils.pysnpe.TargetDevice.__init__"></a>

#### \_\_init\_\_

```python
def __init__(target_device_type: DeviceType = DeviceType.ARM64_ANDROID,
             device_host: str = "localhost",
             target_device_adb_id: str = None,
             target_device_ip: str = None,
             send_root_access_request: bool = True)
```

Description:
Target Device attributes required to prepare it for running inferences.
On instantiation, the DeviceProtocol is selected based on target-device type (architecture + os),
and subsequently the artifacts(binaries and libraries) are pushed onto the device.

Protocol Selection Order:
ARM64_ANDROID = ADB                                 <br>
ARM64_UBUNTU = ADB | NATIVE_BINARY | PYBIND         <br>
ARM64_OELINUX = ADB                                 <br>
ARM64_WINDOWS = NATIVE_BINARY | PYBIND | TSHELL     <br>
X86_64_LINUX = NATIVE_BINARY | PYBIND               <br>
X86_64_WINDOWS = NATIVE_BINARY | PYBIND             <br>

If Protocol == NATIVE_BINARY | PYBIND , then no need for Pushing artifacts onto the device.
Artifacts will be fetched from "SNPE_ROOT" on device itself.

**Arguments**:

- `target_device_type` _DeviceType, optional_ - Target device architecture and OS info. Defaults to DeviceType.ARM64_ANDROID.
  
- `device_host` _str, optional_ - Host Name/IP on which target device is connected. Defaults to "localhost".
  
- `target_device_adb_id` _str, optional_ - ADB Serail ID of target device to uniquely identify when multiple devices are connected on Host machine. Serial ID can be found using `adb devices -l` command. Defaults to None.
  
- `target_device_ip` _str, optional_ - IP address of target device. If provided, this is help to make a wireless TCP/IP connection to the device using ADB or GRPC protocol. Defaults to None.
  
- `send_root_access_request` _bool, optional_ - Requests target device to run commands with root access.

<a id="pysnpe_utils.pysnpe.TargetDevice.setDeviceProtocol"></a>

#### setDeviceProtocol

```python
def setDeviceProtocol(device_protocol: DeviceProtocol = None)
```

Description:
    Sets Protocol to be used for communication with Target device.

<a id="pysnpe_utils.pysnpe.TargetDevice.prepareArtifactsOnsDevice"></a>

#### prepareArtifactsOnsDevice

```python
def prepareArtifactsOnsDevice(location_to_store: str = None,
                              send_root_access_request: bool = False) -> str
```

Description:
Push artifacts (SNPE Libs and Bins) onto the Target Device, based on DeviceProtocol.

**Returns**:

  Storage location of the assets of Target Device.

<a id="pysnpe_utils.pysnpe.SnpeContext"></a>

## SnpeContext Objects

```python
class SnpeContext()
```

<a id="pysnpe_utils.pysnpe.SnpeContext.__init__"></a>

#### \_\_init\_\_

```python
def __init__(model_path: str,
             model_framework: ModelFramework,
             dlc_path: str,
             input_tensor_map: Dict[str, List[int]],
             output_tensor_names: List[str],
             quant_encodings_path: str = None,
             target_device: TargetDevice = None,
             remote_session_name: str = None)
```

Description:
Stores metadata needed to generate a DLC and for other DLC operations

**Arguments**:

- `model_path` _str_ - Path of freezed graph which is to be converted to DLC
- `model_framework` _ModelFramework_ - Specifies the Model framework : TF, ONNX, CAFFE, PYTORCH, TFLITE
- `dlc_path` _str_ - Path to save generated DLC
- `input_tensor_map` _Dict[str, List]_ - Dict of the model's input names and their shape
- `output_tensor_names` _List[str]_ - List of the model's output names
- `quant_encodings_path` _str_ - Path to quantization encodings file (mostly generate using AIMET)
- `target_device` _TargetDevice_ - Target Device on which inference has to be executed.
- `remote_session_name` _str_ - Represents path at Target Device, where DLC and input tensors will be pushed.

<a id="pysnpe_utils.pysnpe.SnpeContext.set_target_device"></a>

#### set\_target\_device

```python
def set_target_device(target_device: TargetDevice = None)
```

Description:
    Adds Target Device into current SnpeContext, on which inference has to be done

<a id="pysnpe_utils.pysnpe.SnpeContext.to_dlc"></a>

#### to\_dlc

```python
def to_dlc()
```

Description:
    Converts freezed graph into DLC by invoking `SNPE Converter`

<a id="pysnpe_utils.pysnpe.SnpeContext.gen_dsp_graph_cache"></a>

#### gen\_dsp\_graph\_cache

```python
def gen_dsp_graph_cache(dlc_type: DlcType,
                        chipsets: List[str] = ["sm8550"],
                        overwrite_cache_records: bool = True)
```

Description:
Generates DSP Offline Cache (serialized graph) to save model initialization time, by invoking `snpe-dlc-graph-prepare` tool

On success, the `self.dlc_path` or `self.quant_dlc_path` will get updated with newly generated cached dlc path

**Arguments**:

- `dlc_type` _DlcType_ - Type of DLC : FLOAT | QUANT . QUANT dlc is generated by SNPE Quantizer.
- `chipset` _List[str], optional_ - List of chipsets for which Graph cache needs to generated [sm8350,sm8450,sm8550]. Defaults to ["sm8550"].
- `overwrite_cache_records` _bool, optional_ - Overwrite previously generated Graph Cache. Defaults to True.

<a id="pysnpe_utils.pysnpe.SnpeContext.visualize_dlc"></a>

#### visualize\_dlc

```python
def visualize_dlc(save_path: str = None)
```

Description:
Saves DLC graph structure in HTML and Tabular textual format by invoking `snpe-dlc-viewer` and `snpe-dlc-info` tools

**Arguments**:

- `save_path` _str, optional_ - Path to save visualization output. If None, then output is save with save DLC name as prefix and ".html" and ".txt" as suffix

<a id="pysnpe_utils.pysnpe.SnpeContext.profile"></a>

#### profile

```python
def profile(runtime: Runtime = Runtime.CPU,
            dlc_type: DlcType = DlcType.FLOAT,
            target_device: TargetDevice = None,
            num_threads: int = 1,
            duration: int = 2,
            cpu_fallback: bool = True)
```

Description:
Profiles model execution time on provided runtime and gives metrics: Inference per second, Model Init and DeInit times

**Arguments**:

- `runtime` _Runtime, optional_ - Runtime on which DLC will be executed [CPU, GPU, GPU_FP16, DSP, AIP]. Defaults to CPU runtime
- `target_device` _TargetDevice, optional_ - Target device on which profiling for DLC is to be done. Defaults to None.
- `num_threads` _int, optional_ - Number of threads to be used for DLC execution. Defaults to 1
- `duration` _int, optional_ - Duration of time (in seconds) to run network execution. Defaults to 2 seconds.
- `cpu_fallback` _bool, optional_ - Fallback unsupported layer to CPU, if any. Defaults to True.

<a id="pysnpe_utils.pysnpe.SnpeContext.quantize"></a>

#### quantize

```python
def quantize(quant_scheme: List[QuantScheme] = [QuantScheme.AXIS_QUANT],
             act_bw: ActBw = ActBw.INT8,
             weight_bw: WeightBw = WeightBw.INT8,
             override_quant_params=False)
```

Description:
Quantizes DLC using provided quant_scheme, activation bitwidth and weight bitwidth

**Arguments**:

- `quant_scheme` _List[QuantScheme]_ - List of SNPE provided quant schemes. Defaults to Axis Quantization
- `act_bw` _ActBw, optional_ - Activation bitwidth (16 or 8). Defaults to 8.
- `weight_bw` _int, optional_ - Weight bitwidth (8 or 4). Defaults to 8.
- `override_quant_params` _bool, optional_ - Use quant encodings provided from encodings file. Defaults to False.

<a id="pysnpe_utils.pysnpe.SnpeContext.execute_dlc"></a>

#### execute\_dlc

```python
def execute_dlc(input_tensor_map: Dict[str, np.ndarray],
                dlc_type: DlcType = DlcType.FLOAT,
                transpose_input_order: Dict[str, Tuple] = None,
                target_acclerator: Runtime = Runtime.CPU,
                target_device: TargetDevice = None) -> Dict[str, np.ndarray]
```

Description:
Executes DLC on target device or x86/ARM64 host machine using 'snpe-net-run' for ADB protocol
and using 'snpe python bindings' for PYBIND and GRPC protocol.

**Arguments**:

- `input_tensor_map` _Dict[str, np.ndarray]_ - Input tensor name and its tensor data in Numpy Nd-Array FP32 format.
  
- `dlc_type` _DlcType_ - Whether the DLC is FLOAT type or QUANT type.
  
  
- `transpose_input_order` _Dict[str, Tuple]_ - SNPE expects the input tensor to be NHWC (Batch x Height x Width x Channel) format, whereas DNN Frameworks like Pytorch, ONNX uses NCHW (Batch x Channel x Height x Width) format. Providing transpose order will make the input tensor in format acceptable to SNPE. For example, ONNX input dim = (1,3,224,224) and SNPE DLC input dim = (1,224,224,3), then providing Dictionary {'layer_name': (0,2,3,1)} will do the needful format conversion.
  
- `target_acclerator` _Runtime_ - Snapdragon DSP, CPU, GPU, GPU-FP16 and legacy AIP acclerator.
  
- `target_device` _TargetDevice_ - Target device on which DLC is to be executed. Defaults to None.
  

**Returns**:

  Dict[str, np.ndarray]: Output Tensor Name : its Output Tensor, generated after inference

<a id="pysnpe_utils.pysnpe.export_to_onnx"></a>

#### export\_to\_onnx

```python
def export_to_onnx(model: torch.nn.Module,
                   input_tensor_map: List[InputMap],
                   output_tensor_names: List[str],
                   onnx_file_name: str,
                   opset_version: int = 11) -> SnpeContext
```

Description:
Exports Pytorch model (nn.Module) to ONNX format and saves it on disk.

**Arguments**:

- `model` _torch.nn.Module_ - A PyTorch model (nn.Module) to export to ONNX format.
- `input_tensor_map` _List[InputMap]_ - A list of `InputMap` object: (input_name, shape, dtype)
- `output_tensor_names` _List[str]_ - List of output names for the ONNX model.
- `onnx_file_name` _str_ - The filename for the exported ONNX model.
- `opset_version` _int, optional_ - ONNX opset version to use. Defaults to 11.
  

**Returns**:

  SnpeContext Class instance required to generate DLC and perform other DLC operations
  

**Raises**:

- `RuntimeError` - If an error occurs during the export process.
  

**Example**:

  ```python
  from pysnpe_utils import pysnpe
  
  # Create a PyTorch model and specify the input shape
  model = MyModel()
  
  input_map = InputMap("img_1", (1, 3, 224, 224), torch.float32)
  output_tensor_names = ["out_1"]
  onnx_file_name = 'model.onnx'
  
  pysnpe.export_to_onnx(model, [input_map], output_tensor_names, onnx_file_name).to_dlc()
  ```

<a id="pysnpe_utils.pysnpe.export_to_torchscript"></a>

#### export\_to\_torchscript

```python
def export_to_torchscript(model: torch.nn.Module,
                          input_shapes: List[Tuple[int]],
                          input_dtypes: List[torch.dtype],
                          output_file_path: str,
                          enable_optimizations: bool = True,
                          strict_tracing: bool = True) -> SnpeContext
```

Exports a PyTorch model to TorchScript format and saves it to disk.

**Arguments**:

- `model` _torch.nn.Module_ - A PyTorch model (nn.Module) to export to TorchScript format.
- `input_shape` _List[Tuple[int]]_ - A tuple specifying the shape of the input tensor for the model.
  input_dtypes (List[torch.dtype]) : A tuple specifying the datatype of the input tensor for the model.
- `output_file_path` _str_ - A string specifying the file path to which the exported TorchScript model will be saved.
- `enable_optimizations` _bool_ - Flag to enable Graph optimizations, which are helpful for inference
- `strict_tracing` _bool_ - Use 'strict' model while tracing torch module
  

**Returns**:

  None
  

**Raises**:

- `RuntimeError` - If an error occurs during the export process.
  

**Example**:

  ```python
  from pysnpe_utils import pysnpe
  
  # Create a PyTorch model and specify the input shape
  model = MyModel()
  input_shapes = [(3, 224, 224)]
  input_dtypes = [torch.float32]
  
  # Export the model to TorchScript format
  output_file_path = 'model.pt'
  pysnpe.export_to_torchscript(model, input_shape,input_dtypes, output_file_path).to_dlc()
  ```

<a id="pysnpe_utils.pysnpe.export_to_tflite"></a>

#### export\_to\_tflite

```python
def export_to_tflite(keras_model: tf.keras.Model,
                     tflite_file_name: str) -> None
```

Description:
Converts a TensorFlow Keras model to TFLite format.

**Arguments**:

- `keras_model` _tf.keras.Model_ - A TensorFlow Keras model.
- `tflite_file_name` _str_ - A string indicating the name of the output TFLite file to be saved.
  

**Returns**:

  SnpeContext Class instance required to generate DLC and perform other DLC operations
  

**Raises**:

- `RuntimeError` - If an error occurs during the export process.
  

**Example**:

  ```python
  model = tf.keras.Sequential([
  tf.keras.layers.Conv2D(32, (3,3), activation='relu', input_shape=(28,28,1)),
  tf.keras.layers.MaxPooling2D((2,2)),
  tf.keras.layers.Flatten(),
  tf.keras.layers.Dense(10, activation='softmax')
  ])
  model.build()
  model.summary()
  
  # Convert and save the model
  export_to_tflite(model, 'test_model.tflite').to_dlc()
  ```

<a id="pysnpe_utils.pysnpe.export_to_tf_keras_model"></a>

#### export\_to\_tf\_keras\_model

```python
def export_to_tf_keras_model(model: Union[tf.keras.Model, tf.function],
                             input_tensor_map: List[InputMap],
                             keras_model_path: str,
                             skip_model_optimizations: bool = False,
                             frozen_graph_path: str = None,
                             tflite_path: str = None) -> SnpeContext
```

Description:
Optimizes and Saves a TensorFlow Keras model to disk, with fixed input shapes for each input layer.

**Arguments**:

- `model` _tf.keras.Model | tf.function_ - A TensorFlow model to export, either a TensorFlow Keras model or a TensorFlow function.
- `input_tensor_map` _List[InputMap]_ - A list of `InputMap` object: (input_name, shape, dtype)
- `keras_model_path` _str_ - Path to save keras model.
- `skip_model_optimizations` _bool, optional_ - Whether to skip optimization passes in the TensorFlow graph. Default is False.
- `frozen_graph_path` _str, optional_ - Path to save optimized frozen graph. Defaults to None.
- `tflite_path` - If provided, the function will convert the model to a TensorFlow Lite model and save it to the specified tflite_path.
  

**Returns**:

  SnpeContext Class instance required to generate DLC and perform other DLC operations
  

**Raises**:

- `RuntimeError` - If an error occurs during the export process.
  

**Example**:

  ```python
  model = tf.keras.Sequential([
  tf.keras.layers.Conv2D(32, (3,3), activation='relu', input_shape=(28,28,1)),
  tf.keras.layers.MaxPooling2D((2,2)),
  tf.keras.layers.Flatten(),
  tf.keras.layers.Dense(10, activation='softmax')
  ])
  model.build()
  model.summary()
  
  # Define the input tensor shapes
  input_map = [ InputMap('input_1', (1,28,28,1), tf.float32) ]
  
  # Optimize and save the model
  export_to_tf_keras_model(model,
  input_map,
  'test_keras_model',
  skip_model_optimizations=False,
  frozen_graph_path='test_frozen_graph.pb')
  ```

<a id="pysnpe_utils.pysnpe.export_to_tf_frozen_graph"></a>

#### export\_to\_tf\_frozen\_graph

```python
def export_to_tf_frozen_graph(session: tf.compat.v1.Session,
                              input_tensor_map: Dict[str, List],
                              output_tensor_names: List,
                              output_model_path: str) -> SnpeContext
```

Exports TF GraphDef to Frozen Graph (.pb) format

**Arguments**:

- `session` _tf.compat.v1.Session_ - The TensorFlow Session containing the trained model.
- `input_tensor_map` _Dict[str, List]_ - A dictionary of model input layer names with their dimensions. It can be found out by visualization of model.
- `output_tensor_names` _List_ - A list of model output layer names. It can be found out by visualization of model.
- `output_model_path` _str_ - The name and path of the model for saving as TF frozen graph with ".pb" extension.

<a id="pysnpe_utils.pysnpe.visualize_tf_session_graph"></a>

#### visualize\_tf\_session\_graph

```python
def visualize_tf_session_graph(session: tf.compat.v1.Session, model_name: str,
                               output_dir: str)
```

Description:
For visualization of TF Session graph with Netron Viewer. It is helpful for identifying model input and output layer names and understanding model topology.

**Arguments**:

- `session` _tf.compat.v1.Session_ - The TF Session containing Graph.
- `model_name` _str_ - Name for frozen graph to be saved with ".pb" extension.
- `output_dir` _str_ - Directory where to write the graph.

<a id="pysnpe_utils.pysnpe.visualize_tf_keras_model"></a>

#### visualize\_tf\_keras\_model

```python
def visualize_tf_keras_model(model: tf.keras.Model, output_file_path: str)
```

Description:
Visualizes a TensorFlow Keras model and saves the visualization to a file.

**Arguments**:

- `model` - A TensorFlow Keras model or a TensorFlow function.
- `output_file_path` - The path to save the visualization file.

