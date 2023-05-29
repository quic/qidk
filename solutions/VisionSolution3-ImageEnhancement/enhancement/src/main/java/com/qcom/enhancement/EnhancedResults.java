
package com.qcom.enhancement;

import android.graphics.Bitmap;

public class EnhancedResults {
    private final Bitmap[] enhancedImages;
    public EnhancedResults(Bitmap[] enhancedImages) {
        this.enhancedImages = enhancedImages;
    }
    public Bitmap[] getEnhancedImages() {
        return enhancedImages;
    }
}
