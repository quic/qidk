//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.qualcomm.qti.sentimentanalysis;

import androidx.appcompat.app.AppCompatActivity;

import android.content.res.AssetManager;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.qualcomm.qti.sentimentanalysis.databinding.ActivityMainBinding;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.qualcomm.qti.sentimentanalysis.tokenization.FullTokenizer;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'sentimentanalysis' library on application startup.
    static {
        System.loadLibrary("sentimentanalysis");
    }

    private ActivityMainBinding binding;
    private AssetManager assetManager;

    private String prevResult = "";
    private String result = "";
    private float[] input_ids = new float[MAX_SEQ_LEN];
    private float[] attn_mask = new float[MAX_SEQ_LEN];
    private static final String TAG = "SNPE_SA";
    private static final String DIC_PATH = "vocab.txt";
    private static final int MAX_SEQ_LEN = 128;
    private static final boolean DO_LOWER_CASE = true;
    private final Map<String, Integer> inputDic = new HashMap<>();
    private FullTokenizer tokenizer;

    public synchronized void loadDictionary() {
        try {
            Log.v(TAG, "==> Loading Dictionary .");
            loadDictionaryFile(this.getAssets());
            Log.v(TAG, "Dictionary loaded.");
        } catch (IOException ex) {
            Log.e(TAG, "Dictionary load exception:" + ex.getMessage());
        }
    }
    /** Load dictionary from assets. */
    public void loadDictionaryFile(AssetManager assetManager) throws IOException {
        try (InputStream ins = assetManager.open(DIC_PATH);
             BufferedReader reader = new BufferedReader(new InputStreamReader(ins))) {
            int index = 0;
            while (reader.ready()) {
                String key = reader.readLine();
                inputDic.put(key, index++);
            }
        }
    }

    public List<Integer> preProcessor(String sentence) {
        List<String> tokens = new ArrayList<>();
        // Start of generating the features.
        tokens.add("[CLS]");
        tokens = tokenizer.tokenize(sentence);
        // For ending mark.
        tokens.add("[SEP]");

        return tokenizer.convertTokensToIds(tokens);
    }

    public String formatOutput(String txtOutput, long infTime,
                               boolean isLive, String runtime, String... userInput) {
        String setOutput;
        String[] res = txtOutput.split("\\s+");
        Integer positive = Integer.parseInt(res[0]);
        Integer negative = Integer.parseInt(res[1]) + 1;
        Log.i("SNPE_INF","positive : "+positive.toString()+ "\t negative: " + negative.toString());
        Log.i(TAG, "INF time = " + infTime);

        if (isLive) {
            setOutput = "\nPositivity : "+positive.toString()+ " %  \t\t  " +
                    "Negativity: " + negative.toString() + " %" ;
            setOutput += "\n" + runtime + " Exec Time = " + infTime + "ms";
            return setOutput;
        }

        DateTimeFormatter dtf = DateTimeFormatter.ofPattern("yyyy/MM/dd HH:mm:ss");
        LocalDateTime now = LocalDateTime.now();
        result = "Result at time: "+dtf.format(now)+"\n\n" +"input: "+ Arrays.toString(userInput) +
                "\nOutput: Positivity : "+positive.toString()+ " %  \t\t  " +
                "Negativity: " + negative.toString() + " %" ;
        result += "\n" + runtime + " Exec Time = " + infTime + "ms";
        setOutput = "__________________________________________\n"+
                " SA Predicted Result \n__________________________________________\n"+
                result +"\n"+ prevResult.toString();
        return setOutput;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        String nativeDirPath = getApplicationInfo().nativeLibraryDir;
        String uiLogger = "";
        TextView tv = binding.textView;
        TextView tv_ip = binding.textViewForPrediction;
        Button predictDSP = findViewById(R.id.button);
        Button predictCPU = findViewById(R.id.button2);
        EditText inputEditText = findViewById(R.id.editTextUserInput);
        ProgressBar Pos = findViewById(R.id.progressBar );
        ProgressBar Neg = findViewById(R.id.progressBar2);

        uiLogger += queryRuntimes(nativeDirPath);
        tv.setText(uiLogger);

        // init QNN
        assetManager = getAssets();
        Log.i(TAG, "onCreate: Initializing SNPE ...");
        uiLogger += initSNPE(assetManager);
        tv.setText(uiLogger);

        loadDictionary();
        tokenizer = new FullTokenizer(inputDic, DO_LOWER_CASE);

        // Live Inference, when users type : letter by letter
        inputEditText.addTextChangedListener(new TextWatcher(){
            public void afterTextChanged(Editable s) {
                String userInput = inputEditText.getText().toString();

                List<Integer> inputIds = preProcessor(userInput);

//                Log.v(TAG, "Set inputs...");
                for (int j = 0; j < inputIds.size(); j++) {
                    input_ids[j] = inputIds.get(j);
                    attn_mask[j] = 1;
                }

                long startTime = System.currentTimeMillis();
                String output = inferSNPE("DSP", input_ids, attn_mask,
                        new int[]{input_ids.length, attn_mask.length});
                long infTime = System.currentTimeMillis() - startTime;

                String[] res = output.split("\\s+");
                Integer positive = Integer.parseInt(res[1]);
                Integer negative = Integer.parseInt(res[0]);

                String printOutput = formatOutput(output, infTime, true, "DSP");
                tv_ip.setText(printOutput);
                Pos.setProgress(positive);
                Neg.setProgress(negative);
            }
            public void beforeTextChanged(CharSequence s, int start, int count, int after){}
            public void onTextChanged(CharSequence s, int start, int before, int count){}
        });

        // On Predict DSP Button Click
        predictDSP.setOnClickListener(
                (View v) -> {
                    String userInput = inputEditText.getText().toString();

                    List<Integer> inputIds = preProcessor(userInput);
                    // List<Integer> inputMask = new ArrayList<>(Collections.nCopies(inputIds.size(), 1));

                    Log.v(TAG, "Set inputs...");
                    for (int j = 0; j < inputIds.size(); j++) {
                        input_ids[j] = inputIds.get(j);
                        attn_mask[j] = 1;
                    }
//                    Log.i(TAG, "onCreate: inp ids: " + Arrays.toString(input_ids));
//                    Log.i(TAG, "onCreate: attn mask: " + Arrays.toString(attn_mask));

                    long startTime = System.currentTimeMillis();
                    String output = inferSNPE("DSP", input_ids, attn_mask,
                            new int[]{input_ids.length, attn_mask.length});
                    long infTime = System.currentTimeMillis() - startTime;

                    prevResult = formatOutput(output, infTime, false, "DSP", userInput);
                    tv.setText(prevResult);
                });

        // On Predict CPU Button Click
        predictCPU.setOnClickListener(
                (View v) -> {
                    String userInput = inputEditText.getText().toString();

                    List<Integer> inputIds = preProcessor(userInput);

                    Log.v(TAG, "Set inputs...");
                    for (int j = 0; j < inputIds.size(); j++) {
                        input_ids[j] = inputIds.get(j);
                        attn_mask[j] = 1;
                    }

                    long startTime = System.currentTimeMillis();
                    String output = inferSNPE("CPU", input_ids, attn_mask,
                            new int[]{input_ids.length, attn_mask.length});
                    long infTime = System.currentTimeMillis() - startTime;

                    prevResult = formatOutput(output, infTime, false, "CPU", userInput);
                    tv.setText(prevResult);
                });
    }

    /**
     * A native method that is implemented by the 'sentimentanalysis' native library,
     * which is packaged with this application.
     */
    public native String queryRuntimes(String nativeDirPath);
    public native String initSNPE(AssetManager assetManager);
    public native String inferSNPE(String runtime, float[] input_ids,  float[] attn_masks, int[] arraySizes);
}
