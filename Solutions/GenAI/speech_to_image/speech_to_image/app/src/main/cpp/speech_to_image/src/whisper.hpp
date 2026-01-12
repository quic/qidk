//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#pragma once

#include <stddef.h>
#include <string>

#define START_TOKEN 50258
#define END_TOKEN 50257
#define MAX_ITERATIONS 1000

constexpr size_t N_MODELS = 2;

namespace models {
    enum {
        ENCODER = 3,
        DECODER = 4
    };
}

namespace encoder_input {
    enum{
        AUDIO
    };
}

namespace encoder_output {
    enum{
        K_CACHE_CROSS_0,
        V_CACHE_CROSS_0,
        K_CACHE_CROSS_1,
        V_CACHE_CROSS_1,
        K_CACHE_CROSS_2,
        V_CACHE_CROSS_2,
        K_CACHE_CROSS_3,
        V_CACHE_CROSS_3,
    };
}

namespace decoder_input {
    enum{
        INPUT_IDS,
        POSITION_IDS,
        K_CACHE_SELF_0,
        V_CACHE_SELF_0,
        ATTENTION_MASK,
        K_CACHE_CROSS_0,
        V_CACHE_CROSS_0,
        K_CACHE_SELF_1,
        V_CACHE_SELF_1,
        K_CACHE_CROSS_1,
        V_CACHE_CROSS_1,
        K_CACHE_SELF_2,
        V_CACHE_SELF_2,
        K_CACHE_CROSS_2,
        V_CACHE_CROSS_2,
        K_CACHE_SELF_3,
        V_CACHE_SELF_3,
        K_CACHE_CROSS_3,
        V_CACHE_CROSS_3,
    };
}

namespace decoder_output {
    enum{
        LOGITS,
        K_CACHE_SELF_0,
        V_CACHE_SELF_0,
        K_CACHE_SELF_1,
        V_CACHE_SELF_1,
        K_CACHE_SELF_2,
        V_CACHE_SELF_2,
        K_CACHE_SELF_3,
        V_CACHE_SELF_3,
    };
}

static void* sg_backendHandle{nullptr};

