//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include <jni.h>
#include <string>
#include <iostream>

#include <iterator>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <hpp/inference.h>

#include "android/log.h"

#include "hpp/CheckRuntime.hpp"
#include "hpp/SetBuilderOptions.hpp"
#include "hpp/Util.hpp"
#include "LoadContainer.hpp"
#include "CreateUserBuffer.hpp"
#include "LoadInputTensor.hpp"

#include <opencv2/core/types_c.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/types_c.h>

std::unique_ptr<zdl::SNPE::SNPE> snpe_BB;

std::mutex mtx;
static zdl::DlSystem::Runtime_t runtime = zdl::DlSystem::Runtime_t::CPU;
static zdl::DlSystem::RuntimeList runtimeList;
bool useUserSuppliedBuffers = true;
bool useIntBuffer = false;

zdl::DlSystem::UserBufferMap inputMap, outputMap;
std::vector <std::unique_ptr<zdl::DlSystem::IUserBuffer>> snpeUserBackedInputBuffers, snpeUserBackedOutputBuffers;
std::unordered_map <std::string, std::vector<float32_t>> applicationOutputBuffers;
std::unordered_map <std::string, std::vector<float32_t>> applicationInputBuffers;
int bitWidth = 32;


#include <android/trace.h>
#include <dlfcn.h>

std::vector<std::string> classnamemapping = {
        "N/A", "person", "bicycle", "car", "motorcycle", "airplane", "bus",
        "train", "truck", "boat", "traffic light", "fire hydrant", "N/A",
        "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse",
        "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "N/A", "backpack",
        "umbrella", "N/A", "N/A", "handbag", "tie", "suitcase", "frisbee", "skis",
        "snowboard", "sports ball", "kite", "baseball bat", "baseball glove",
        "skateboard", "surfboard", "tennis racket", "bottle", "N/A", "wine glass",
        "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich",
        "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake",
        "chair", "couch", "potted plant", "bed", "N/A", "dining table", "N/A",
        "N/A", "toilet", "N/A", "tv", "laptop", "mouse", "remote", "keyboard",
        "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator", "N/A",
        "book", "clock", "vase", "scissors", "teddy bear", "hair drier",
        "toothbrush"
};



inline float ComputeIntersectionOverUnion(const BoxCornerEncoding &box_i,const BoxCornerEncoding &box_j)
{
    const float box_i_y_min = std::min<float>(box_i.y1, box_i.y2);
    const float box_i_y_max = std::max<float>(box_i.y1, box_i.y2);
    const float box_i_x_min = std::min<float>(box_i.x1, box_i.x2);
    const float box_i_x_max = std::max<float>(box_i.x1, box_i.x2);
    const float box_j_y_min = std::min<float>(box_j.y1, box_j.y2);
    const float box_j_y_max = std::max<float>(box_j.y1, box_j.y2);
    const float box_j_x_min = std::min<float>(box_j.x1, box_j.x2);
    const float box_j_x_max = std::max<float>(box_j.x1, box_j.x2);

    const float area_i =
            (box_i_y_max - box_i_y_min) * (box_i_x_max - box_i_x_min);
    const float area_j =
            (box_j_y_max - box_j_y_min) * (box_j_x_max - box_j_x_min);
    if (area_i <= 0 || area_j <= 0) return 0.0;
    const float intersection_ymax = std::min<float>(box_i_y_max, box_j_y_max);
    const float intersection_xmax = std::min<float>(box_i_x_max, box_j_x_max);
    const float intersection_ymin = std::max<float>(box_i_y_min, box_j_y_min);
    const float intersection_xmin = std::max<float>(box_i_x_min, box_j_x_min);
    const float intersection_area =
            std::max<float>(intersection_ymax - intersection_ymin, 0.0) *
            std::max<float>(intersection_xmax - intersection_xmin, 0.0);
    return intersection_area / (area_i + area_j - intersection_area);
}

std::vector<BoxCornerEncoding> NonMaxSuppression(std::vector<BoxCornerEncoding> boxes,
                                                 const float iou_threshold)
{

    if (boxes.size()==0) {
        return boxes;
    }

    std::sort(boxes.begin(), boxes.end(), [] (const BoxCornerEncoding& left, const BoxCornerEncoding& right) {
        if (left.score > right.score) {
            return true;
        } else {
            return false;
        }
    });


    std::vector<bool> flag(boxes.size(), false);
    for (unsigned int i = 0; i < boxes.size(); i++) {
        if (flag[i]) {
            continue;
        }

        for (unsigned int j = i + 1; j < boxes.size(); j++) {
            if (ComputeIntersectionOverUnion(boxes[i],boxes[j]) > iou_threshold) {
                flag[j] = true;
            }
        }
    }

    std::vector<BoxCornerEncoding> ret;
    for (unsigned int i = 0; i < boxes.size(); i++) {
        if (!flag[i])
            ret.push_back(boxes[i]);
    }

    return ret;
}

std::string build_network_BB(const uint8_t * dlc_buffer_BB, const size_t dlc_size_BB, const char runtime_arg)
{
    std::string outputLogger;
    bool usingInitCaching = false;  //shubham: TODO check with true

    std::unique_ptr<zdl::DlContainer::IDlContainer> container_BB = nullptr ;

    container_BB = loadContainerFromBuffer(dlc_buffer_BB, dlc_size_BB);

    if (container_BB == nullptr) {
        LOGE("Error while opening the container file.");
        return "Error while opening the container file.\n";
    }

    runtimeList.clear();
    LOGI("runtime arg %c",runtime_arg);
    zdl::DlSystem::Runtime_t runtime = zdl::DlSystem::Runtime_t::CPU;
    if (runtime_arg == 'D'){
        runtime = zdl::DlSystem::Runtime_t::DSP;
        LOGI("Added DSP");
    }
    else if (runtime_arg == 'G')
    {
        runtime = zdl::DlSystem::Runtime_t::GPU_FLOAT32_16_HYBRID; //can be written as GPU
        LOGI("Added GPU");
    }

    if(runtime != zdl::DlSystem::Runtime_t::UNSET)
    {
        bool ret = runtimeList.add(checkRuntime(runtime));
        if(ret == false){
            LOGE("Cannot set runtime");
            return outputLogger + "\nCannot set runtime";
        }
    } else {
        return outputLogger + "\nCannot set runtime";
    }


    mtx.lock();
    snpe_BB = setBuilderOptions(container_BB, runtime, runtimeList, useUserSuppliedBuffers, usingInitCaching);
    mtx.unlock();

    if (snpe_BB == nullptr) {
        LOGE("SNPE Prepare failed: Builder option failed for BB");
        outputLogger += "Model Prepare failed for BB";
        return outputLogger + "SNPE Prepare failed for BB";
    }

    outputLogger += "\nBB Model Network Prepare success !!!\n";

    //Creating Buffer
    createInputBufferMap(inputMap, applicationInputBuffers, snpeUserBackedInputBuffers, snpe_BB, useIntBuffer, bitWidth);
    createOutputBufferMap(outputMap, applicationOutputBuffers, snpeUserBackedOutputBuffers, snpe_BB, useIntBuffer, bitWidth);
    return outputLogger;
}


bool executeDLC(cv::Mat &img, int orig_width, int orig_height, int &numberofobj, std::vector<std::vector<float>> &BB_coords, std::vector<std::string> &BB_names) {

    LOGI("execute_net_BB");
    ATrace_beginSection("preprocessing");

    struct timeval start_time, end_time;
    float milli_time, seconds, useconds;

    mtx.lock();
    assert(snpe_BB!=nullptr);

    if(!loadInputUserBuffer_BB(applicationInputBuffers, snpe_BB, img, inputMap, bitWidth))
    {
        LOGE("Failed to load Input UserBuffer");
        mtx.unlock();
        return false;
    }

    //TODO Need to take from model, Output value might change.
    std::string name_out_boxes = "5842";
    std::string name_out_classes =  "5850";

    ATrace_endSection();
    gettimeofday(&start_time, NULL);
    ATrace_beginSection("inference time");

    bool execStatus = snpe_BB->execute(inputMap, outputMap);
    ATrace_endSection();
    ATrace_beginSection("postprocessing time");
    gettimeofday(&end_time, NULL);
    seconds = end_time.tv_sec - start_time.tv_sec; //seconds
    useconds = end_time.tv_usec - start_time.tv_usec; //milliseconds
    milli_time = ((seconds) * 1000 + useconds/1000.0);
    //LOGI("Inference time %f ms", milli_time);

    if(execStatus== true){
        LOGI("Exec BB status is true");
    }
    else{
        LOGE("Exec BB status is false");
        mtx.unlock();
        return false;
    }
    const auto& outputNamesOpt = snpe_BB->getOutputTensorNames();
    const zdl::DlSystem::StringList& outputNames = *outputNamesOpt;
    for(auto k:outputNames){
        LOGI("output tensors %s",k);
    }
    std::vector<float32_t> BBout_boxcoords1600 = applicationOutputBuffers.at(name_out_boxes);
    std::vector<float32_t> BBout_class3640 = applicationOutputBuffers.at(name_out_classes);

    std::vector<float32_t> BBout_boxcoords(BBout_boxcoords1600.begin(), BBout_boxcoords1600.begin() + 400);
    std::vector<float32_t> BBout_class(BBout_class3640.begin(), BBout_class3640.begin() + 9100);

    std::vector<BoxCornerEncoding> Boxlist;
    std::vector<std::string> Classlist;
    //Post Processing
    for(int i =0;i<(100);i++)  //TODO change value of 2100 to soft value
    {
        int start = i*91;
        int end = (i+1)*91;

        auto it = max_element (BBout_class.begin()+start, BBout_class.begin()+end);
        int index = distance(BBout_class.begin()+start, it);
        std::string classname = classnamemapping[index];
        if(*it>0.4 )
        {

            int x1 = (BBout_boxcoords[i * 4 + 0]-0.5*BBout_boxcoords[i * 4 + 2])*320;
            int y1 = (BBout_boxcoords[i * 4 + 1]-0.5*BBout_boxcoords[i * 4 + 3])*320;
            int x2 = (BBout_boxcoords[i * 4 + 0]+0.5*BBout_boxcoords[i * 4 + 2])*320;
            int y2 = (BBout_boxcoords[i * 4 + 1]+0.5*BBout_boxcoords[i * 4 + 3])*320;

            LOGI("it %d %d %d %d %s",x1,y1,x2,y2,classname.c_str());


            Boxlist.push_back(BoxCornerEncoding(x1, y1, x2, y2,*it,classname));
        }
    }

    std::vector<BoxCornerEncoding> reslist = NonMaxSuppression(Boxlist,0.20);

    numberofobj = reslist.size();
    float ratio_2 = orig_width/320.0f;
    float ratio_1 = orig_height/320.0f;

    for(int k=0;k<numberofobj;k++) {
        float top,bottom,left,right;
        left = reslist[k].y1 * ratio_1;   //y1
        right = reslist[k].y2 * ratio_1;  //y2

        bottom = reslist[k].x1 * ratio_2;  //x1
        top = reslist[k].x2 * ratio_2;   //x2


        std::vector<float> singleboxcoords{top, bottom, left, right, milli_time};
        BB_coords.push_back(singleboxcoords);
        BB_names.push_back(reslist[k].objlabel);
    }

    ATrace_endSection();
    mtx.unlock();
    return true;
}