package com.qc.posedetectionYoloNAS;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.view.View;


import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.concurrent.locks.ReentrantLock;

/**
 * FragmentRender class is utility for making boxes on camera frames.
 * FragmentRender has utility in fragment_camera.xml and CameraFragment Class
 */
public class FragmentRender extends View {

    private ReentrantLock mLock = new ReentrantLock();
    private ArrayList<float[][]> coordsList = new ArrayList<>();
    private ArrayList<RectangleBox> boxlist = new ArrayList<>();

    private Paint mTextColor = new Paint();
    private Paint mBorderColor = new Paint();
    private Paint mPosepaint = new Paint();
    int[][] Connections = {{1,3},{1,0},{2,4},{2,0},{0,5},{0,6},{5,7},{7,9},{6,8},{8,10},{5,11},{6,12},{11,12},{11,13},{13,15},{12,14},{14,16}};

    public FragmentRender(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        init();
    }


    //Initiate coordsList and boxList from model inferences
    public void setCoordsList(ArrayList<float[][]> newcoordslist, ArrayList<RectangleBox> t_boxlist) {
        mLock.lock();
        postInvalidate();

        if (newcoordslist==null)
        {
            mLock.unlock();
            return;
        }
        coordsList.clear();
        boxlist.clear();
        for(int j=0;j<newcoordslist.size();j++) {
            System.out.println("writing coordList in java");
                coordsList.add(newcoordslist.get(j));
                boxlist.add(t_boxlist.get(j));
        }
        mLock.unlock();
        postInvalidate();
    }

    private void init() {
        mBorderColor.setColor(Color.TRANSPARENT);
        mBorderColor.setColor(Color.MAGENTA);
        mBorderColor.setStyle(Paint.Style.STROKE);
        mBorderColor.setStrokeWidth(6);
        mTextColor.setStyle(Paint.Style.FILL);
        mTextColor.setTypeface(Typeface.DEFAULT_BOLD);
        mTextColor.setTextSize(50);
        mTextColor.setColor(Color.RED);
        mPosepaint= new Paint(Paint.ANTI_ALIAS_FLAG);
        mPosepaint.setStrokeWidth(8);
    }

    @Override
    protected void onDraw(Canvas canvas) {

        mLock.lock();
        int[] color_arr = {Color.GREEN, Color.BLUE, Color.RED, Color.YELLOW, Color.BLACK};
        //System.out.println("COORD LOST SIZE:    "+coordsList.size());
        //System.out.println("BOX LIST SIZE:    "+boxlist.size());
        for(int j=0;j<coordsList.size();j++) {
            mPosepaint.setColor(color_arr[(j%color_arr.length)]);
            float[][] coords = coordsList.get(j);
            RectangleBox rbox = boxlist.get(j);

            float y = rbox.left;
            float y1 = rbox.right;
            float x =  rbox.top;
            float x1 = rbox.bottom;

            String fps_textLabel = "FPS: "+String.valueOf(rbox.fps);
            canvas.drawText(fps_textLabel,10,70,mTextColor);

            String processingTimeTextLabel= rbox.processing_time+"ms";


            canvas.drawRect(x1, y, x, y1, mBorderColor);
            canvas.drawText(processingTimeTextLabel,x1+10, y+30, mTextColor);

            for (int i = 0; i < Connections.length; i++) {
                int kpt_a = Connections[i][0];
                int kpt_b = Connections[i][1];
                int x_a = (int) coords[kpt_a][0];
                int y_a = (int) coords[kpt_a][1];
                int x_b = (int) coords[kpt_b][0];
                int y_b = (int) coords[kpt_b][1];
                if ((x_a | y_a) != 0) {
                    canvas.drawCircle(x_a, y_a, 8, mPosepaint);
                    if ((x_b | y_b) != 0) {
                        //System.out.println("COORDS: to draw "+coords[kpt_a][0]+" "+coords[kpt_a][1]+" "+coords[kpt_b][0]+" "+coords[kpt_b][1]);
                        canvas.drawCircle(x_b, y_b, 8, mPosepaint);
                        canvas.drawLine(x_a, y_a, x_b, y_b, mPosepaint);
                    }
                }
            }
        }
        mLock.unlock();
    }
}
