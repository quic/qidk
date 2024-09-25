//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.qcom.enhancement;

import java.util.List;

public class Result<E> {
    private final List<E> results;
    private final long inferenceTime;
    public Result(List<E> results, long inferenceTime) {

        this.results = results;
        this.inferenceTime = inferenceTime;
    }

    public List<E> getResults() {
        return results;
    }


    public long getInferenceTime() {
        return inferenceTime;
    }

}
