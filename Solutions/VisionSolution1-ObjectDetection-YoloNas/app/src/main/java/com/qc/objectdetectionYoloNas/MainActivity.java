package com.qc.objectdetectionYoloNas;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.support.v4.app.FragmentTransaction;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.WindowManager;
import android.widget.RadioGroup;

import org.opencv.android.OpenCVLoader;

/**
 * MainActivity class helps choose runtime from UI through main_activity.xml
 * Passes choice of runtime to CameraFragment for making inference using selected runtime.
 */
public class MainActivity extends AppCompatActivity {

    static {
       System.loadLibrary("objectdetectionYoloNas");
    }

    public static char runtime_var;  //TODO change here as well as main_activity.xml, change checked "android:checked="true""
    RadioGroup rg;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_activity);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        OpenCVLoader.initDebug();
        rg = (RadioGroup) findViewById(R.id.rg1);
        rg.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                switch (checkedId) {
                    case R.id.CPU:
                        runtime_var = 'C';
                        overToCamera(runtime_var);
                        System.out.println("CPU instance running");
                        break;
                    case R.id.GPU:
                        runtime_var = 'G';
                        overToCamera(runtime_var);
                        System.out.println("GPU instance running");
                        break;
                    case R.id.DSP:
                        runtime_var = 'D';
                        overToCamera(runtime_var);
                        System.out.println("DSP instance running");
                        break;
                    default:
                        runtime_var = 'N';
                        overToCamera(runtime_var);
                        System.out.println("Do Nothing");
                }
            }
        });

    }

    /**
     * Method to request Camera permission
     */
    private void cameraPermission() {
        requestPermissions(new String[]{Manifest.permission.CAMERA}, 1);
    }

    /**
     * Method to navigate to CameraFragment along with runtime choice
     */
    private void overToCamera(char runtime_value) {
        Boolean passToFragment;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            passToFragment = MainActivity.this.checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED;
        }
        else{
            passToFragment = true;
        }
        if (passToFragment) {
            FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
            Bundle args = new Bundle();
            args.putChar("key", runtime_value);
            transaction.add(R.id.main_content, CameraFragment.create(args));
            transaction.commit();
        } else {
            cameraPermission();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        overToCamera(runtime_var);
    }

    @Override
    protected void onStop() {
        super.onStop();
    }
}