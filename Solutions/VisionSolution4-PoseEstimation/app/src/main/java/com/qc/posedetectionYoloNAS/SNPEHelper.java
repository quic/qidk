package com.qc.posedetectionYoloNAS;

import android.app.Application;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.os.Trace;

import org.opencv.android.Utils;
import org.opencv.core.Mat;

import java.util.ArrayList;

public class SNPEHelper {
    private final Application mApplication;
    private AssetManager assetManager;
    private int NUMJOINTS = 17;

    // Constructor
    public SNPEHelper(Application application) {
        mApplication = application;
    }


    //Native functions
    public native String queryRuntimes(String a);
    public native String initSNPE(AssetManager assetManager, char a);
    public native int inferSNPE(long inputmataddress, int width,int height, float[][][] posecoords, float[][]boxcoords, String[] classname);


    /**
     * This method loads ML models on selected runtime
     */
    public boolean loadingMODELS(char runtime_var) {

        assetManager = mApplication.getAssets();
        String nativeDirPath = mApplication.getApplicationInfo().nativeLibraryDir;
        String res_query = queryRuntimes(nativeDirPath);
        System.out.println(res_query);
        String tt = initSNPE(assetManager, runtime_var);
        System.out.println("RESULT:"+tt);

        int success_count = tt.split("success", -1).length -1;

        if(success_count>=1)  //TODO to be changed for both models
        {
            System.out.println("Atleast 1 model is build");
            return true;
        }
        return false;
    }

    /*
        This method makes inference on bitmap.
    */
    public ArrayList<float[][]> snpeInference(Bitmap modelInputBitmap, int fps, ArrayList<RectangleBox> BBlist) {

        try{

        //Trace.beginSection("Java Pre Processing time");
        Mat inputMat = new Mat();
        Utils.bitmapToMat(modelInputBitmap, inputMat);



        float[][][] poseCoords = new float[100][17][2];   //Stores poses for multi person, MAXLIMIT is 100
        float[][] boxCoords = new float[100][5];  //Stores box coords for all person, MAXLIMIT is 100, last coords i.e. boxCoords[k][4] stores confidence value <-- IMP
        String[] boxnames = new String[100];
        //Trace.endSection();

        //Trace.beginSection("Java inference time");
        int numhuman = inferSNPE(inputMat.getNativeObjAddr(), modelInputBitmap.getWidth(), modelInputBitmap.getHeight(), poseCoords,boxCoords,boxnames);
        //Trace.endSection();

        //Trace.beginSection("Java post process time");
        final ArrayList<float[][]> coordlist = new ArrayList<>();

        for(int k=0;k<numhuman;k++){

            float[][] coords_array = new float[17][2];
            RectangleBox tempbox = new RectangleBox();

            for(int i=0;i<NUMJOINTS;i++)
            {
              coords_array[i][0] = poseCoords[k][i][0];
              coords_array[i][1] = poseCoords[k][i][1];
            }

            tempbox.top = boxCoords[k][0];
            tempbox.bottom = boxCoords[k][1];
            tempbox.left = boxCoords[k][2];
            tempbox.right = boxCoords[k][3];
            tempbox.fps = fps;
            tempbox.processing_time = String.valueOf(boxCoords[k][4]);
            tempbox.label = boxnames[k];

            BBlist.add(tempbox);
            coordlist.add(coords_array);
        }
        //Trace.endSection();
        return coordlist;
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

}