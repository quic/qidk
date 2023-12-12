package com.qcom.imagesuperres;

import android.graphics.Bitmap;
import static android.graphics.Color.blue;
import static android.graphics.Color.green;
import static android.graphics.Color.red;
import static android.graphics.Color.rgb;


class PrePostProcess {
    //preprocess: method to do preprocess of images
    static float[] preprocess(Bitmap[] bitmaps, ModelConstants model) {
        int channel = 3;
        int dstWidth = model.getInputWidth();//model input layer width
        int dstHeight = model.getInputHeight();//model input layer height
        float[] rgb = new float[channel * dstWidth * dstHeight * bitmaps.length];
        for (int i = 0; i < bitmaps.length; i++) {
            int rgbOffset = channel * dstWidth * dstHeight * i;
            int idx, batchIdx;
            int[] colors = new int[dstWidth * dstHeight];
            bitmaps[i].getPixels(colors, 0, dstWidth, 0, 0, dstWidth, dstHeight);
            toNormalize preproc;
            if (model.doNormaliseInput()) {//method to normalize the input
                preproc = new toNormalize() {
                    @Override
                    public float apply(double val) {
                        return (float) (val / 255.0f);
                    }
                };
                if (model.isEncodingRGB()) {// if color encoding RGB, then computing RGB value corresponding to each pixel
                    for (int y = 0; y < dstHeight; y++) {
                        for (int x = 0; x < dstWidth; x++) {
                            idx = y * dstWidth + x;
                            batchIdx = idx * 3;
                            rgb[rgbOffset + batchIdx] = preproc.apply(red(colors[idx]));
                            rgb[rgbOffset + batchIdx + 1] = preproc.apply(green(colors[idx]));
                            rgb[rgbOffset + batchIdx + 2] = preproc.apply(blue(colors[idx]));
                        }
                    }
                }
            }
        }

        return rgb;
    }

    static Bitmap[] postprocess(float[] buff,  ModelConstants model) {
        int idx, batchIdx;
        int[] colors = new int[model.getOutputWidth() * model.getOutputHeight()];
        int imageCount = buff.length / (3 * colors.length);
        Bitmap[] outputs = new Bitmap[imageCount];
        int dstHeight = model.getOutputHeight();
        int dstWidth = model.getOutputWidth();
        toNormalize preproc;
        if (model.doNormaliseInput()) {//method to get back normal values from normalize values
            preproc = new toNormalize() {
                @Override
                public float apply(double val) {
                    return Math.max(0, Math.min(255, (float) (val * 255.0)));
                }
            };
            for (int imgIdx = 0; imgIdx < imageCount; imgIdx++) {
                // Condition got out of loops to improve performance
                if (model.isEncodingRGB()) {// if color encoding RGB, then computing RGB value corresponding to each pixel
                    for (int y = 0; y < dstHeight; y++) {
                        for (int x = 0; x < dstWidth; x++) {
                            idx = y * dstWidth + x;
                            batchIdx = 3 * (idx + imgIdx * colors.length);
                            int red = (int) preproc.apply(buff[batchIdx]);
                            int green = (int) preproc.apply(buff[batchIdx + 1]);
                            int blue = (int) preproc.apply(buff[batchIdx + 2]);
                            colors[idx] = rgb(red, green, blue);
                        }
                    }
                }

                Bitmap bitmap = Bitmap.createBitmap(dstWidth, dstHeight, Bitmap.Config.ARGB_8888);
                bitmap.setPixels(colors, 0, dstWidth, 0, 0, dstWidth, dstHeight);
                outputs[imgIdx] = bitmap;
            }
        }
        return outputs;
    }

    private interface toNormalize {
        float apply(double v);
    }
}
