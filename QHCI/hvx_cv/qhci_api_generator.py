#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

import os
import sys
import re
from xml.dom.minidom import parse
from xml.etree import cElementTree as ET
import xml.dom.minidom

IDL_FILE = "inc/qhci.idl"
SRC_ROOT = "src/"

DEBUG_MODE = 0

class qhciFeature:
    def __init__(self,name=None):
        self.name=name
        self.description=""
        self.apilist = []


class qhciApi:
    def __init__(self,name=None):
        self.name=name
        self.description=""
        self.type="intrinsic" # "asm"
        self.inputs = []
        self.outputs = []
        self.func = ''

class qhciVar:
    def __init__(self,name=None):
        self.name=name
        self.description=""
        self.dtype = ""
        self.testsize = ""

def mkdir(path):
    folder = os.path.exists(path)
    if not folder:
        os.makedirs(path)
        print("Create dir for " + path)
    else:
        print("The folder exists!")
        sys.exit()

def parse_qhciXml(feature_list, path):
    tree = ET.parse(path)
    root = tree.getroot()
    for FeatureDef in list(root):
        name = FeatureDef.find('Name').text
        if name != None:
            print("*****Add feature " + name)
            qhci_feature = qhciFeature(name)
            qhci_feature.description = FeatureDef.find('Description').find('Content').text
            if qhci_feature.description == None:
                qhci_feature.description = ''
            for ApiDef in list(FeatureDef.find('ApiDefList')):
                name = ApiDef.find('Name').text
                if name != None:
                    qhci_api = qhciApi(name)
                    type = ApiDef.find('Type').text
                    if type == 'asm':
                        qhci_api.type = "asm"
                    qhci_api.description = ApiDef.find('Description').find('Content').text
                    if qhci_api.description == None:
                        qhci_api.description = ''
                    for Input in list(ApiDef.find('InputList')):
                        if Input.find('Name') != None:
                            name = Input.find('Name').text
                            if name != None:
                                qhci_var = qhciVar(name)
                                qhci_var.description = Input.find('Description').find('Content').text
                                if qhci_var.description == None:
                                    qhci_var.description = ''
                                qhci_var.dtype = Input.find('Datatype').text
                                qhci_var.testsize = Input.find('TestSize').text
                                qhci_api.inputs.append(qhci_var)
                    for Output in list(ApiDef.find('OutputList')):
                        if Output.find('Name') != None:
                            name = Output.find('Name').text
                            if name != None:
                                qhci_var = qhciVar(name)
                                qhci_var.description = Output.find('Description').find('Content').text
                                if qhci_var.description == None:
                                    qhci_var.description = ''
                                qhci_var.dtype = Output.find('Datatype').text
                                qhci_var.testsize = Output.find('TestSize').text
                                qhci_api.outputs.append(qhci_var)
                qhci_feature.apilist.append(qhci_api)
            feature_list.append(qhci_feature)

def print_featurelist(feature_list):
    if DEBUG_MODE == 1 or DEBUG_MODE == 2:
        for feature in feature_list:
            print("**************Feature************")
            print("Feature Name: " + feature.name)
            print("Feature Description " + feature.description)
            idx = 0
            for api in feature.apilist:
                print("  Api" + str(idx) + " " + api.name + " " + api.type + " (" + api.description + ")")
                idx1 = 0
                idx2 = 0
                for input in api.inputs:
                    print("    Input" + str(idx1) + ": " + input.name + " " + input.dtype + " " + input.testsize + " " + input.description)
                    idx1 += 1
                for output in api.outputs:
                    print("    Output" + str(idx2) + ": " + output.name + " " + output.dtype + " " + output.testsize + " " + output.description)
                    idx2 += 1
                idx +=1

#Auto add api in idl
def auto_idl_add(feature_list):
    str = ""
    for feature in feature_list:
        for api in feature.apilist:
            api.func = "    AEEResult " + api.name + "(\n"
            for input in api.inputs:
                if '*' in input.dtype:
                    dtype = input.dtype.replace('*', '')
                    dtype = dtype.replace(' ', '')
                    api.func += "        in sequence<" + dtype + "> " + input.name + ", " + "//" + input.description + "\n"
                else:
                    api.func += "        in " + input.dtype + " " + input.name + ", " + "//" + input.description + "\n"
            for cout, output in enumerate(api.outputs):
                if '*' in output.dtype:
                    dtype = output.dtype.replace('*', '')
                    dtype = dtype.replace(' ', '')
                    if cout == len(api.outputs) - 1:
                        api.func += "        rout sequence<" + dtype + "> " + output.name + " " + "//" + output.description + "\n"
                    else:
                        api.func += "        rout sequence<" + dtype + "> " + output.name + ", " + "//" + output.description + "\n"
            api.func += "    );" + "\n"
            str += api.func
    #print(str)
    lines = ""
    pos = 0
    with open(IDL_FILE, 'r') as f:
        lines = f.readlines()
        for i,line in enumerate(reversed(lines)):
            if "}" in line:
                pos = len(lines) - i
                break
    #print(pos)
    start = lines[:pos-1]
    end = lines[pos-1:]
    str = ''.join(start) + str + ''.join(end)
    if DEBUG_MODE == 1:
        print(str)
    elif DEBUG_MODE == 2:
        pass
    else:
        exist_api = ""
        with open(IDL_FILE, "r") as f:
            file_content = f.read()
        for feature in feature_list:
            for api in feature.apilist:
                _str = api.func.splitlines()[0]
                if _str in file_content:
                    exist_api = api.name
                    break
        if exist_api == "":
            print("\033[32mNew Api, generate define in IDL.\033[0m")
            with open(IDL_FILE,"w") as f:
                f.write(str)
        else:
            print("\033[31m{} exists, pls remove define in IDL first !!\033[0m".format(exist_api))


#Auto create dsp impl files
def auto_dsp_add(feature_list):
    for feature in feature_list:
        DSP_DIR = SRC_ROOT + feature.name + "/dsp"
        if not os.path.exists(DSP_DIR):
            print("Create new DSP dir: " + DSP_DIR)
            os.makedirs(DSP_DIR)
        DSP_FILE = DSP_DIR + "/" + feature.name + "_imp.c"
        BASE_DSP_FILE = SRC_ROOT + 'dummy/dsp/dummy_imp.c'
        lines = ''
        with open(BASE_DSP_FILE, 'r') as f:
            lines = f.readlines()
        str = ''.join(lines)
        for api in feature.apilist:
            sublist = api.func.splitlines(True)
            newlist = []
            for k,v in enumerate(sublist):
                sublist[k] = sublist[k][4:]
                if k == 0:
                    newlist.append(sublist[k][:10] + "qhci_" + sublist[k][10:])
                    newlist.append("    remote_handle64 handle,\n")
                if " in " in sublist[k]:
                    sublist[k] = sublist[k].replace(" in ", " ")
                    if "sequence" in sublist[k]:
                        p1 = sublist[k].find('sequence')
                        p2 = sublist[k].find('>')
                        p3 = sublist[k].find('<')
                        oldstr = sublist[k][p1:p2+1]
                        newstr = "const " + sublist[k][p3+1:p2] + "*"
                        sublist[k] = sublist[k].replace(oldstr, newstr)
                        newlist.append(sublist[k])
                        p1 = sublist[k].find('* ')
                        p2 = sublist[k].find(',')
                        newstr = "    int " + sublist[k][p1+2:p2] + "Len,\n"
                        newlist.append(newstr)
                    else:
                        newlist.append(sublist[k])
                if " rout " in sublist[k]:
                    sublist[k] = sublist[k].replace(" rout ", " ")
                    if "sequence" in sublist[k]:
                        p1 = sublist[k].find(' /')
                        start = sublist[k][:p1]
                        end = sublist[k][p1:]
                        p1 = start.find('sequence')
                        p2 = start.find('>')
                        p3 = start.find('<')
                        oldstr = start[p1:p2+1]
                        newstr = start[p3+1:p2] + "*"
                        start = start.replace(oldstr, newstr)
                        if "," not in start:
                            start += ","
                        newlist.append(start+end)
                        p1 = start.find('* ')
                        newstr = "    int " + start[p1+2:].replace(',', '').replace(' ', '') + "Len,\n"
                        newlist.append(newstr)
                if k == len(sublist) - 1:
                    newlist.append(sublist[k].replace(";", ""))
            p1 = newlist[-2].find(',')
            newstr = list(newlist[-2])
            newstr[p1] = ''
            newlist[-2] = ''.join(newstr)
            api.func = ''.join(newlist)
            api.func += "{\n    return AEE_SUCCESS;\n}\n"
            str += api.func
        if DEBUG_MODE == 1:
            print(str)
        elif DEBUG_MODE == 2:
            pass
        else:
            if os.path.exists(DSP_FILE):
                print("\033[31m{} exists, pls rename or remove current file first !!\033[0m".format(DSP_FILE))
            else:
                print("\033[32m{} not exist, start to generate new file..\033[0m".format(DSP_FILE))
                with open(DSP_FILE,"w") as f:
                    f.write(str)

#Auto create cpu ref/test files
def auto_cpu_add(feature_list):
    for feature in feature_list:
        CPU_DIR = SRC_ROOT + feature.name + "/cpu"
        if not os.path.exists(CPU_DIR):
            print("Create new CPU dir: " + CPU_DIR)
            os.makedirs(CPU_DIR)
        CPU_FILE = CPU_DIR + "/" + feature.name + ".cpp"
        BASE_CPU_FILE = SRC_ROOT + 'dummy/cpu/dummy.cpp'
        lines = []
        new_lines = []
        with open(BASE_CPU_FILE, 'r') as f:
            lines = f.readlines()
        for k,v in enumerate(lines):
            if "#define FEATURE_NAME" in v:
                v = v.replace("dummy", feature.name)
                new_lines.append(v)
            elif "Step0" in v:
                new_lines.append(v)
                apilist = []
                for api in feature.apilist:
                    _list = re.split('(\n)', api.func)
                    _list[0] = _list[0].replace('(', "_ref(")
                    _list.append("\n")
                    apilist += _list
                new_lines.extend(apilist)
            elif "class" in v:
                v = v.replace("DUMMY", feature.name.upper())
                new_lines.append(v)
            elif "Api teat" in v:
                new_lines.append(v)
                sublist = lines[k+1:k+14]
                for api in feature.apilist:
                    for substr in sublist:
                        if "Step1" in substr:
                            new_lines.append(substr)
                            param_list = []
                            _param = ''
                            for input in api.inputs:
                                if '*' in input.dtype:
                                    _param = "            uint32_t " + input.name + "Len" + " = " + input.testsize + ";\n"
                                    param_list.append(_param)
                                    _param = "            " + input.dtype + " " + input.name + " = " \
                                        + "(" + input.dtype + ")"  + "rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, " \
                                        + input.name + "Len" + "*sizeof(" + input.dtype.replace("*", "") + "));\n"
                                    param_list.append(_param)
                                else:
                                    _param = "            " + input.dtype + " " + input.name + " = " + input.testsize + ";\n"
                                    param_list.append(_param)
                            for output in api.outputs:
                                if '*' in output.dtype:
                                    _param = "            uint32_t " + output.name + "Len" + " = " + output.testsize + ";\n"
                                    param_list.append(_param)
                                    _param = "            " + output.dtype + " " + output.name + "_cpu = " \
                                        + "(" + output.dtype + ")"  + "rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, " \
                                        + output.name + "Len" + "*sizeof(" + output.dtype.replace("*", "") + "));\n"
                                    param_list.append(_param)
                                    _param = "            " + output.dtype + " " + output.name + "_dsp = " \
                                        + "(" + output.dtype + ")"  + "rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, " \
                                        + output.name + "Len" + "*sizeof(" + output.dtype.replace("*", "") + "));\n"
                                    param_list.append(_param)
                            new_lines.extend(param_list)
                        elif "Step2" in substr:
                            new_lines.append(substr)
                            param_list = []
                            _param = ''
                            for input in api.inputs:
                                if '*' in input.dtype:
                                    _param = "            GenerateRandomData((uint8_t*)" + input.name + ", " \
                                        + input.name + "Len" + "*sizeof(" + input.dtype.replace("*", "") + "));\n"
                                    param_list.append(_param)
                            new_lines.extend(param_list)
                        elif "Step3" in substr:
                            new_lines.append(substr)
                            _param = "            TIME_STAMP(\"Cpu\", "
                            param_list = api.func.splitlines()
                            for k,v in enumerate(param_list[:-3]):
                                if "AEEResult" in v:
                                    _param += v.replace("AEEResult ", "").replace("(", "_ref(")
                                elif ")" in v:
                                    _param += v
                                elif "const" in v:
                                    _param += v.split()[2]
                                elif "*" in v:
                                    _str = v.split()[1]
                                    for output in api.outputs:
                                        if output.name in _str:
                                            _str = _str.replace(",", "_cpu,")
                                    _param += _str
                                else:
                                    _param += v.split()[1]
                            _param += ");\n"
                            new_lines.append(_param)
                        elif "Step4" in substr:
                            new_lines.append(substr)
                            _param = "            TIME_STAMP(\"DSP\", "
                            param_list = api.func.splitlines()
                            for k,v in enumerate(param_list[:-3]):
                                if "AEEResult" in v:
                                    _param += v.replace("AEEResult ", "")
                                elif ")" in v:
                                    _param += v
                                elif "const" in v:
                                    _param += v.split()[2]
                                elif "*" in v:
                                    _str = v.split()[1]
                                    for output in api.outputs:
                                        if  output.name in _str:
                                            _str = _str.replace(",", "_dsp,")
                                    _param += _str
                                else:
                                    _param += v.split()[1]
                            _param += ");\n"
                            new_lines.append(_param)
                        elif "Step5" in substr:
                            new_lines.append(substr)
                            for output in api.outputs:
                                _param = "            nErr = CompareBuffers(" + output.name + "_cpu, " + output.name + "_dsp, " + output.name + "Len);\n"
                                new_lines.append(_param)
                                _param = "            CHECK(nErr==AEE_SUCCESS);\n"
                                new_lines.append(_param)
                        elif "Step6" in substr:
                            new_lines.append(substr)
                            for input in api.inputs:
                                if "*" in input.dtype:
                                    _param = "            if(" + input.name + ") rpcmem_free(" + input.name + ");\n"
                                    new_lines.append(_param)
                            for output in api.outputs:
                                _param = "            if(" + output.name + "_cpu) rpcmem_free(" + output.name + "_cpu);\n"
                                new_lines.append(_param)
                                _param = "            if(" + output.name + "_dsp) rpcmem_free(" + output.name + "_dsp);\n"
                                new_lines.append(_param)
                        else:
                            new_lines.append(substr)
            elif k >= 12 and k <= 24:
                continue
            elif "static DUMMY*" in v:
                new_lines.append(v.replace("DUMMY", feature.name.upper()))
            else:
                new_lines.append(v)
                
        str = ''.join(new_lines)
        if DEBUG_MODE == 1:
            print(str)
        elif DEBUG_MODE == 2:
            pass
        else:
            if os.path.exists(CPU_FILE):
                print("\033[31m{} exists, pls rename or remove current file first !!\033[0m".format(CPU_FILE))
            else:
                print("\033[32m{} not exist, start to generate new file..\033[0m".format(CPU_FILE))
                with open(CPU_FILE,"w") as f:
                    f.write(str)

def auto_makefile_add(feature_list):
    CPU_MIN = "cpu.min"
    DSP_MIN = "hexagon.min"

    #cpu
    with open(CPU_MIN, "r") as f:
        file_content = f.read()
    for feature in feature_list:
        cpu_str = "src/" + feature.name + "/cpu/" + feature.name
    if cpu_str in file_content:
        if DEBUG_MODE != 2 and DEBUG_MODE != 1:
            print("\033[31m{} exists, skip add to cpu.min !!\033[0m".format(cpu_str))
    else:
        substr = "$(EXE_NAME)_CPP_SRCS += " + cpu_str + "\n$(EXE_NAME)_DEFINES"
        file_content = file_content.replace("$(EXE_NAME)_DEFINES", substr)
        if DEBUG_MODE == 1:
            print(file_content)
        elif DEBUG_MODE == 2:
            pass
        else:
            print("\033[32m{} not exist, start to add to cpu.min..\033[0m".format(cpu_str))
            with open(CPU_MIN,"w") as f:
                f.write(file_content)

    #dsp
    with open(DSP_MIN, "r") as f:
        file_content = f.read()
    for feature in feature_list:
        dsp_str = "src/" + feature.name + "/dsp/" + feature.name + "_imp"
    if dsp_str in file_content:
        if DEBUG_MODE != 2 and DEBUG_MODE != 1:
            print("\033[31m{} exists, skip add to dsp.min !!\033[0m".format(dsp_str))
    else:
        substr = "libqhci_skel_C_SRCS += " + dsp_str + "\nlibqhci_skel_DLLS"
        file_content = file_content.replace("libqhci_skel_DLLS", substr)
        if DEBUG_MODE == 1:
            print(file_content)
        elif DEBUG_MODE == 2:
            pass
        else:
            print("\033[32m{} not exist, start to add to dsp.min..\033[0m".format(dsp_str))
            with open(DSP_MIN,"w") as f:
                f.write(file_content)

if __name__ == "__main__":
    featureList = []
    #step1. parse xml config  "ExampleQhciApi.xml"
    path = 'QhciApi.xml'
    parse_qhciXml(featureList, path)
    # parse_qhciXml(featureList, sys.argv[1])
    #print_featurelist(featureList)
    auto_idl_add(featureList)
    auto_dsp_add(featureList)
    auto_cpu_add(featureList)
    auto_makefile_add(featureList)

