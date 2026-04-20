//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.example.asr_llm_tts.llm_assistant;

import android.app.Activity;
import android.content.Context;
import android.util.Log;

import com.example.asr_llm_tts.utils.JsonUtils;

import java.util.Arrays;

public final class Assistant {


//    private final String LLAMA_318_CONFIG_FILE_PATH="htp-model-config-llama3-8b-gqa_ssd.json";
//    private final String LLAMA_323_CONFIG_FILE_PATH="htp-model-config-llama32-3b-gqa.json";
    private final String LLAMA_323_CONFIG_FILE_PATH="genie_config.json"; //If llm model is from AI Hub, generally config file name is genie_config.json
    private final String nativeDirPath;


    private static final String SYS_PROMPT =
            "You are a Qualcomm Voice AI Assistant \n" +
                    "Your goals:\n" +
                    "- Understand the user’s request and constraints.\n" +
                    "- Think step by step and then provide accurate, practical response strictyly within 50 words\n";


    private final String SYS_PROMPT_HEADER="<|begin_of_text|><|start_header_id|>system<|end_header_id|>\n\n";
    private final String START_USR_HEADER="<|start_header_id|>user<|end_header_id|>\n\n";
    private final String END_USR_HEADER=" <|eot_id|>";
    private final String START_ASSISTANT_HEADER="<|start_header_id|>assistant<|end_header_id|>\n\n";
    String processed_prompt="";
    private final String INFO_TAG="LLM_Summariser_INFO";
    private final String PerformanceTAG="Performance_TAG";
    private Context context;
    private Handler handler;

    static {
        System.loadLibrary("aiassistant");
    }

    public interface StringCallback {
        String handleString(String input);
    }
    public interface Handler {
        void onQueryResponse(String text, boolean isEndOfResponse, boolean isAborted);
    }
    public Assistant(Context context, String nativeDirPath, Handler handler) {
        this.context=context;
        this.nativeDirPath=nativeDirPath;
        this.handler=handler;
    }


    //Loading the Model
    public int JloadModel() {

        try{
            String config= JsonUtils.loadJSONFromAsset(context,LLAMA_323_CONFIG_FILE_PATH);
            Log.d(INFO_TAG,"Config file string: \n"+config);
            //TODO Need to handle this part also
            Log.d(INFO_TAG,"NativeDirPath: "+nativeDirPath);
            int status=load_model(config,nativeDirPath);
            Log.d(INFO_TAG,"Status of Model Loading:"+status);
            return status;
            } catch (Exception ex) {
            return -1;
            }

    }

    public void query(String text,boolean first_prompt)   {
        final long[] start = {System.currentTimeMillis()};

        final String[] temp_res = {""};

        //TODO Currently each prompt has been encapsulated with System prompt
//        processed_prompt=SYS_PROMPT_HEADER+SYS_PROMPT+START_USR_HEADER+text+END_USR_HEADER+START_ASSISTANT_HEADER;

        if(first_prompt){
            processed_prompt=SYS_PROMPT_HEADER+SYS_PROMPT+START_USR_HEADER+text+END_USR_HEADER+START_ASSISTANT_HEADER;
        }
        else{
            processed_prompt=START_USR_HEADER+text+END_USR_HEADER+START_ASSISTANT_HEADER;
        }
        Log.d(INFO_TAG,"Processed system prompt:"+processed_prompt+" Processed System prompt size: "+processed_prompt.length());

        final int[] token_count = {0};
        new Thread(new Runnable() {
            @Override
            public void run() {
                int result_code=genie_infer(processed_prompt, new StringCallback() {
                    @Override
                    public String handleString(String result) {

                        temp_res[0] +=result;
                        if(token_count[0] ==0){
                            long end = System.currentTimeMillis();
                            Log.d(PerformanceTAG, "TTFT: " + (end - start[0]) + " ms");

                            //Updating StartTime to Calculate Tokens/Sec without Prefill time
                            start[0] = System.currentTimeMillis();
                        }
                        token_count[0]++;
                        if(result.isEmpty()) {
                            long end = System.currentTimeMillis();
                            handler.onQueryResponse(temp_res[0], true, false);
                            long totalTimeMs = (end - start[0]);
                            double totalTimeSec = totalTimeMs / 1000.0;

                            //Subtracting 1 token count generated during prefill time
                            double tokensPerSec = (token_count[0]-1) / totalTimeSec;
                            Log.d(PerformanceTAG, "Total time: " + totalTimeMs + " ms");
                            Log.d(PerformanceTAG, "Total tokens: " + Arrays.toString(token_count));
                            Log.d(PerformanceTAG, "Tokens per second: " + String.format("%.2f", tokensPerSec));
                            Log.d(PerformanceTAG,"Output:"+temp_res[0]);
                            return " ";
                        }
                        ((Activity) context).runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                handler.onQueryResponse(temp_res[0], false, false);
                            }
                        });
                        return "";
                    }

                });
            }
        }).start();

    }

    public native int load_model(String jsonStr, String nativeDirPath);
    public native int genie_infer(String text,StringCallback intermediateResultCallback);

}