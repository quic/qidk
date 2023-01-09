package com.qc.objectdetection;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.support.annotation.Nullable;
import android.util.ArrayMap;
import android.util.AttributeSet;
import android.view.View;


import java.util.ArrayList;
import java.util.Map;
import java.util.concurrent.locks.ReentrantLock;

/**
 * FragmentRender class is utility for making boxes on camera frames.
 * FragmentRender has utility in fragment_camera.xml and CameraFragment Class
 */
public class FragmentRender extends View {
    private ReentrantLock mLock = new ReentrantLock();
    private ArrayList<RectangleBox> mBoxes = new ArrayList<>();
    private final Map<Integer, Integer> mColorIndexMapping = new ArrayMap<>();
    private float mConfidenceThreshold = 0.4f;
    private Paint mCenterColor = new Paint();
    private Paint mTextColor = new Paint();
    private Paint mBorderColor = new Paint();

    public FragmentRender(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public void makeBoxes(ArrayList<RectangleBox> newBoxes) {
        mLock.lock();
        mBorderColor.setColor(Color.TRANSPARENT);
        postInvalidate();
        if (newBoxes == null) {
            for (RectangleBox rbox : mBoxes) {
                rbox.confidence = 0;
            }
        }
        else {
            for (int i = 0; i < newBoxes.size(); i++) {
                if (i >= mBoxes.size()) {
                    mBoxes.add(new RectangleBox());
                }
                newBoxes.get(i).createCopy(mBoxes.get(i));
            }
        }
        mLock.unlock();
        postInvalidate();
    }


    private void init() {
        mBorderColor.setStyle(Paint.Style.STROKE);
        mBorderColor.setStrokeWidth(8);
        mCenterColor.setColor(Color.BLUE);
        mTextColor.setStyle(Paint.Style.FILL);
        mTextColor.setTextSize(50);
        mTextColor.setColor(Color.WHITE);
    }

    private int createColor(int index) {
        if (!mColorIndexMapping.containsKey(index)) {
            float[] hsv = {(float) (Math.random() * 360), (float) (0.5 + Math.random() * 0.5), (float) (0.5 + Math.random() * 0.5)};
            mColorIndexMapping.put(index, Color.HSVToColor(hsv));
        }
        return mColorIndexMapping.get(index);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        mLock.lock();
        for (int i = 0; i < mBoxes.size(); i++) {
            final RectangleBox rbox = mBoxes.get(i);
            if (rbox.confidence < mConfidenceThreshold)
                break;
            if (i ==0){
                String textLabel;
                textLabel="Processing Time : "+rbox.processing_time+"ms";
                canvas.drawText(textLabel, 10, 35, mTextColor);
                textLabel="FPS : "+rbox.fps;
                canvas.drawText(textLabel, 10, 70, mTextColor);
            }
        computeCoordinate(rbox, canvas);
        }
        mLock.unlock();
    }

    private void computeCoordinate(RectangleBox rbox, Canvas canvas) {

        float y = getHeight() * rbox.left;
        float y1 = getHeight() * rbox.right;
        float x = getWidth() * rbox.top;
        float x1 = getWidth() * rbox.bottom;

        String textLabel;
        if (rbox.label != null && !rbox.label.isEmpty())
            textLabel = rbox.label;
        else
            textLabel = String.valueOf(rbox.label_index + 2);
        canvas.drawText(textLabel, x + 10, y + 30, mTextColor);
        mBorderColor.setColor(createColor(rbox.label_index));
        canvas.drawRect(x, y, x1, y1, mBorderColor);
    }

}
