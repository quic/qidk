//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include <jni.h>
#include <string>
#include <iostream>
#include <opencv2/core/types_c.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/types_c.h>
#include "android/log.h"

#include "zdl/SNPE/SNPE.hpp"
#include "zdl/SNPE/SNPEFactory.hpp"
#include "zdl/DlSystem/DlVersion.hpp"
#include "zdl/DlSystem/DlEnums.hpp"
#include "zdl/DlSystem/String.hpp"
#include "zdl/DlContainer/IDlContainer.hpp"
#include "zdl/SNPE/SNPEBuilder.hpp"
#include "zdl/DlSystem/ITensor.hpp"
#include "zdl/DlSystem/StringList.hpp"
#include "zdl/DlSystem/TensorMap.hpp"
#include "zdl/DlSystem/TensorShape.hpp"
#include "DlSystem/ITensorFactory.hpp"

#include "hpp/LoadInputTensor.hpp"
#include "hpp/Util.hpp"
#include "inference.h"

bool SetAdspLibraryPath(std::string nativeLibPath) {
    nativeLibPath += ";/data/local/tmp/mv_dlc;/vendor/lib/rfsa/adsp;/vendor/dsp/cdsp;/system/lib/rfsa/adsp;/system/vendor/lib/rfsa/adsp;/dsp";

    __android_log_print(ANDROID_LOG_INFO, "SNPE ", "ADSP Lib Path = %s \n", nativeLibPath.c_str());
    std::cout << "ADSP Lib Path = " << nativeLibPath << std::endl;

    return setenv("ADSP_LIBRARY_PATH", nativeLibPath.c_str(), 1 /*override*/) == 0;
}


std::unique_ptr<zdl::DlContainer::IDlContainer> loadContainerFromBuffer(const uint8_t * buffer, const size_t size)
{
    std::unique_ptr<zdl::DlContainer::IDlContainer> container;
    container = zdl::DlContainer::IDlContainer::open(buffer, size);
    return container;
}


zdl::DlSystem::Runtime_t checkRuntime(zdl::DlSystem::Runtime_t runtime)
{
    static zdl::DlSystem::Version_t Version = zdl::SNPE::SNPEFactory::getLibraryVersion();

    LOGI("SNPE Version = %s", Version.asString().c_str()); //Print Version number

    if (!zdl::SNPE::SNPEFactory::isRuntimeAvailable(runtime)) {
        LOGE("Selected runtime not present. Falling back to GPU.");
        runtime = zdl::DlSystem::Runtime_t::GPU;
        if (!zdl::SNPE::SNPEFactory::isRuntimeAvailable(runtime)){
            LOGE("Selected runtime not present. Falling back to CPU.");
            runtime = zdl::DlSystem::Runtime_t::CPU;
        }
    }

    return runtime;
}

std::unique_ptr<zdl::SNPE::SNPE> setBuilderOptions(std::unique_ptr<zdl::DlContainer::IDlContainer> & container,
                                                   zdl::DlSystem::Runtime_t runtime,
                                                   zdl::DlSystem::RuntimeList runtimeList,
                                                   bool useUserSuppliedBuffers,
                                                   bool useCaching)
{
    std::unique_ptr<zdl::SNPE::SNPE> snpe;
    zdl::SNPE::SNPEBuilder snpeBuilder(container.get());

    if(runtimeList.empty())
    {
        runtimeList.add(runtime);
    }

    std::string platformOptionStr = "useAdaptivePD:ON";
//  if (isSignedStatus == UNSIGNED_PD) {
        // use unsignedPD feature for untrusted app.
        // platformOptionStr += "unsignedPD:ON";
//  }
    zdl::DlSystem::PlatformConfig platformConfig;
    bool setSuccess = platformConfig.setPlatformOptions(platformOptionStr);
    if (!setSuccess)
        LOGE("=========> failed to set platformconfig: %s", platformOptionStr.c_str());
    else
        LOGI("=========> platformconfig set: %s", platformOptionStr.c_str());

    bool isValid = platformConfig.isOptionsValid();
    if (!isValid)
        LOGE("=========> platformconfig option is invalid");
    else
        LOGI("=========> platformconfig option: valid");


    zdl::DlSystem::StringList stringruntime = runtimeList.getRuntimeListNames();
    for (const char *name : stringruntime)
        LOGI("runtime sh %s", name);

    snpe = snpeBuilder.setOutputLayers({})
            .setPerformanceProfile(zdl::DlSystem::PerformanceProfile_t::BURST)
            .setExecutionPriorityHint(
                    zdl::DlSystem::ExecutionPriorityHint_t::HIGH)
            .setRuntimeProcessorOrder(runtimeList)
            .setUseUserSuppliedBuffers(useUserSuppliedBuffers)
            .setPlatformConfig(platformConfig)
            .setInitCacheMode(useCaching)
            //.setCPUFallbackMode(true)
            .setUnconsumedTensorsAsOutputs(true)
            .build();

    return snpe;
}

// ==============================User Buffer func=================================== //
// ================================================================================= //


//CreateUserbuffer INPUT/OUTPUT for BB
void createUserBuffer(zdl::DlSystem::UserBufferMap& userBufferMap,
                      std::unordered_map<std::string, std::vector<float32_t>>& applicationBuffers,
                      std::vector<std::unique_ptr<zdl::DlSystem::IUserBuffer>>& snpeUserBackedBuffers,
                      std::unique_ptr<zdl::SNPE::SNPE>& snpe,
                      const char * name,
                      const bool isTfNBuffer,
                      int bitWidth)
{

    auto bufferAttributesOpt = snpe->getInputOutputBufferAttributes(name);
    if (!bufferAttributesOpt) throw std::runtime_error(std::string("Error obtaining attributes for input tensor ") + name);

    // calculate the size of buffer required by the input tensor
    const zdl::DlSystem::TensorShape& bufferShape = (*bufferAttributesOpt)->getDims();

    size_t bufferElementSize = 0;
    if (isTfNBuffer) {
        bufferElementSize = bitWidth / 8;
    }
    else {
        bufferElementSize = sizeof(float);
    }

    // Calculate the stride based on buffer strides.
    // Note: Strides = Number of bytes to advance to the next element in each dimension.
    // For example, if a float tensor of dimension 2x4x3 is tightly packed in a buffer of 96 bytes, then the strides would be (48,12,4)
    // Note: Buffer stride is usually known and does not need to be calculated.

    int num_dims = bufferShape.rank(); //bufferShape rank is generally 1 more than expected, as it add 1 for batchSize, so 300x300x3 will look like 1x300x300x3
    std::vector<size_t> strides(num_dims);
    strides[strides.size() - 1] = bufferElementSize;
    size_t stride = strides[strides.size() - 1];
    for (size_t i = num_dims - 1; i > 0; i--) {
        stride *= bufferShape[i];
        strides[i - 1] = stride;
        //LOGI("\nstrides[%d]: %d",i,stride);
        //LOGI("\nbuffershape[%d]: %d",i,bufferShape[i]);
    }

    size_t bufSize=bufferElementSize;
    for(int i=0;i<num_dims;i++)
        bufSize*=bufferShape[i];

    LOGI("\n BufferName: %s Bufsize: %d", name, bufSize);

    // set the buffer encoding type
    std::unique_ptr<zdl::DlSystem::UserBufferEncoding> userBufferEncoding;
    if (isTfNBuffer)
        userBufferEncoding = std::unique_ptr<zdl::DlSystem::UserBufferEncodingTfN>(
                new zdl::DlSystem::UserBufferEncodingTfN(0, 1.0, bitWidth));
    else
        userBufferEncoding = std::unique_ptr<zdl::DlSystem::UserBufferEncodingFloat>(
                new zdl::DlSystem::UserBufferEncodingFloat());

    // create user-backed storage to load input data onto it
    applicationBuffers.emplace(name, std::vector<float32_t>(bufSize));

    // create SNPE user buffer from the user-backed buffer
    zdl::DlSystem::IUserBufferFactory &ubFactory = zdl::SNPE::SNPEFactory::getUserBufferFactory();
    snpeUserBackedBuffers.push_back(
            ubFactory.createUserBuffer(applicationBuffers.at(name).data(),
                                       bufSize,
                                       strides,
                                       userBufferEncoding.get()));
    if (snpeUserBackedBuffers.back() == nullptr)
        throw std::runtime_error(std::string("Error while creating user buffer."));

    // add the user-backed buffer to the inputMap, which is later on fed to the network for execution
    userBufferMap.add(name, snpeUserBackedBuffers.back().get());

}

/*
 Cretae OutPut Buffer Map for BB
 */
void createOutputBufferMap(zdl::DlSystem::UserBufferMap& outputMap,
                           std::unordered_map<std::string, std::vector<float32_t>>& applicationBuffers,
                           std::vector<std::unique_ptr<zdl::DlSystem::IUserBuffer>>& snpeUserBackedBuffers,
                           std::unique_ptr<zdl::SNPE::SNPE>& snpe,
                           bool isTfNBuffer,
                           int bitWidth)
{
    //LOGI("Creating Output Buffer for BB");
    const auto& outputNamesOpt = snpe->getOutputTensorNames();
    if (!outputNamesOpt) throw std::runtime_error("Error obtaining output tensor names");
    const zdl::DlSystem::StringList& outputNames = *outputNamesOpt;

    // create SNPE user buffers for each application storage buffer
    for (const char *name : outputNames) {
        LOGI("Creating output buffer %s", name);
        createUserBuffer(outputMap, applicationBuffers, snpeUserBackedBuffers, snpe, name, isTfNBuffer, bitWidth);
    }
}
/*
 * Create Input Buffer Map for BB
 */
void createInputBufferMap(zdl::DlSystem::UserBufferMap& inputMap,
                          std::unordered_map<std::string, std::vector<float32_t>>& applicationBuffers,
                          std::vector<std::unique_ptr<zdl::DlSystem::IUserBuffer>>& snpeUserBackedBuffers,
                          std::unique_ptr<zdl::SNPE::SNPE>& snpe,
                          bool isTfNBuffer,
                          int bitWidth) {
    //LOGI("Creating Input Buffer for BB");
    const auto &inputNamesOpt = snpe->getInputTensorNames();
    if (!inputNamesOpt) throw std::runtime_error("Error obtaining input tensor names");
    const zdl::DlSystem::StringList &inputNames = *inputNamesOpt;
    assert(inputNames.size() > 0);

    // create SNPE user buffers for each application storage buffer
    for (const char *name: inputNames) {
        LOGI("Creating Input Buffer = %s", name);
        createUserBuffer(inputMap, applicationBuffers, snpeUserBackedBuffers, snpe, name,
                         isTfNBuffer, bitWidth);
    }
}

void preprocess_BB(std::vector<float32_t> &dest_buffer, cv::Mat &img)
{
    cv::Mat img320;
    cv::resize(img,img320,cv::Size(320,320),cv::INTER_LINEAR);  //TODO get the size from model itself

    float inputScale = 0.00392156862745f;    //normalization value, this is 1/255

    float * accumulator = reinterpret_cast<float *> (&dest_buffer[0]);

    //opencv read in BGRA by default
    cvtColor(img320, img320, CV_BGRA2BGR);
    LOGI("num of channels: %d",img320.channels());
    int lim = img320.rows*img320.cols*3;
    for(int idx = 0; idx<lim; idx++)
        accumulator[idx]= img320.data[idx]*inputScale;

}


//Preprocessing and loading in application Input Buffer for BB
bool loadInputUserBuffer_BB(std::unordered_map<std::string, std::vector<float32_t>>& applicationBuffers,
                            std::unique_ptr<zdl::SNPE::SNPE>& snpe,
                            cv::Mat &img,
                            zdl::DlSystem::UserBufferMap& inputMap,
                            int bitWidth) {

    // get input tensor names of the network that need to be populated
    const auto &inputNamesOpt = snpe->getInputTensorNames();
    if (!inputNamesOpt) throw std::runtime_error("Error obtaining input tensor names");
    const zdl::DlSystem::StringList &inputNames = *inputNamesOpt;
    assert(inputNames.size() > 0);

    if (inputNames.size()) LOGI("Preprocessing and loading in application Input Buffer for BB");


    for (size_t j = 0; j < inputNames.size(); j++) {
        const char *name = inputNames.at(j);
        LOGI("Filling %s buffer ", name);

        if(bitWidth == 8 || bitWidth == 16) {
            LOGE("bitwidth 8 and 16 are NOT DEFINED");
            return false;
        } else {

            preprocess_BB(applicationBuffers.at(name),img);  //functions loads data in applicationBuffer

        }
    }
    return true;
}


void preprocess_pose(std::vector<float32_t> &dest_buffer, const cv::Mat &img, double center[], double scale[], const float &top, const float &bottom, const float &left, const float &right)
{
    getcenterscale(img.cols, img.rows, center, scale, bottom, left, top, right);
    cv::Mat trans = get_affine_transform(HRNET_model_input_height, HRNET_model_input_width, 0, center, scale);
    cv::Mat model_input(HRNET_model_input_width,HRNET_model_input_height, img.type());
    cv::warpAffine(img, model_input, trans,model_input.size(), cv::INTER_LINEAR);

    cvtColor(model_input, model_input, CV_BGRA2BGR);  //Changing num of channels
    //TODO if above statement takes time, then we can skip it and change logic in below loop
    int lim = model_input.rows*model_input.cols*3;
    float * accumulator = reinterpret_cast<float *> (&dest_buffer[0]);
    for(int idx = 0; idx<lim; ){
        accumulator[idx]= (float) (((model_input.data[idx] / 255.0f) - 0.485f) / 0.229f);
        accumulator[idx+1] = (float) (((model_input.data[idx+1] / 255.0f) - 0.456f) / 0.224f);
        accumulator[idx+2] = (float) (((model_input.data[idx+2] / 255.0f) - 0.406f) / 0.225f);
        idx=idx+3;
    }


}

bool loadInputUserBuffer_pose(std::unordered_map<std::string, std::vector<float32_t>>& applicationBuffers,
                         std::unique_ptr<zdl::SNPE::SNPE>& snpe,
                         const cv::Mat &img,
                         zdl::DlSystem::UserBufferMap& inputMap,
                         int bitWidth, double center[], double scale[], const float &top, const float &bottom, const float &left, const float &right) {

    // get input tensor names of the network that need to be populated
    const auto &inputNamesOpt = snpe->getInputTensorNames();
    if (!inputNamesOpt) throw std::runtime_error("Error obtaining input tensor names");
    const zdl::DlSystem::StringList &inputNames = *inputNamesOpt;
    assert(inputNames.size() > 0);

    if (inputNames.size()) LOGI("Preprocessing and loading in application Input Buffer for HRNET ");

    for (size_t j = 0; j < inputNames.size(); j++) {
        const char *name = inputNames.at(j);
        LOGI("Filling %s buffer ", name);

        if(bitWidth == 8 || bitWidth == 16) {
            LOGE("bitwidth 8 and 16 are NOT DEFINED");
            return false;
        }
        else {
            preprocess_pose(applicationBuffers.at(name),img, center, scale, top, bottom, left, right);
        }
    }
    return true;
}


void get_3rd_point(double a[], double b[], double dst[3][2]) {

    //dst is of size 3x2
    dst[2][1] = a[0] - b[0] +b[1];
    dst[2][0] = -a[1] + b[1] +b[0];
}

cv::Mat get_affine_transform(int dst_w, int dst_h, int inv, double center[], double scale[])
{
    float shift[] =  {0.0F, 0.0F};

    double scale_tmp[] = {scale[0]*200, scale[1]*200};
    double src_w = scale_tmp[0];

    double src_dir[] = {0, src_w * -0.5};
    double dst_dir[] = {0, dst_w * -0.5};

    double src[3][2];
    double dst[3][2];


    //for all columns
    for (int z = 0; z<2; z++) {
        src[0][z] = center[z] + scale_tmp[z] * shift[z];
        src[1][z] =center[z] + src_dir[z] + scale_tmp[z] * shift[z];

    }
    dst[0][0] = dst_w * 0.5;
    dst[0][1]=dst_h * 0.5;
    dst[1][0] = dst_w * 0.5+dst_dir[0];
    dst[1][1] = (dst_h * 0.5) + dst_dir[1];

    get_3rd_point(src[0], src[1], src);

    get_3rd_point(dst[0], dst[1], dst);

    std::vector<cv::Point2f> srcMat_vec;
    srcMat_vec.push_back(cv::Point2f(src[0][0],src[0][1]));
    srcMat_vec.push_back(cv::Point2f(src[1][0],src[1][1]));
    srcMat_vec.push_back(cv::Point2f(src[2][0], src[2][1]));


    std::vector<cv::Point2f> dstMat_vec;
    dstMat_vec.push_back(cv::Point2f(dst[0][0],dst[0][1]));
    dstMat_vec.push_back(cv::Point2f(dst[1][0],dst[1][1]));
    dstMat_vec.push_back(cv::Point2f(dst[2][0], dst[2][1]));

    cv::Mat warpMat;

    if (inv==1) {
        //LOGI("inv is 1");
        warpMat = cv::getAffineTransform(dstMat_vec, srcMat_vec);
    }
    else{
        warpMat = cv::getAffineTransform(srcMat_vec, dstMat_vec);
    }
    return warpMat;
}


void getcenterscale(int image_width, int image_height, double center[2], double scale[2],float bottom, float left, float top, float right) {
    center[0] = 0.0f;
    center[1] = 0.0F;

//    int bottom_left_corner[] = {0,0};
//    int top_right_corner[] = {image_width, image_height};

    int bottom_left_corner[] = {(int) bottom, (int) left};
    int top_right_corner[] = {(int) top, (int) right};

    double box_width = top_right_corner[0] - bottom_left_corner[0];
    double box_height = top_right_corner[1] - bottom_left_corner[1];
    int bottom_left_x = bottom_left_corner[0];
    int bottom_left_y = bottom_left_corner[1];
    center[0] = bottom_left_x + (box_width * 0.5);
    center[1] = bottom_left_y + (box_height * 0.5);

    double aspect_ratio = image_width * 1.0 / image_height;
    double pixel_std = 200;

    if (box_width > (aspect_ratio * box_height)) {
        //LOGI("width is big");
        box_height = box_width * 1.0 / aspect_ratio;
    }

    else if( box_width <aspect_ratio * box_height) {
        //LOGI("height is big");
        box_width = box_height * aspect_ratio;
    }

    scale[0] = box_width * 1.0 / pixel_std;
    scale[1] = box_height * 1.0 / pixel_std;

    if (center[0] != -1)
        for(int z = 0; z< 2; z++)
            scale[z] = scale[z] * 1.25;

    LOGI("center: %f %f",center[0],center[1]);
    LOGI("scale : %f %f",scale[0],scale[1]);
}

float** get_max_preds(float ***buff)
{
    int num_joints = 17;
    int width = 48; //TODO:take from global //treat 48 as width
    int height = 64;//TODO:take from global //treat 64 as height
    int size = width * height;

    //Converting 3D array to 2D array, here converting 17x64x48 -> 17x3072
    // TODO: could merge this part to later while calculating argmax
    float res[17][3072];
    for (int i = 0; i < num_joints; i++) {
        int k = 0; //index to hover on buff
        for (int j = 0; j < height; j++) {
            for (int z = 0; z < width; z++) {
                res[i][k++] = buff[i][j][z];
            }
        }
    }

    //argmax and maxval
    float max_list[num_joints];
    int max_idx_list[num_joints];

    float** preds = new float*[num_joints];

    for (int i = 0; i < num_joints; i++) {
        preds[i] = new float[3];
        float max = -1;
        int max_idx = -1;
        for (int j = 0; j < size; j++) {
            if (max < res[i][j]) {
                max = res[i][j];
                max_idx = j;
            }
        }

        //TODO make the threshold global or generic
        //Checking the value with threshold
        if (max > 0.2f) {
            int tempval = max_idx % width;
            preds[i][0] = tempval;
            preds[i][1] = max_idx / width;
            preds[i][2]=1;
        } else {
            preds[i][0] = 0;
            preds[i][1] = 0;
            preds[i][2]=0;

        }

    }

    return preds;
}


float** getCoords(std::vector<float32_t> buff, double center[], double scale[]) {


    int numKeypoints = 17; //NUMJOINTS; //17 keypoints are for 17 body parts. Each body port has a heat map.
    int heatmap_height = 64; //MODELOUTPUTHEIGHT; //treat 64 as height
    int heatmap_width = 48; //MODELOUTPUTWIDTH; //treat 48 as width

/*
        //Model gives output of size 5224 that could be directly mapped to 64x48x17.  <---important to understand structure

        //Following code Convert 1D heatmap buff to 3D heatmap array 52224->64,48,17
        int k = 0; //index to hover on buff
        float[][][] heatmaps = new float[heatmap_height][heatmap_width][numKeypoints];
        for (int i = 0; i < heatmap_height; i++) {
            for (int j = 0; j < heatmap_width; j++) {
                for (int z = 0; z < numKeypoints; z++)
                    heatmaps[i][j][z] = buff[k++];
            }
        }
*/
    //17 64 48
    //Converting flat buff of 5224(64x48x17)format to 17x64x48 directly
    float ***heatmaps_converted = new float**[numKeypoints];
    for (int i = 0; i < numKeypoints; i++) {
        heatmaps_converted[i] = new float*[heatmap_height];
        for (int j = 0; j < heatmap_height; j++) {
            heatmaps_converted[i][j] = new float[heatmap_width];
            for (int z = 0; z < heatmap_width; z++) {
                int zoffset = z * numKeypoints;
                int joffset = j * heatmap_width * numKeypoints;
                int ioffset = i;
                heatmaps_converted[i][j][z] = buff[zoffset + ioffset + joffset];
            }
        }
    }

    float** coords;
    coords = get_max_preds(heatmaps_converted);   //ccords is of shape 17x2 but in native, this function is returning 17x3

    for (int p = 0; p < numKeypoints; p++) {
        int px = (int) floor(coords[p][0] + 0.5);
        int py = (int) floor(coords[p][1] + 0.5);
        if ((1 < px) && (px < heatmap_width - 1) && (1 < py) && (py < heatmap_height - 1)) {
            float diff[2] = {0.f};// = new float[2];

            diff[0] = heatmaps_converted[p][py][px + 1] - heatmaps_converted[p][py][px - 1];
            diff[1] = heatmaps_converted[p][py + 1][px] - heatmaps_converted[p][py - 1][px];

            if (diff[0] > 0) {
                coords[p][0] += 0.25;
            } else {
                coords[p][0] -= 0.25;
            }

            if (diff[1] > 0) {
                coords[p][1] += 0.25;
            } else {
                coords[p][1] -= 0.25;
            }
        }
        //LOGI("coords: %f %f",coords[p][0],coords[p][1]);
    }

    //Scaling
    cv::Mat scaletrans = get_affine_transform( heatmap_width, heatmap_height, 1,center, scale);

    double trans_array[2][3];

    //Mat to array
    for (int i = 0; i < scaletrans.rows; ++i) {
        for(int j=0;j<scaletrans.cols;++j)
            trans_array[i][j] = scaletrans.at<double>(i,j);
    }

    for (int p = 0; p < numKeypoints; p++) {
        for (int i = 0; i < 2; i++) {
            float temp_sum = 0;
            for (int j = 0; j < 3; j++) {
                temp_sum += coords[p][j] * (float) trans_array[i][j];
            }
            coords[p][i] = temp_sum;
        }
    }

    return coords;
}

