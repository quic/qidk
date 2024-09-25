#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

import numpy as np
import onnx
from onnx import helper
from onnx import helper, numpy_helper
from onnx import AttributeProto, TensorProto, GraphProto

array1 = np.random.rand(128).astype(np.float32)
gamma = numpy_helper.from_array(array1, name='Gamma')


array2 = np.random.rand(128).astype(np.float32)
beta = numpy_helper.from_array(array2, name='Beta')

custom_op = helper.make_node(
    "InputPack",
    inputs=["In", "Mask"],
    outputs=["Out"]
)

#attribute0 = helper.make_attribute('Eps', 0.000009999999747378752)
#custom_op.attribute.append(attribute0)
#
#attribute1 = helper.make_attribute('Axis', [2])
#custom_op.attribute.append(attribute1)
#
#attribute2 = helper.make_attribute('C', 1.2533141374588013)
#custom_op.attribute.append(attribute2)

'''
value = onnx.helper.make_tensor('C', onnx.TensorProto.FLOAT, [1], [1.2533141374588013])
new_attr = onnx.helper.make_attribute("C", value)
custom_op.attribute.insert(0, new_attr)

value = onnx.helper.make_tensor('epsilon', onnx.TensorProto.FLOAT, [1], [9.999999974752427e-7])
new_attr = onnx.helper.make_attribute("epsilon", value)
custom_op.attribute.insert(1, new_attr)

new_attr = onnx.helper.make_attribute("gamma", gamma)
custom_op.attribute.insert(0, new_attr)

new_attr = onnx.helper.make_attribute("beta", beta)
custom_op.attribute.insert(1, new_attr)
'''

graph = helper.make_graph(
    [custom_op],
    "model",
    [
        helper.make_tensor_value_info("In", TensorProto.FLOAT, [1,128,1,1680]),
        helper.make_tensor_value_info("Mask", TensorProto.FLOAT, [1,1680])
    ],
    [
        helper.make_tensor_value_info("Out", TensorProto.FLOAT, [1,128,1,1680])
    ]
)

#graph.value_info.append(gamma_info)
#graph.value_info.append(beta_info)

model = helper.make_model(graph, producer_name="model")


onnx.save(model, "model.onnx")
