//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.example.ai_assistant_v1;

import android.util.Log;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;


public class CircularQueue {
    private Queue<String> queue;
    private int maxSize;

    public String TAG="Summary_queue";

    public CircularQueue(int size) {
        this.queue = new LinkedList<>();
        this.maxSize = size;
    }

    public void enqueue(String item) {
        if (queue.size() == maxSize) {
            dequeue();
        }
        queue.add(item);
    }

    public String dequeue() {
        return queue.poll();
    }

    public void display() {
        Log.d(TAG,"Queue: " + queue);
    }

    public String mergeStrings() {
        List<String> list = new ArrayList<>(queue);
        Collections.reverse(list);

        StringBuilder mergedString = new StringBuilder();
        Iterator<String> iterator = list.iterator();
        while (iterator.hasNext()) {
            mergedString.append(iterator.next());
            if (iterator.hasNext()) {
                mergedString.append(".");
            }
        }
        Log.d(TAG, "String Length:->" + mergedString.toString().length());
        return mergedString.toString();
    }


    }
