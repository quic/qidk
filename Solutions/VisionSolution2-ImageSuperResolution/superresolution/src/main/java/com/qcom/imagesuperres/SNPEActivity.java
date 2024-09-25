//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.qcom.imagesuperres;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import java.io.IOException;
import java.io.InputStream;

public class SNPEActivity extends AppCompatActivity {
    public static final String SNPE_MODEL_NAME = "SuperResolution_sesr"; //DLC model file name
    public static InputStream originalFile = null;
    SuperResolution superResolution;
    //creating objects for UI element used in layout files (activity_snpe.xml)
    RadioButton rb1, rb2, rb3;
    TextView txt4;
    ImageView imageView, imageView2;
    RadioGroup radioGroup;
    Bitmap bmps = null;
    public static Result<SuperResolutionResult> result = null;
    Spinner spin;
    String[] options = {"No Selection","Sample1.jpg","Sample2.jpg"}; //Image filenames on which model inference is made
    protected void executeRadioButton(int checkedId) {
        switch (checkedId) {
            case R.id.rb1:
                // set text for your textview here
                System.out.println("CPU instance running");
                result = process(bmps, "CPU");
                txt4.setText("CPU inference time : " + result.getInferenceTime() + "milli sec");
                imageView2.setImageBitmap(result.getResults().get(0).getHighResolutionImages()[0]);
                break;
            case R.id.rb2:
                // set text for your textview here
                System.out.println("GPU instance running");
                result = process(bmps, "GPU");
                txt4.setText("GPU inference time : " + result.getInferenceTime() + "milli sec");
                imageView2.setImageBitmap(result.getResults().get(0).getHighResolutionImages()[0]);
                break;
            case R.id.rb3:
                System.out.println("DSP instance running");
                System.out.println("Device runtime " + "DSP");
                result = process(bmps, "DSP");
                txt4.setText("DSP inference time : " + result.getInferenceTime() + "milli sec");
                imageView2.setImageBitmap(result.getResults().get(0).getHighResolutionImages()[0]);
                break;
            default:
                System.out.println("Do Nothing");
        }
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_snpe);
        rb1 = (RadioButton) findViewById(R.id.rb1);
        rb2 = (RadioButton) findViewById(R.id.rb2);
        rb3 = (RadioButton) findViewById(R.id.rb3);
        txt4 = (TextView) findViewById(R.id.textView4);
        imageView = (ImageView) findViewById(R.id.im1);
        imageView2 = (ImageView) findViewById(R.id.im2);
        radioGroup = (RadioGroup) findViewById(R.id.rg1);
        spin = (Spinner) findViewById((R.id.spinner));

        ArrayAdapter ad = new ArrayAdapter(this, android.R.layout.simple_spinner_item, options);
        ad.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spin.setAdapter(ad);

        spin.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
//                radioGroup.clearCheck();
                Toast.makeText(getApplicationContext(), options[position], Toast.LENGTH_SHORT).show();
                // loading picture from assets...
                if (!parent.getItemAtPosition(position).equals("No Selection")) {//if no selection of image
                    imageView2.setImageResource(R.drawable.ic_launcher_background);
                    txt4.setText("Stats");
                    try {
                        originalFile = getAssets().open((String) parent.getItemAtPosition(position));
                    } catch (IOException e) {
                        e.printStackTrace();
                    }

                    // Convert input image to Bitmap
                    bmps = BitmapFactory.decodeStream(originalFile);
                    Bitmap scaled1 = Bitmap.createScaledBitmap(bmps, 128, 128, true);
                    try {
                        // Set the input image in UI view
                        imageView.setImageBitmap(scaled1);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    int checkedID_RB = radioGroup.getCheckedRadioButtonId();
                    if (originalFile!=null && bmps!=null && checkedID_RB !=-1){
                        executeRadioButton(checkedID_RB);
                    }
                    // Lister to check the change in HW accelerator input in APP UI
                    radioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
                        @Override
                        public void onCheckedChanged(RadioGroup group, int checkedId) {
                            if (originalFile!=null && bmps!=null){
                                executeRadioButton(checkedId);
                            }
                            else{
                                Toast.makeText(getApplicationContext(), "Please select image first", Toast.LENGTH_SHORT).show();
                                System.out.println("Tag2");
                            }
                        }
                    });
                }
                else{
                    originalFile=null;
                    bmps=null;
                    imageView.setImageResource(R.drawable.ic_launcher_background);
                    imageView2.setImageResource(R.drawable.ic_launcher_background);
                    txt4.setText("Stats");
                    radioGroup.clearCheck();
                    System.out.println("Tag3");
                    Toast.makeText(getApplicationContext(), "Please select image first", Toast.LENGTH_SHORT).show();
                }
            }
            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                System.out.println("Nothing");
            }
        });
    }

    public Result<SuperResolutionResult> process(Bitmap bmps, String run_time){

        Result<SuperResolutionResult> result = null;
        superResolution = new SuperResolution();
        // initializingModel : method to initiate SNPE model into desired runtime
        boolean superresInited = superResolution.initializingModel(this, SNPE_MODEL_NAME, run_time);
        // sizeOperation can take values 0,1 and 2. It is used to set input image size equal to model input layer size.
        int sizeOperation = 1;
        result = superResolution.process(new Bitmap[] {bmps}, sizeOperation);
        superResolution.freeNetwork();
        return result;
    }
}

