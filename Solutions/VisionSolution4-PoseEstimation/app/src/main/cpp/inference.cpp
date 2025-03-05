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

//#include <android/trace.h>
//#include <dlfcn.h>

std::unique_ptr<zdl::SNPE::SNPE> snpe_HRNET;
std::unique_ptr<zdl::SNPE::SNPE> snpe_BB;

std::mutex mtx;
static zdl::DlSystem::Runtime_t runtime = zdl::DlSystem::Runtime_t::CPU;
static zdl::DlSystem::RuntimeList runtimeList;
bool useUserSuppliedBuffers = true;
bool useIntBuffer = false;
int bitWidth = 32;

struct timeval start_time, end_time;
float milli_time, seconds, useconds;

zdl::DlSystem::UserBufferMap inputMap, outputMap;
std::vector <std::unique_ptr<zdl::DlSystem::IUserBuffer>> snpeUserBackedInputBuffers, snpeUserBackedOutputBuffers;
std::unordered_map <std::string, std::vector<float32_t>> applicationOutputBuffers;
std::unordered_map <std::string, std::vector<float32_t>> applicationInputBuffers;


zdl::DlSystem::UserBufferMap inputMap_pose, outputMap_pose;
std::vector <std::unique_ptr<zdl::DlSystem::IUserBuffer>> snpeUserBackedInputBuffers_pose, snpeUserBackedOutputBuffers_pose;
std::unordered_map <std::string, std::vector<float32_t>> applicationOutputBuffers_pose;
std::unordered_map <std::string, std::vector<float32_t>> applicationInputBuffers_pose;


std::map<int, std::string> classnamemapping =
        {
                {0, "person"},{ 1, "bicycle"},{ 2, "car"},{ 3, "motorcycle"},{ 4, "airplane"},{ 5, "bus"},{
                 6, "train"},{ 7, "truck"},{ 8, "boat"},{ 9, "traffic"},{ 10, "fire"},{ 11, "stop"},{ 12, "parking"},{
                 13, "bench"},{ 14, "bird"},{ 15, "cat"},{ 16, "dog"},{ 17, "horse"},{ 18, "sheep"},{ 19, "cow"},{
                 20, "elephant"},{ 21, "bear"},{ 22, "zebra"},{ 23, "giraffe"},{ 24, "backpack"},{ 25, "umbrella"},{
                 26, "handbag"},{ 27, "tie"},{ 28, "suitcase"},{ 29, "frisbee"},{ 30, "skis"},{ 31, "snowboard"},{
                 32, "sports"},{ 33, "kite"},{ 34, "baseball"},{ 35, "baseball"},{ 36, "skateboard"},{ 37, "surfboard"},{
                 38, "tennis"},{ 39, "bottle"},{ 40, "wine"},{ 41, "cup"},{ 42, "fork"},{ 43, "knife"},{ 44, "spoon"},{
                 45, "bowl"},{ 46, "banana"},{ 47, "apple"},{ 48, "sandwich"},{ 49, "orange"},{ 50, "broccoli"},{
                 51, "carrot"},{ 52, "hot"},{ 53, "pizza"},{ 54, "donut"},{ 55, "cake"},{ 56, "chair"},{ 57, "couch"},{
                 58, "potted"},{ 59, "bed"},{ 60, "dining"},{ 61, "toilet"},{ 62, "tv"},{ 63, "laptop"},{ 64, "mouse"},{
                 65, "remote"},{ 66, "keyboard"},{ 67, "cell"},{ 68, "microwave"},{ 69, "oven"},{ 70, "toaster"},{
                 71, "sink"},{ 72, "refrigerator"},{ 73, "book"},{ 74, "clock"},{ 75, "vase"},{ 76, "scissors"},{
                 77, "teddy"},{ 78, "hair"},{ 79, "toothbrush"}
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
    createInputBufferMap(inputMap, applicationInputBuffers, snpeUserBackedInputBuffers, snpe_BB, useIntBuffer, bitWidth);
    createOutputBufferMap(outputMap, applicationOutputBuffers, snpeUserBackedOutputBuffers, snpe_BB, useIntBuffer, bitWidth);

    return outputLogger;
}

std::string build_network_pose(const uint8_t * dlc_buffer_HRNET, const size_t dlc_size_HRNET, const char runtime_arg)
{
    std::string outputLogger;
    bool usingInitCaching = false;  //shubham: TODO check with true

    std::unique_ptr<zdl::DlContainer::IDlContainer> container_HRNET = nullptr ;

    container_HRNET = loadContainerFromBuffer(dlc_buffer_HRNET, dlc_size_HRNET);

    if (container_HRNET == nullptr) {
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
    snpe_HRNET = setBuilderOptions(container_HRNET, runtime, runtimeList, useUserSuppliedBuffers, usingInitCaching);
    mtx.unlock();
    if (snpe_HRNET == nullptr) {
        LOGE("SNPE Prepare failed: Builder option failed for HRNET");
        outputLogger += "Model Prepare failed for HRNET";
        return outputLogger + "SNPE Prepare failed for HRNET";
    }

    outputLogger += "\nHRNET Model Network Prepare success !!!\n";
    createInputBufferMap(inputMap_pose, applicationInputBuffers_pose, snpeUserBackedInputBuffers_pose, snpe_HRNET, useIntBuffer, bitWidth); //256 x192
    createOutputBufferMap(outputMap_pose, applicationOutputBuffers_pose, snpeUserBackedOutputBuffers_pose, snpe_HRNET, useIntBuffer, bitWidth); //64x48

    return outputLogger;
}


float*** execcomb(cv::Mat &img, int orig_width, int orig_height, int &numberofhuman, std::vector<std::vector<float>> &BB_coords) {


    //LOGI("execute_net_BB");
    mtx.lock();
    //ATrace_beginSection("BB time");
    assert(snpe_BB!=nullptr);

    gettimeofday(&start_time, NULL);

    if(!loadInputUserBuffer_BB(applicationInputBuffers, snpe_BB, img, inputMap, bitWidth))
    {
        LOGE("Failed to load Input UserBuffer");
        mtx.unlock();
        return nullptr;
    }

    std::string name_out_boxes = "893";
    std::string name_out_classes = "885";

    bool execStatus = snpe_BB->execute(inputMap, outputMap);
    if(execStatus== true){
        LOGI("Exec BB status is true");
    }
    else{
        LOGE("Exec BB status is false");
        mtx.unlock();
        return nullptr;
    }


    std::vector<float32_t> BBout_boxcoords = applicationOutputBuffers.at(name_out_boxes);
//    LOGI("BBout_boxcoords: %f %f %f %f ",BBout_boxcoords[0], BBout_boxcoords[1], BBout_boxcoords[2], BBout_boxcoords[3]);

    std::vector<float32_t> BBout_class = applicationOutputBuffers.at(name_out_classes);
//    LOGI("BBout_class: %f",BBout_class[0]);

    std::vector<BoxCornerEncoding> Boxlist;
    std::vector<float32_t> Scorelist;
    float ***multipersoncoors = new float**[100]; //TODO MAX person limit is 100


    for(int i =0;i<(2100);i++)  //TODO change value of 8400 to soft value
    {
        int start = i*80;
        int end = (i+1)*80;

        auto it = max_element (BBout_class.begin()+start, BBout_class.begin()+end);
        int index = distance(BBout_class.begin()+start, it);

        std::string classname = classnamemapping[index];
        if(*it>=0.5 && (index%80) == 0)  //0 is for person class
        {
            int x1 = BBout_boxcoords[i * 4 + 0];
            int y1 = BBout_boxcoords[i * 4 + 1];
            int x2 = BBout_boxcoords[i * 4 + 2];
            int y2 = BBout_boxcoords[i * 4 + 3];
            Boxlist.push_back(BoxCornerEncoding(x1, y1, x2, y2,*it));
        }
    }

    //LOGI("Boxlist size:: %d",Boxlist.size());
    std::vector<BoxCornerEncoding> reslist = NonMaxSuppression(Boxlist,0.20);
    //LOGI("reslist ssize %d", reslist.size());

    //LOGI("orig_width = %d orig_height = %d",orig_width,orig_height);
    numberofhuman = reslist.size();
    float ratio_2 = orig_width/320.0f;
    float ratio_1 = orig_height/320.0f;
    //LOGI("ratio1 %f :: ratio_2 %f",ratio_1,ratio_2);
    //ATrace_endSection();

    for(int i=0;i<numberofhuman;i++) {

            //ATrace_beginSection("PRE process time HRNET");
            double center[2],scale[2];

            // LOGI("orig_width %d orig_hight %d", orig_width, orig_height);
            // orig_width is equal to img.cols
            // orig_height is equal to img.rows

            float top,bottom,left,right;
            left = reslist[i].y1 * ratio_1;   //y1
            right = reslist[i].y2 * ratio_1;  //y2

            bottom = reslist[i].x1 * ratio_2;  //x1
            top = reslist[i].x2 * ratio_2;   //x2

            //LOGI("bottomlefttopright%f %f %f %f",bottom, left, top, right);

            //LOGI("execute_net_pose");

            assert(snpe_HRNET!=nullptr);
            //Enabling opencv mat
            if(!loadInputUserBuffer_pose(applicationInputBuffers_pose, snpe_HRNET, img, inputMap_pose, bitWidth, center,scale, top, bottom, left, right))
            {
                LOGE("Failed to load Input UserBuffer");
                mtx.unlock();
                return nullptr;
            }

            std::string name_output_heatmap = "output";
            std::vector<float32_t> myoutput = applicationOutputBuffers_pose.at(name_output_heatmap);

            //ATrace_endSection();
            //ATrace_beginSection("Inference time HRNET");

            gettimeofday(&start_time, NULL);

            bool execStatus = snpe_HRNET->execute(inputMap_pose, outputMap_pose);
            //ATrace_endSection();

            gettimeofday(&end_time, NULL);
            seconds = end_time.tv_sec - start_time.tv_sec; //seconds
            useconds = end_time.tv_usec - start_time.tv_usec; //milliseconds
            milli_time = ((seconds) * 1000 + useconds/1000.0);

            //LOGI("Pose Inference time %f ms", milli_time);

            //ATrace_beginSection("POST processing HRNET");
            std::vector<float> singleboxcoords{top,bottom,left,right,milli_time};
            BB_coords.push_back(singleboxcoords);

            if(execStatus== true){
                LOGI("ExecPose status is true");
            }
            else{
                LOGE("ExecPose  status is False");
                mtx.unlock();
                return nullptr;
            }
            std::vector<float32_t> outpose_heatmap = applicationOutputBuffers_pose.at(name_output_heatmap);

            //LOGI("outpose_heatmap.size(): %d",outpose_heatmap.size());

            float **resCoords;
            resCoords = getCoords(outpose_heatmap,center,scale);
            multipersoncoors[i] = resCoords;
            //ATrace_endSection();
            //LOGI("coords added");
    }

    mtx.unlock();
    return multipersoncoors;

}

