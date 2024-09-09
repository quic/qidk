#include <jni.h>
#include <string>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <cstdio>
#include <android/log.h>

#include "whisper.h"
#include "input_features.h"

#include "inference.h"
#include <fstream>



const char *vocab_filename = "filters_vocab_gen.bin";
AAsset *asset_vocab;

//extern "C" JNIEXPORT jstring JNICALL
//Java_com_example_automatic_1speech_1recognition_MainActivity_queryRuntimes(
//        JNIEnv* env,
//        jobject /* this */,
//        jstring native_dir_path) {
//    const char *cstr = env->GetStringUTFChars(native_dir_path, nullptr);
//    env->ReleaseStringUTFChars(native_dir_path, cstr);
//
//    std::string runT_Status;
//    std::string nativeLibPath = std::string(cstr);
//
////    runT_Status += "\nLibs Path : " + nativeLibPath + "\n";
//
//    if (!SetAdspLibraryPath(nativeLibPath)) {
//        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "Failed to set ADSP Library Path\n");
//
//        runT_Status += "\nFailed to set ADSP Library Path\nTerminating";
//        return env->NewStringUTF(runT_Status.c_str());
//    }
//
//    // ====================================================================================== //
//    runT_Status = "Querying Runtimes : \n\n";
//    // DSP unsignedPD check
//    if (!zdl::SNPE::SNPEFactory::isRuntimeAvailable(zdl::DlSystem::Runtime_t::DSP,zdl::DlSystem::RuntimeCheckOption_t::UNSIGNEDPD_CHECK)) {
//        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "UnsignedPD DSP runtime : Absent\n");
//        runT_Status += "UnsignedPD DSP runtime : Absent\n";
//    }
//    else {
//        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "UnsignedPD DSP runtime : Present\n");
//        runT_Status += "UnsignedPD DSP runtime : Present\n";
//    }
//    // DSP signedPD check
//    if (!zdl::SNPE::SNPEFactory::isRuntimeAvailable(zdl::DlSystem::Runtime_t::DSP)) {
//        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "DSP runtime : Absent\n");
//        runT_Status += "DSP runtime : Absent\n";
//    }
//    else {
//        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "DSP runtime : Present\n");
//        runT_Status += "DSP runtime : Present\n";
//    }
//    // GPU check
//    if (!zdl::SNPE::SNPEFactory::isRuntimeAvailable(zdl::DlSystem::Runtime_t::GPU)) {
//        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "GPU runtime : Absent\n");
//        runT_Status += "GPU runtime : Absent\n";
//    }
//    else {
//        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "GPU runtime : Present\n");
//        runT_Status += "GPU runtime : Present\n";
//    }
//    // CPU check
//    if (!zdl::SNPE::SNPEFactory::isRuntimeAvailable(zdl::DlSystem::Runtime_t::CPU)) {
//        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "CPU runtime : Absent\n");
//        runT_Status += "CPU runtime : Absent\n";
//    }
//    else {
//        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "CPU runtime : Present\n");
//        runT_Status += "CPU runtime : Present\n";
//    }
//
//    return env->NewStringUTF(runT_Status.c_str());
//}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_automatic_1speech_1recognition_MainActivity_loadModelJNI(JNIEnv *env, jobject thiz,
                                                                          jobject assetManager,
                                                                          jint is_recorded,
                                                                          jstring fileName,
                                                                          jfloatArray input_features,
                                                                          jfloatArray encoder_hidden_state,
                                                                          jstring runtime) {
    //Load Whisper Model into buffer
    const char* pcmfilename = env->GetStringUTFChars(fileName, 0);
    jstring result = NULL;
    //WAV input
    //Preprocessing is taken from https://github.com/usefulsensors/openai-whisper
    std::vector<float> pcmf32;
    {
        drwav wav;
        size_t audio_dataSize=0;
        char* audio_buffer = nullptr;

        if(is_recorded) {
            if (!drwav_init_file(&wav,
                                 pcmfilename,
                                 NULL)) {
                __android_log_print(ANDROID_LOG_VERBOSE, "ASRHelper",
                                    "failed to open WAV file '%s' - check your input\n",
                                    pcmfilename);
                return result;
            }
        }
        else {
            if (!(env->IsSameObject(assetManager, NULL))) {
                AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
                AAsset *asset = AAssetManager_open(mgr, pcmfilename, AASSET_MODE_UNKNOWN);
                assert(asset != nullptr);

                audio_dataSize = AAsset_getLength(asset);
                audio_buffer = (char *) malloc(sizeof(char) * audio_dataSize);
                LOGI("ASRHelper: audio Size = %d\n",audio_dataSize);
                AAsset_read(asset, audio_buffer, audio_dataSize);
                AAsset_close(asset);
                LOGI("ASRHelper: audio  = %c \n",audio_buffer[0]);
            }

            if (!drwav_init_memory(&wav, audio_buffer, audio_dataSize,NULL)) {
                __android_log_print(ANDROID_LOG_VERBOSE, "ASR", "failed to open WAV file '%s' - check your input\n", pcmfilename);
                return result;
            }
        }
        if (wav.channels != 1 && wav.channels != 2) {
            __android_log_print(ANDROID_LOG_VERBOSE, "ASR", "WAV file '%s' must be mono or stereo\n", pcmfilename);

            return result;
        }

        if (wav.sampleRate != WHISPER_SAMPLE_RATE) {
            __android_log_print(ANDROID_LOG_VERBOSE, "ASR", "WWAV file '%s' must be 16 kHz\n", pcmfilename);
            return result;
        }

        if (wav.bitsPerSample != 16) {
            __android_log_print(ANDROID_LOG_VERBOSE, "ASR", "WAV file '%s' must be 16-bit\n", pcmfilename);
            return result;
        }

        int n = wav.totalPCMFrameCount;

        std::vector<int16_t> pcm16;
        pcm16.resize(n*wav.channels);
        drwav_read_pcm_frames_s16(&wav, n, pcm16.data());
        drwav_uninit(&wav);
        // convert to mono, float
        pcmf32.resize(n);
        if (wav.channels == 1) {
            for (int i = 0; i < n; i++) {
                pcmf32[i] = float(pcm16[i])/32768.0f;
            }
        } else {
            for (int i = 0; i < n; i++) {
                pcmf32[i] = float(pcm16[2*i] + pcm16[2*i + 1])/65536.0f;
            }
        }
    }

    //Hack if the audio file size is less than 30ms append with 0's
    pcmf32.resize((WHISPER_SAMPLE_RATE*WHISPER_CHUNK_SIZE),0);
    const auto processor_count = std::thread::hardware_concurrency();
    __android_log_print(ANDROID_LOG_VERBOSE, "Whisper ASR", "\ncpu_cores%d\n",processor_count);
    if (!log_mel_spectrogram(pcmf32.data(), pcmf32.size(), WHISPER_SAMPLE_RATE, WHISPER_N_FFT, WHISPER_HOP_LENGTH, WHISPER_N_MEL, processor_count,filters, mel)) {
        fprintf(stderr, "%s: failed to compute mel spectrogram\n", __func__);
        return result;
    }
    __android_log_print(ANDROID_LOG_VERBOSE, "Whisper ASR", "\nmel.n_len%d\n",mel.n_len);
    __android_log_print(ANDROID_LOG_VERBOSE, "Whisper ASR", "\nmel.n_mel:%d\n",mel.n_mel);






    std::vector<std::vector<std::vector<float>>> temp_array(1,std::vector<std::vector<float>>(80,std::vector<float>(3000)));

    int index=0;
    for(int d=0;d<1;d++){
        for(int r=0;r<80;r++){
            for(int c=0;c<3000;c++){
                temp_array[d][r][c]=mel.data[index++];
            }
        }
    }

    jfloat * inp_id_array;
    jfloat * logit_array;
    inp_id_array = env->GetFloatArrayElements(input_features, NULL);
    logit_array=env->GetFloatArrayElements(encoder_hidden_state,NULL);


    std::vector<float *> inputVec {inp_id_array};
    std::vector<float *> outputVec {logit_array};
    index=0;
    for(int d=0;d<1;d++){
        for(int r=0;r<3000;r++){
            for(int c=0;c<80;c++){
                mel.data[index++]=temp_array[d][c][r];
                inputVec.push_back(&mel.data[index-1]);
                //LOGI("ASRHelper native-lib_preprost: %d -> %f",index,inputVec[0][index]);
            }
        }
    }

    if(!inputVec.empty()){
        inputVec.erase(inputVec.begin());
    }

    const char *accl_name = env->GetStringUTFChars(runtime, NULL);
    env->ReleaseStringUTFChars(runtime, accl_name);
    std::string backend = std::string(accl_name);
    std::string return_msg = execute_net(inputVec,240000,outputVec, backend);

    for ( int index = 0; index <576000; index++ ) {
        logit_array[index] = outputVec[0][index];
        //LOGI("ASRHelper native-lib_prepost_logit: %d -> %f",index,logit_array[index]);
    }

    return env->NewStringUTF(return_msg.c_str());
}
extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_automatic_1speech_1recognition_MainActivity_InitSnpe(JNIEnv *env, jobject thiz,
                                                                      jobject assets, jstring native_dir_path) {
    // TODO: implement InitSnpe()
    std::string result;
    AAssetManager* mgr = AAssetManager_fromJava(env, assets);

    //Changes

    const char *cstr = env->GetStringUTFChars(native_dir_path, nullptr);
    env->ReleaseStringUTFChars(native_dir_path, cstr);
    std::string nativeLibPath = std::string(cstr);

    SetAdspLibraryPath(nativeLibPath);
        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "Failed to set ADSP Library Path\n");


    AAsset* asset = AAssetManager_open(mgr, "whisper_tiny_encoder_w8a16.dlc", AASSET_MODE_UNKNOWN);

    if (NULL == asset) {
        LOGE("ASRHelper: Failed to load ASSET, needed to load DLC\n");
    }
    long dlc_size = AAsset_getLength(asset);
    LOGI("ASRHelper: DLC Size = %ld MB\n", dlc_size / (1024*1024));

    char* dlc_buffer = (char*) malloc(sizeof(char) * dlc_size);
    AAsset_read(asset, dlc_buffer, dlc_size);


    //return build_network(reinterpret_cast<const uint8_t *>(dlc_buffer), dlc_size);
    result += build_network(reinterpret_cast<const uint8_t *>(dlc_buffer), dlc_size);

    return env->NewStringUTF(result.c_str());
}
extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_automatic_1speech_1recognition_MainActivity_LoadVocab(JNIEnv *env, jobject thiz,
                                                                       jobject assets) {
    //Loading the Vocab Asset
    std::string result;

    AAssetManager* mgr = AAssetManager_fromJava(env, assets);
    asset_vocab = AAssetManager_open(mgr, vocab_filename, AASSET_MODE_UNKNOWN);
    //Checking if the asset is loaded properly or not
    assert(asset_vocab != nullptr);
    uint32_t magic = 0;
    //Reading the asset
    AAsset_read(asset_vocab, &magic, sizeof(magic));
    //@magic:USEN
    //Error Checking
    if (magic != 0x5553454e) {
        // printf("%s: invalid vocab file '%s' (bad magic)\n", __func__, fname.c_str());
        __android_log_print(ANDROID_LOG_VERBOSE, "Whisper ASR",
                            "%s: invalid vocab file '%s' (bad magic)\n", __func__,
                            vocab_filename);
        return (jstring) "";
    }
    // load mel filters
    {
        AAsset_read(asset_vocab, (char *) &filters.n_mel, sizeof(filters.n_mel));
        AAsset_read(asset_vocab, (char *) &filters.n_fft, sizeof(filters.n_fft));
        __android_log_print(ANDROID_LOG_VERBOSE, "Whisper ASR", "%s: n_mel:%d n_fft:%d\n",
                            __func__,filters.n_mel, filters.n_fft);
        filters.data.resize(filters.n_mel * filters.n_fft);

        AAsset_read(asset_vocab, (char *) filters.data.data(), filters.data.size() * sizeof(float));
    }


    int32_t n_vocab = 0;
    std::string word;
    // load vocab
    {
        AAsset_read(asset_vocab, (char *) &n_vocab, sizeof(n_vocab));
        g_vocab.n_vocab = n_vocab;
        __android_log_print(ANDROID_LOG_VERBOSE, "Whisper ASR", "\nn_vocab:%d\n",
                            (int) n_vocab);

        for (int i = 0; i < n_vocab; i++) {
            uint32_t len;
            AAsset_read(asset_vocab, (char *) &len, sizeof(len));

            word.resize(len);
            AAsset_read(asset_vocab, (char *) word.data(), len);
            g_vocab.id_to_token[i] = word;
            printf("Whisper ASR len:%d",(int)len);
            printf("Whisper ASR '%s'\n", g_vocab.id_to_token[i].c_str());
        }

        g_vocab.n_vocab = 51864;//add additional vocab ids
        if (g_vocab.is_multilingual()) {
            g_vocab.token_eot++;
            g_vocab.token_sot++;
            g_vocab.token_prev++;
            g_vocab.token_solm++;
            g_vocab.token_not++;
            g_vocab.token_beg++;
        }
        for (int i = n_vocab; i < g_vocab.n_vocab; i++) {
            if (i > g_vocab.token_beg) {
                word = "[_TT_" + std::to_string(i - g_vocab.token_beg) + "]";
            } else if (i == g_vocab.token_eot) {
                word = "[_EOT_]";
            } else if (i == g_vocab.token_sot) {
                word = "[_SOT_]";
            } else if (i == g_vocab.token_prev) {
                word = "[_PREV_]";
            } else if (i == g_vocab.token_not) {
                word = "[_NOT_]";
            } else if (i == g_vocab.token_beg) {
                word = "[_BEG_]";
            } else {
                word = "[_extra_token_" + std::to_string(i) + "]";
            }
            g_vocab.id_to_token[i] = word;
            printf("%s: g_vocab[%d] = '%s'\n", __func__, i, word.c_str());
        }
    }
    AAsset_close(asset_vocab);
    result += "Successfull";

    return env->NewStringUTF(result.c_str());
}
extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_automatic_1speech_1recognition_MainActivity_Transcribe(JNIEnv *env, jobject thiz,
                                                                        jobject assets,
                                                                        jlongArray logits_,
                                                                        jint length) {
    // Transcribtion Function
    jlong * logits;

    logits= env->GetLongArrayElements(logits_, NULL);
    std::string text = "";
    std::string word_add;
    int i=0;
    while(true) {
        int token=static_cast<int>(logits[i++]);
        if(token==50257 or i>=length){
            break;
        }

        text += whisper_token_to_str(token);
    }


    return env->NewStringUTF(text.c_str());
}