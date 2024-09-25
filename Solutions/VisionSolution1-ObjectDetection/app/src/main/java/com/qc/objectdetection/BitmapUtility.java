//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.qc.objectdetection;

import android.graphics.Bitmap;



import java.nio.ByteBuffer;

public class BitmapUtility {
    private float[] tempFloatBuffer;
    private ByteBuffer tempByteBuffer;
    private boolean mIsBufferBlack;

    /**
     * This will assume the geometry of both buffers from the first input bitmap.
     */
    public boolean checkBufferSize(ByteBuffer byteBuffer, final int inputBitmapSize){
        if (byteBuffer == null || byteBuffer.capacity() != inputBitmapSize){
            return true;
        }
        return false;
    }
    public void convertBitmapToBuffer(final Bitmap inputBitmap) {
        int inputWidth = inputBitmap.getRowBytes();
        int inputHeight = inputBitmap.getHeight();
        final int inputBitmapSize = inputWidth * inputHeight;

        if (checkBufferSize(tempByteBuffer, inputBitmapSize)){
            tempByteBuffer = ByteBuffer.allocate(inputBitmapSize);
            tempFloatBuffer = new float[1 * inputBitmap.getWidth() * inputBitmap.getHeight() * 3];
        }
        //rewind
        tempByteBuffer.rewind();
        inputBitmap.copyPixelsToBuffer(tempByteBuffer);
    }

    public long countGreenPixel(int dim, final byte[] inputArray){
        final float inputScale = 0.00784313771874f;
        long sumGreenValues = 0;
        int srcIndex = 0, dstIndex = 0;
        for (int i = 0; i < dim; i++) {
            final int tempRedPixel = inputArray[srcIndex] & 0xFF;
            tempFloatBuffer[dstIndex + 2] = inputScale * (float) tempRedPixel - 1;

            final int tempGreenPixel = inputArray[srcIndex + 1] & 0xFF;
            tempFloatBuffer[dstIndex + 1] = inputScale * (float) tempGreenPixel - 1;

            final int tempBluePixel = inputArray[srcIndex + 2] & 0xFF;
            tempFloatBuffer[dstIndex] = inputScale * (float) tempBluePixel - 1;
            sumGreenValues += tempBluePixel;
            dstIndex += 3;
            srcIndex += 4;
        }
        return sumGreenValues;
    }
    /**
     * This will process pixels RGBA(0..255) to BGR(-1..1)
     */
    public float[] bufferToFloatsBGR() {

        int dim = tempFloatBuffer.length / 3;
        final byte[] inputArray = tempByteBuffer.array();
        long sumGreenValues = countGreenPixel(dim,inputArray);
        mIsBufferBlack = sumGreenValues < (dim * 13);
        return tempFloatBuffer;
    }

    public boolean isBufferBlack() {
        return mIsBufferBlack;
    }
}
