package com.qcom.enhancement;

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
    public static final String ENHANCEMENT_SNPE_MODEL_NAME = "enlight_axisQ_cached";
    public static InputStream originalFile = null;
    private static final String TAG = EnhancOps.class.getSimpleName();
    EnhancOps enhancOps;
    RadioButton rb1, rb2, rb3;
    TextView txt4;
    ImageView imageView, imageView2;
    RadioGroup radioGroup;
    Bitmap bmps = null;
    public static Result<EnhancedResults> result = null;
    Spinner spin;
    String[] options = {"No Selection","Sample1.jpg","Sample2.jpg"};
    protected void executeRadioButton(int checkedId) {
        switch (checkedId) {
            case R.id.rb1:
                // set text for your textview here
                System.out.println("CPU instance running");
                result = process(bmps, "CPU");
                txt4.setText("CPU inference time : " + result.getInferenceTime() + "milli sec");
                imageView2.setImageBitmap(result.getResults().get(0).getEnhancedImages()[0]);
                break;
            case R.id.rb2:
                // set text for your textview here
                System.out.println("GPU instance running");
                result = process(bmps, "GPU_FLOAT16");
                txt4.setText("GPU inference time : " + result.getInferenceTime() + "milli sec");
                imageView2.setImageBitmap(result.getResults().get(0).getEnhancedImages()[0]);
                break;
            case R.id.rb3:
                System.out.println("DSP instance running");
                System.out.println("Device runtime " + "DSP");
                result = process(bmps, "DSP");
                txt4.setText("DSP inference time : " + result.getInferenceTime() + "milli sec");
                imageView2.setImageBitmap(result.getResults().get(0).getEnhancedImages()[0]);
                break;
            default:
                System.out.println("Do Nothing");
        }
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_s_n_p_e);
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
                    bmps = BitmapFactory.decodeStream(originalFile);
                    Bitmap scaled1 = Bitmap.createScaledBitmap(bmps, bmps.getWidth(), bmps.getHeight(), true);
                    try {//preview bitmap to image view...
                        imageView.setImageBitmap(scaled1);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    int checkedID_RB = radioGroup.getCheckedRadioButtonId();
                    if (originalFile!=null && bmps!=null && checkedID_RB !=-1){
                        executeRadioButton(checkedID_RB);
                    }
                    radioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
                        @Override
                        public void onCheckedChanged(RadioGroup group, int checkedId) {
                            if (originalFile!=null && bmps!=null){
                                executeRadioButton(checkedId);
                            }
                            else{
                                Toast.makeText(getApplicationContext(), "Please select image first", Toast.LENGTH_SHORT).show();
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
                    Toast.makeText(getApplicationContext(), "Please select image first", Toast.LENGTH_SHORT).show();
                }
            }
            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                System.out.println("Nothing");
            }
        });
    }

//==================================
    public Result<EnhancedResults> process(Bitmap bmps, String run_time){

        Result<EnhancedResults> result = null;
        enhancOps = new EnhancOps();
        //model is initialised in below fun. Model congif been set as per SNPE.
        boolean enhancementInited = enhancOps.initializingModel(this, ENHANCEMENT_SNPE_MODEL_NAME, run_time);
        int sizeOperation = 2;
        result = enhancOps.process(new Bitmap[] {bmps}, sizeOperation);
        enhancOps.freeNetwork();
        return result;
    }
}

