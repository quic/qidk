//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.example.ai_assistant_v1;

import android.graphics.Color;
import android.os.Build;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.BackgroundColorSpan;
import android.text.style.ForegroundColorSpan;
import android.text.style.StrikethroughSpan;

import androidx.annotation.NonNull;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.List;


public class Message {

    private final MessageType textType;
    private String text;




    Message(MessageType textType, String text) {

        this.textType = textType;
        this.text = text;
    }

    public MessageType getTextType() {
        return textType;
    }

    public String getText() {
        return text;
    }

    public void appendText(String new_text) {
            this.text += new_text;
    }

    @NonNull
    @Override
    public String toString() {
        return "Message{" +
                ", textType=" + textType +
                ", text=" + text +
                '}';
    }

}
