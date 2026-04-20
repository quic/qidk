//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.example.ai_assistant_v1;

import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import opennlp.tools.tokenize.SimpleTokenizer;

public class Summarizer {

    private static final int MAX_LENGTH = 300;
    public static String initial_result="";

    public static String updated_result="";
    public static String TAG="Summary_TAG";
    public static Map<String, Integer> tokenCount = new HashMap<>();

    public static CircularQueue queue=new CircularQueue(2);
//    private static final Set<String> STOP_WORDS = new HashSet<>(Arrays.asList(
//            "was", "of", "in", "and", "for", "one", "the", "a", "an", "to", "is", "it", "that", "on", "with", "as", "by", "at", "from", "or", "this", "but", "be", "not", "are", "have", "has", "had", "which", "were", "their", "they", "them", "its", "if", "then", "there", "so", "such", "these", "those", "can", "will", "would", "should", "could", "may", "might", "must", "shall", "do", "does", "did", "done", "been", "being", "am", "i", "you", "he", "she", "we", "us", "our", "your", "his", "her", "their", "my", "mine", "yours", "ours", "theirs", "me", "him", "her", "it", "who", "whom", "whose", "which", "what", "when", "where", "why", "how", "all", "any", "each", "every", "few", "more", "most", "some", "such", "no", "nor", "too", "very", "s", "t", "can", "will", "just", "don", "should", "now"));
    private static final Set<String> STOP_WORDS = new HashSet<>(Collections.singletonList(
        "a"));

    public static String removeFirstTwoSentences(String text) {
        // Split the text into sentences based on the period
        String[] sentences = text.split("\\.");

        // Check if there are at least two sentences
        if (sentences.length <= 2) {
            return "";
        }

        // Join the remaining sentences back into a single string
        StringBuilder result = new StringBuilder();
        for (int i = 2; i < sentences.length; i++) {
            result.append(sentences[i].trim());
            if (i < sentences.length - 1) {
                result.append(". ");
            }
        }

        return result.toString();
    }
    public static String preprocessString(String input) {

        // Removing brackets, commas, and other unnecessary symbols
        String noSymbols = input.replaceAll("[^a-zA-Z0-9:? ]", "");

        // Removing extra whitespace
        String shrunk = noSymbols.trim().replaceAll(" +", " ");

        return shrunk;
    }

    private static boolean isArabic(String text) {
        // Check if the text contains Arabic characters
        for (char c : text.toCharArray()) {
            if (Character.UnicodeBlock.of(c) == Character.UnicodeBlock.ARABIC) {
                return true;
            }
        }
        return false;
    }
    public static String summarizeResponses(String responses) {
        String initial_result=responses;
        // Checking if the respone is in Arabic0


        // Preprocess if the response is in English
        initial_result = preprocessString(responses);
        Log.d(TAG, "2.Summarizer:->intermediate_result:->" + initial_result);



        // Tokenize the combined text
        SimpleTokenizer tokenizer = SimpleTokenizer.INSTANCE;
        String[] tokens = tokenizer.tokenize(initial_result);


        //Filtering stop words
        List<String> filteredTokens = new ArrayList<>();
        for (String token : tokens) {
            if (!STOP_WORDS.contains(token.toLowerCase())) {
                filteredTokens.add(token.toLowerCase());

            }
        }


        // Combine filtered tokens into a summary
        StringBuilder summary = new StringBuilder();
        for (String token : filteredTokens) {

            if (summary.length() + token.length() + 1 <= MAX_LENGTH) {
                summary.append(token).append(" ");
            } else {
                break;
            }
        }


        String intermediate_result=summary.toString().trim();

        if(intermediate_result.length()>MAX_LENGTH){
            intermediate_result=intermediate_result.substring(0,MAX_LENGTH);
        }



        //Log.d(TAG,"2.Summarizer:->intermediate_result:->"+intermediate_result);
        //Adding to the queue, the last 3 conversation
        queue.enqueue(intermediate_result);

        //Showing the content of queue
//        queue.display();

        return queue.mergeStrings();
    }


}
