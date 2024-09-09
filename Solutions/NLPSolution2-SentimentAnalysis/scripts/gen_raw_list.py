# <input_layer_name>:=<input_layer_path>[<space><input_layer_name>:=<input_layer_path>]

import sys

if len(sys.argv) != 2:
    print("Usage : python gen_raw_list.py <no. of iterations>")
    sys.exit()

total_iter = int(sys.argv[1])
print("Generating input_list \"small_raw_list.txt\" with {} iterations".format(total_iter))

with open("tf_raw_list.txt",'w') as f:
    for i in range(total_iter):
        f.write("input_ids:=input_ids/inp_ids_{}.raw attention_mask:=attention_mask/attn_mask_{}.raw\n".format(i,i)) # add token mask if needed

with open("snpe_raw_list.txt",'w') as f:
    for i in range(total_iter):
        f.write("input_ids:0:=input_ids/inp_ids_{}.raw attention_mask:0:=attention_mask/attn_mask_{}.raw\n".format(i,i)) 
