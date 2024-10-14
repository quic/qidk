#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

import tensorflow as tf
import numpy as np
from tensorflow.python.platform import gfile

tf.compat.v1.disable_eager_execution()
tf.compat.v1.disable_v2_behavior()

import sys, os
import json

if len(sys.argv) < 4 :
    print("Usage : python batch_tf_inf.py <Frozen_pb_file_name> <Input_raw_img_list.txt> <Input_layer_name> <Output_layer_name>")
    print("\nMultiple Input, Output tensors/layer name can be specified with comma seprated values, without space")
    print("Multiple Raw inputs can be specified by giving a InputList.txt, same as SNPE/QNN")
    sys.exit()
else:
    print("Frozen MOdel = {}".format(sys.argv[1]))
    print("Raw_input = {}".format(sys.argv[2]))
    print("Input layer name = {}".format(sys.argv[3]))
    print("Output layer name = {}".format(sys.argv[4]))

    os.system("mkdir -p tf_out")
    print("Output Dir = tf_out")

    # check if argv[2], argv[3] & argv[4] has comma seperated values
    raw_input_file = sys.argv[2]
    with open(raw_input_file, 'r') as f:
        raw_inputs = f.read().splitlines()
    input_layers = sys.argv[3].split(",")
    output_layers = sys.argv[4].split(",")

    if (len(raw_inputs[0].split()) != len(input_layers)):
        raise Exception("no. of raw_inputs must match no. of inputs layers")

    with tf.Graph().as_default() as graph:
        with tf.compat.v1.Session() as sess:
            print("Session Loaded\n")

            with gfile.FastGFile(sys.argv[1], 'rb') as f:
                print("PB loaded\n")

                # load graph def
                graph_def = tf.compat.v1.GraphDef()
                graph_def.ParseFromString(f.read())
                sess.graph.as_default()
                print("Graph def Loaded\n")

                #load weights, ...
                tf.import_graph_def(
                    graph_def,
                    input_map=None,
                    return_elements=None,
                    name="",
                    op_dict=None,
                    producer_op_list=None
                )
                print("Graph imported\n")

                # print ops
                # for op in graph.get_operations():
                #     print("Operation Name = {}".format(op.name))
                #     print("Tensor Stats = {}\n".format(op.values()))
                # print("OPs printed\n")

                #init global vars
                tf.compat.v1.global_variables_initializer()

                # iterate till no. of entries in InputList.txt
                for iter in range(len(raw_inputs)):
                    print("Iter = {}".format(iter))
                    current_raw_inputs = raw_inputs[iter].split()
                    # set Input Tensor & map input raws
                    layer_inputRaw_map = {}
                    for idx, l_name in enumerate(input_layers):
                        # set key
                        l_inp_tensor = graph.get_tensor_by_name(l_name)
                        print("l_input", l_inp_tensor)
                        print("Shape of input : ", l_inp_tensor.shape)

                        if l_name.split(":")[0] not in current_raw_inputs[idx]:
                            print("{} not in {}".format(l_name, current_raw_inputs[idx]))
                            raise Exception("Input list sequence mismatch")
                        # reshape input
                        img = open(current_raw_inputs[idx].split(":=")[-1], 'r')
                        inp = np.fromfile(img, dtype='float32')
                        img.close()
                        print("Image Loaded\n")
                        inp = np.reshape(inp, l_inp_tensor.shape)
                        print("input img shape : ", inp.shape)

                        layer_inputRaw_map[l_inp_tensor] = inp
                        # layer_inputRaw_map[l_inp_tensor] = raw_inputs[idx] # for ref. input raw name
                    # print(layer_inputRaw_map)

                    # set Output Tensor 
                    l_output_tensors = []
                    for idx, l_name in enumerate(output_layers):
                        l_output_tensors.append(graph.get_tensor_by_name(l_name))
                    print("l_output", l_output_tensors)

                    # set IO tensors
                    # l_input = graph.get_tensor_by_name(sys.argv[3])
                    # l_output = graph.get_tensor_by_name(sys.argv[4])              
                    
                    # init inference
                    # outputs = sess.run(l_output, feed_dict={l_input:inp})
                    outputs = sess.run(l_output_tensors, feed_dict=layer_inputRaw_map)
                    print("No. of outputs : {}".format(len(outputs)))

                    # os.system("mkdir -p tf_out/{}".format(iter))
                    # out_dir = "tf_out/infer_{0:03}_".format(iter)
                    out_dir = "tf_out/infer_{}_".format(iter)
                    for idx, output in enumerate(outputs):    
                        print("output shape : {}".format(output.shape))
                        #Grammar: model_name + output_layer_name.raw
                        out_raw_file_name = sys.argv[1].split(".")[0] + "__" + output_layers[idx].split(":")[0].replace('/', '_')
                        out_raw_file_name = out_dir + out_raw_file_name 
                        # out_raw_file_name = sys.argv[2].split(".")[0] + "_out.raw"
                        print("Saving raw output with name = ", out_raw_file_name)

                        with open(out_raw_file_name + ".raw", "w") as fd:
                            output.astype('float32').tofile(fd)

                        out = output.ravel()
                        out_elem_to_show = 10 if len(out)>=10 else len(out)
                        # print("\nFirst {} output elem: ".format(out_elem_to_show))
                        # for i in range(out_elem_to_show):
                        #     print(out[i])

                        print("writing output in text format = {}.json\n\n======".format(out_raw_file_name))
                        with open(out_raw_file_name + ".json",'w') as tp:
                            tp.write(json.dumps(output.tolist()))
                            # tp.write('[\n')
                            # for x in range(len(out)):
                            #     tp.write(str(out[x])+' , ')
                            # tp.write('\n]\n')
