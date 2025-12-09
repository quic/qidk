package com.example.ai_assistant_v1;

//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================


import android.app.Activity;
import android.content.Context;
import android.util.Log;

public class Assistant {



    private Context context;
    private Handler handler;


    private static String[] intermediate_result;

    private static int total_length=0;

    private String sys_prompt;

    private int chat_count=0;

    private final String TAG="GENIE_Assistant_DEBUG";






    static {
        System.loadLibrary("aiassistant");
    }
    public Assistant(Context context, Handler handler, String sys_prompt) {
        this.context=context;
        this.handler=handler;
        intermediate_result= new String[]{""};
        total_length=0;
        chat_count=0;
        this.sys_prompt = (sys_prompt == null || sys_prompt.isEmpty()) ? "You are an AI assistant designed to help users with a wide range of tasks. Your primary goals are to provide accurate information, assist with various tasks, and engage in meaningful conversations." : sys_prompt;


    }


    public interface StringCallback {
        String handleString(String input);

    }



    public void query(String text)   {

        final String[] local_result = {""};
        chat_count++;
        String query_prefix="";
        Log.d(TAG,"Chat Count: "+ chat_count);
        if(chat_count<3){

            query_prefix="<|begin_of_text|><|start_header_id|>user<|end_header_id|> Instruction: "+this.sys_prompt+ " {Question}: "+ text;
            Log.d(TAG,"Query Prefix: "+query_prefix);

        }
        else{
            chat_count=0;
            query_prefix="<|begin_of_text|><|start_header_id|>user<|end_header_id|> Instruction: "+this.sys_prompt+ " {Question}: "+ text +"  \n\n{Context}:"+intermediate_result[0];
            Log.d(TAG,"Query prefix c=3: "+query_prefix);
        }


        String query_suffix="<|eot_id|><|start_header_id|>assistant<|end_header_id|>";


        final int[] running_total = {0};

        String processed_prompt=query_prefix+query_suffix;
        Log.d(TAG,"Processed system prompt:"+processed_prompt+" Processed System prompt size: "+processed_prompt.length());

        new Thread(new Runnable() {
            @Override
            public void run() {
                int result_code=genie_infer(processed_prompt,chat_count, new StringCallback() {
                    @Override
                    public String handleString(String result) {

                        local_result[0] +=result;

                        running_total[0] =local_result[0].length()+total_length;

//                        Log.d(TAG,"Running Total: "+running_total[0]);

                        final String l_res=result;
                        if(result.isEmpty()) {
                            handler.onQueryResponse(l_res, true, false);

                            intermediate_result[0] =Summarizer.summarizeResponses(" ,"+local_result[0]);
                            Log.d(TAG,"Updated Intermediate Result: "+intermediate_result[0]);

                            //Debug Purpose
                            total_length+=local_result[0].length();
                            Log.d(TAG,"Total Length of the message: "+total_length);
                            return " ";
                        }


                        ((Activity) context).runOnUiThread(new Runnable() {
                            @Override
                            public void run() {

                                    handler.onQueryResponse(l_res, false, false);
                            }
                        });



                        return "";
                    }



                });
            }
        }).start();



    }
    public interface Handler {
        void onQueryResponse(String text, boolean isEndOfResponse, boolean isAborted);
    }

    public native int genie_infer(String text,int chat_count,StringCallback intermediateResultCallback);
    public native int reset_genie();

}


