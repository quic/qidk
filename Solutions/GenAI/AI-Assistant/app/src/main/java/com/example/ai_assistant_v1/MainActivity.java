//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.example.ai_assistant_v1;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import com.google.android.material.floatingactionbutton.FloatingActionButton;

import java.util.HashMap;
import java.util.Map;

public class MainActivity extends AppCompatActivity {


    //This is to start the ChatActivity
    private FloatingActionButton fab;

    private Spinner modelSpinner;

    private String selectedModel;
    private String nativeDirPath;

    private TextView model_status;

    private  ProgressBar statusProgressBar;

    private EditText sysprompt_input;


    static {
        System.loadLibrary("aiassistant");
    }
    Map<String, String> modelConfigMap = new HashMap<>();

    private String TAG="GENIE_MAIN_ACTIVITY";

    @SuppressLint("MissingInflatedId")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        //nativeDirPath = this.getApplication().getApplicationInfo().nativeLibraryDir;
        nativeDirPath = "/data/local/tmp";
        Log.d(TAG,"native Dir Path: "+nativeDirPath);
        model_status=findViewById(R.id.modelStatusTextView);
        statusProgressBar=findViewById(R.id.model_loading_bar);
        // Add model names and config files to the map
        modelConfigMap.put("llama_v2_CPU", "llama2-7b-genaitransformer.json");
        modelConfigMap.put("llama_v3_0_HTP", "llama_v3_8b_chat_quantized.json");
        modelConfigMap.put("llama_v3_1_HTP", "llama_v3_1_8b_chat_quantized.json");
        modelConfigMap.put("llama_v3_2_HTP", "llama_v3_8b_chat_quantized.json");
        modelConfigMap.put("llama_v3_3_HTP", "llama_v3_2_3b_chat_quantized.json");


        fab=findViewById(R.id.fab);
        modelSpinner = findViewById(R.id.model_spinner);
        //sysprompt_input=findViewById(R.id.layout_prompt);
        //Adding the model list to the spinner
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(this,
                R.array.model_array, android.R.layout.simple_spinner_item);

        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        modelSpinner.setAdapter(adapter);
        //llama_v3_2_3b set as default
        int defaultPos = adapter.getPosition("llama_v3_3_HTP");
        modelSpinner.setSelection(defaultPos);


        // Disable the FAB
        fab.setEnabled(false);

        // Change the alpha to indicate it's disabled
        fab.setAlpha(0.5f); // Adjust the alpha value as needed

        //TODO Need to handle the error
        selectAndLoadModel();

    }

    private void selectAndLoadModel(){

        Log.d(TAG,"Model is loaded properly");
        modelSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                selectedModel = parent.getItemAtPosition(position).toString();
                Toast.makeText(MainActivity.this,"Selected Model:  "+selectedModel, Toast.LENGTH_SHORT).show();
                loadModel(selectedModel);
            }
            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                selectedModel = parent.getItemAtPosition(4).toString();
                Log.d(TAG,"Model is loaded properly" + selectedModel);
                Toast.makeText(MainActivity.this, "Selecting default model: " + selectedModel, Toast.LENGTH_SHORT).show();
                //Use Thread to do this
                loadModel(selectedModel);
            }
        });
    }
    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG,"On Resume is called");
        selectAndLoadModel();

    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        free_genie();
    }



    private void run_on_UI_thread(String message){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                // Disable the FAB

                Toast.makeText(MainActivity.this, message, Toast.LENGTH_SHORT).show();

            }
        });
    }


    private void loadModel(String selectedModel) {
        // Disable the FAB
        fab.setEnabled(false);
        statusProgressBar.setVisibility(View.VISIBLE);

        // Change the alpha to indicate it's disabled
        fab.setAlpha(0.5f); // Adjust the alpha value as needed
        new Thread(new Runnable() {
            @SuppressLint("SetTextI18n")
            @Override
            public void run() {

                try{
                    String config_file_name=modelConfigMap.get(selectedModel);

                    String config=JsonUtils.loadJSONFromAsset(MainActivity.this,config_file_name);

                    Log.d(TAG,"Config file string: "+config);

                    int status=load_model(config,nativeDirPath);
                    Log.d(TAG,"Status of Config creation:"+status);

                    if(status!=0){
                        run_on_UI_thread("Model is not present");
                        model_status.setText("Model is not present");
                    }
                    else {
                        run_on_UI_thread("Model Loaded Successfully");

                        runOnUiThread(new Runnable() {

                            @Override
                            public void run() {
                                // Disable the FAB
                                model_status.setText("Model Loaded Successfully");
                                fab.setEnabled(true);
                                fab.setAlpha(1.0f);
                                fab.setOnClickListener(new View.OnClickListener() {
                                    @Override
                                    public void onClick(View v) {
                                        //String sys_prompt = sysprompt_input.getText().toString();
                                        Intent intent=new Intent(MainActivity.this, ChatActivity.class);
                                        //intent.putExtra("SYS_PROMPT", sys_prompt);
                                        startActivity(intent);
                                    }
                                });

                            }
                        });

                    }

                } catch (Exception e) {

                    run_on_UI_thread("Error in Model loading");
                    Log.d(TAG,"Error :"+e );

                }

                statusProgressBar.setVisibility(View.INVISIBLE);

            }
        }).start();
    }


    public native int load_model(String jsonStr, String nativeDirPath);

    public native int free_genie();
}