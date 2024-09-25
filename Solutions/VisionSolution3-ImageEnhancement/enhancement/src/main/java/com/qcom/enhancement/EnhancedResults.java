//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

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
