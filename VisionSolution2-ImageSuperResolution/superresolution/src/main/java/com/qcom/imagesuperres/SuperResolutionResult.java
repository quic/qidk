package com.qcom.imagesuperres;

import android.graphics.Bitmap;

public class SuperResolutionResult {
    private final Bitmap[] highResolutionImages;

    public SuperResolutionResult(Bitmap[] highResolutionImages) {
        this.highResolutionImages = highResolutionImages;
    }

    public Bitmap[] getHighResolutionImages() {
        return highResolutionImages;
    }
}
