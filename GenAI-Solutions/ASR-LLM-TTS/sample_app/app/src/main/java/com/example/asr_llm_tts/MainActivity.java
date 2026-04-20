//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.example.asr_llm_tts;

import android.Manifest;
import android.content.pm.PackageManager;
import android.graphics.Typeface;
import android.os.Build;
import android.os.Bundle;
import android.text.Editable;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.text.style.StyleSpan;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.EdgeToEdge;
import androidx.activity.SystemBarStyle;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.example.asr_llm_tts.asr.utils.TranscriptionListener;
import com.example.asr_llm_tts.asr.Whisper_ASR;
import com.example.asr_llm_tts.llm_assistant.Assistant;
import com.example.asr_llm_tts.tts_manager.TTSManager;
import com.google.android.material.progressindicator.CircularProgressIndicator;
import com.qualcomm.qti.voice.assist.tts.sdk.TTS;
import com.qualcomm.qti.voice.assist.whisper.sdk.Whisper;
import com.qualcomm.qti.voice.assist.whisper.sdk.WhisperResponseListener;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;

import com.google.android.material.floatingactionbutton.ExtendedFloatingActionButton;


public class MainActivity extends AppCompatActivity implements  Assistant.Handler, TranscriptionListener {
    private static final String DEBUG_TAG = "ASR_LLM_TTS_MAIN_DEBUG";
    private static final String INFO_TAG = "ASR_LLM_TTS_MAIN_INFO";
    private final String Performance_TAG="ASR_LLM_TTS_MAIN_Performance";
    private static final String MODEL_DIR = "/storage/emulated/0/Android/data/com.example.tts/files/tts/";
    private EditText mTTSInputEditText;

    private TextView llm_output_text;
    private String mLLMTextINP;
    private String mCurrentLanguage;
    private int mLanguagePosition;
    private final float mTTSSpeechingRate = 1.0f;
    private final float mTTSPitch = 0f;
    private final float mTTSVolumeGain = 0f;
    private final int mTTSSampleRate = 44100;
    ExtendedFloatingActionButton extend_btn;
    ExtendedFloatingActionButton fabReset,fabReload,summariser_btn,clearAll_btn;

    CircularProgressIndicator llm_loading;

    private Assistant assistant;
    private  String nativeDirPath;
    private static int llm_chat_count=0;
    private final int mTTSAudioEncoding = TTS.AUDIO_ENCODING.LINEAR16.ordinal(); //"LINEAR16"

    private boolean llm_loaded=false;
    private boolean isLlmResponseActive=false;
    private TTSManager mTTSManager;
    private boolean fabMenuOpen = true;
    private boolean mRecording;
    private String mLastLanguage = null;

    private String mTranscription = "";

    private Whisper_ASR whisper_asr;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);



        EdgeToEdge.enable(this,
                SystemBarStyle.dark(getResources().getColor(R.color.toolbar_background_color, getTheme())));
        setContentView(R.layout.activity_main);

        whisper_asr = new Whisper_ASR(getApplicationContext(), MainActivity.this);
        getWindow().setNavigationBarColor(getColor(R.color.activity_background_color));
        initUI();
        isModelFileExisting();
    }

    //-------------------------------------------------------------------- Utils & Helper functions------------------------------------------------------------------
    private void run_toast_on_ui_thread(String text){

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(MainActivity.this, text, Toast.LENGTH_LONG).show();
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(INFO_TAG, "onDestroy");
        mTTSManager.stop();
        mTTSManager.release();
        free_genie();
    }
    @Override
    protected void onStop() {
        super.onStop();
        Log.d(INFO_TAG, "onStop");
        mTTSManager.stop();
    }
    @Override
    protected void onStart() {
        super.onStart();
        Log.d(INFO_TAG, "onStart");
        initTTSEngine();
        reset_genie();
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(INFO_TAG,"onPause");
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        Log.d(INFO_TAG,"onRestart");
    }

    // --------------------------------------------------------------------- Initializing UI & Models -----------------------------------------------------------------
    private void initUI() {

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        nativeDirPath= this.getApplication().getApplicationInfo().nativeLibraryDir;
        mTTSInputEditText = findViewById(R.id.tts_input_editor);
        assistant=new Assistant(this, nativeDirPath,this);
        llm_loading=findViewById(R.id.loading_indicator);
        llm_output_text=findViewById(R.id.llm_output);
        mTTSInputEditText.setHint("Ask me anything — type or use voice");

        //Initializing and Setting up the Floating Action Button
        setupFabs();

        File externalDir = getExternalFilesDir(null);
        Log.d("PATH","External Dir:"+externalDir);
        File myFile = new File(externalDir, "example.txt");
        try (FileWriter writer = new FileWriter(myFile)) {
            writer.write("Hello, LLM!");
            Log.d(DEBUG_TAG, "File saved at: " + myFile.getAbsolutePath());
        } catch (IOException e) {
            Log.e(DEBUG_TAG, "Error writing file: " + e.getMessage(), e);
        }



        mTTSInputEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {

            }
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }
            @Override
            public void afterTextChanged(Editable s) {
                mLLMTextINP = String.valueOf(s);
            }
        });
        mCurrentLanguage = getResources().getStringArray(R.array.languages)[0];
        //Loading TTS Engine

    }

    private void openFabMenu(ExtendedFloatingActionButton fabMain,
                             ExtendedFloatingActionButton fabReset,
                             ExtendedFloatingActionButton fabReload,
                             ExtendedFloatingActionButton summariser_btn,
                             ExtendedFloatingActionButton clearAll_btn) {
        fabReset.setVisibility(View.VISIBLE);
        fabReload.setVisibility(View.VISIBLE);
        summariser_btn.setVisibility(View.VISIBLE);
        clearAll_btn.setVisibility(View.VISIBLE);

        // Animate up and fade in
        fabReset.animate().alpha(1f).translationY(-15f).setDuration(180).start();
        fabReload.animate().alpha(1f).translationY(-15f).setDuration(200).start();
        summariser_btn.animate().alpha(1f).translationY(-10f).setDuration(200).start();
        clearAll_btn.animate().alpha(1f).translationY(-15f).setDuration(200).start();

        fabMain.extend(); // show label on E-FAB
        fabMain.setIconResource(R.drawable.ic_cross);
        fabMenuOpen = true;
    }

    private void closeFabMenu(ExtendedFloatingActionButton fabMain,
                              ExtendedFloatingActionButton fabReset,
                              ExtendedFloatingActionButton fabReload,
                              ExtendedFloatingActionButton summariser_btn,
                              ExtendedFloatingActionButton clearAll_btn) {
        fabReset.animate().alpha(0f).translationY(0f).setDuration(160)
                .withEndAction(() -> fabReset.setVisibility(View.GONE)).start();

        fabReload.animate().alpha(0f).translationY(0f).setDuration(160)
                .withEndAction(() -> fabReload.setVisibility(View.GONE)).start();

        summariser_btn.animate().alpha(0f).translationY(0f).setDuration(160)
                .withEndAction(() -> summariser_btn.setVisibility(View.GONE)).start();

        clearAll_btn.animate().alpha(0f).translationY(0f).setDuration(160)
                .withEndAction(() -> clearAll_btn.setVisibility(View.GONE)).start();

        fabMain.shrink(); // hide label on E-FAB
        fabMain.setIconResource(R.drawable.baseline_add_24);
        fabMenuOpen = false;
    }

    private void setupFabs() {
        extend_btn      =findViewById(R.id.fab_main);
        summariser_btn  = findViewById(R.id.summariser_btn);
        fabReset        = findViewById(R.id.fab_secondary_1);
        fabReload       = findViewById(R.id.load_mdl_btn);
        clearAll_btn    = findViewById(R.id.clear_all_btn);

        // Start hidden and reset position
        fabReset.setVisibility(View.GONE);
        fabReload.setVisibility(View.VISIBLE);
        summariser_btn.setVisibility(View.VISIBLE);
        clearAll_btn.setVisibility(View.VISIBLE);

        fabReset.setAlpha(0f);
        fabReload.setAlpha(1f);
        summariser_btn.setAlpha(1f);
        clearAll_btn.setAlpha(1f);

        fabReset.setTranslationY(0f);
        fabReload.setTranslationY(0f);
        summariser_btn.setTranslationY(0f);
        clearAll_btn.setTranslationY(0f);

        extend_btn.setOnClickListener(v -> {
            if (fabMenuOpen) {
                closeFabMenu(extend_btn, fabReset, fabReload,summariser_btn,clearAll_btn);
            } else {
                openFabMenu(extend_btn, fabReset, fabReload,summariser_btn,clearAll_btn);
            }
        });


        //TODO-- Need to deactivate the buttons when it's transcribing
        clearAll_btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mRecording) {
                    Log.d(DEBUG_TAG, "stop Recording manually");
                    whisper_asr.stopRecording();
                    mRecording = false;
                    clearAll_btn.setText("Start Recording");

                } else {
                    Log.i(INFO_TAG, "Clearing previous text");
                    runOnUiThread(() -> mTTSInputEditText.getText().clear());
                    whisper_asr.startRecording();
                    mRecording = true;

                    clearAll_btn.setText("Stop Recording");
                }
            }

        });

        summariser_btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

                //Disable all the buttons
                runOnUiThread(() -> {
                    llm_loading.setVisibility(View.VISIBLE);
                    summariser_btn.setEnabled(false); // disable button while loading\
                    fabReload.setEnabled(false);
                    fabReset.setEnabled(false);
                    clearAll_btn.setEnabled(false);
                });
                summarise();
            }
        });

        fabReset.setOnClickListener(v -> {

            Toast.makeText(this, "Resetting LLM & UI", Toast.LENGTH_SHORT).show();

            mTTSInputEditText.setText("");
            llm_output_text.setText("");
            llm_loading.setVisibility(View.VISIBLE);
            summariser_btn.setEnabled(false); // disable button while loading\
            fabReload.setEnabled(false);
            fabReset.setEnabled(false);
            clearAll_btn.setEnabled(false);

            if(reset_genie()==0){
                Toast.makeText(this, "Resetting is successful", Toast.LENGTH_SHORT).show();
            }
            else{
                Toast.makeText(this, "Resetting is unsuccessful", Toast.LENGTH_SHORT).show();
            }

            //Enabling all the buttons after resetting
            llm_loading.setVisibility(View.GONE);
            summariser_btn.setEnabled(true); // disable button while loading\
            fabReload.setEnabled(true);
            fabReset.setEnabled(true);
            clearAll_btn.setEnabled(true);



        });

        fabReload.setOnClickListener(v -> {

            //Disabling all the buttons
            runOnUiThread(() -> {
                llm_loading.setVisibility(View.VISIBLE);
                summariser_btn.setEnabled(false); // disable button while loading\
                fabReload.setEnabled(false);
                fabReset.setEnabled(false);
                clearAll_btn.setEnabled(false);
            });

            Toast.makeText(this, "loading models", Toast.LENGTH_SHORT).show();
            load_llm_summariser();
        });
    }

    //------------------------------------------------------------ AI assistant model loading & inferencing---------------------------------------------------

    //Loading the Model
    private void load_llm_summariser(){
        //Running this on a separate thread
        new Thread(new Runnable() {
            @Override
            public void run() {
                int status = assistant.JloadModel();
                llm_loaded = (status == 0);
                if (llm_loaded) {
                    run_toast_on_ui_thread("LLM loaded successfully");
                    Log.d(INFO_TAG, "LLM loaded successfully");
                }
                // Load Whisper after LLM completes (sequential DSP access)
                whisper_asr.initializeWhisper();
                run_toast_on_ui_thread("Whisper loaded");
                
                initTTSEngine();
                Log.d(INFO_TAG, "load_llm_summariser: TTS done");
                run_toast_on_ui_thread("TTS loaded");

                if (llm_loaded) {
                    runOnUiThread(() -> {
                        summariser_btn.setEnabled(true);
                        fabReset.setEnabled(true);
                        clearAll_btn.setEnabled(true);
                    });

                } else {
                    run_toast_on_ui_thread("Model load failed, Try again!!!");
                    Log.e(INFO_TAG, "LLM failed to load. Status: " + status);
                }

                //Enabling reload button
                runOnUiThread(() -> {
                    llm_loading.setVisibility(View.GONE);
                    fabReload.setEnabled(true);
                });
            }
        }).start();
    }

    //Summarisation
    private void summarise(){
        //It's running on separate thread
        if(llm_loaded && !isLlmResponseActive){
            if (mLLMTextINP == null || mLLMTextINP.trim().isEmpty()) {
                run_toast_on_ui_thread("Add some Text to summarise!!");
                Log.d(INFO_TAG,"Add some Text to summarise!!");

                //If there is no text to summarise then enable the buttons
                runOnUiThread(() -> {
                    llm_loading.setVisibility(View.GONE);
                    summariser_btn.setEnabled(true);
                    fabReload.setEnabled(true);
                    fabReset.setEnabled(true);
                    clearAll_btn.setEnabled(true);
                });
                return;
            }
            runOnUiThread(() -> {
                llm_loading.setVisibility(View.VISIBLE);
            });
            llm_chat_count+=1;
            boolean isFirstPrompt=llm_chat_count==1;
            assistant.query(mLLMTextINP,isFirstPrompt);
        }
        else{
            run_toast_on_ui_thread("Load the model first!!");
            Log.d(INFO_TAG,"Load the model first!!");

        }
    }


    //LLM Response callback function
    @Override
    public void onQueryResponse(String text, boolean isEndOfResponse, boolean isAborted) {
        isLlmResponseActive=true;
        try {
            // Ensuring UI updates happen on main thread
            runOnUiThread(() -> {
                if (llm_output_text != null) {
                    llm_loading.setVisibility(View.GONE);
                    llm_output_text.setVisibility(View.VISIBLE);
                    // Handle null or empty text
                    llm_output_text.setText(text != null ? text : "");
                    if (isEndOfResponse) {
                        try {
                            if (text != null && !text.trim().isEmpty()) {
                                Toast.makeText(MainActivity.this,"Summarisation completed, Now TTS starting",Toast.LENGTH_SHORT).show();
                                play_audio(text);
                                isLlmResponseActive=false;
                            } else {
                                Log.w(INFO_TAG, "Skipped play(): text is null or empty");
                            }
                        } catch (Exception e) {
                            Log.e(DEBUG_TAG, "Error during play(): " + e.getMessage(), e);
                        }
                        Log.d(DEBUG_TAG, "isEndOfResponse: " + isEndOfResponse);
                    }
                } else {
                    Log.e(DEBUG_TAG, "llm_output_text is null. Cannot update UI.");
                }
            });
        } catch (Exception e) {
            Log.e(DEBUG_TAG, "Unexpected error in UI update: " + e.getMessage(), e);
        }
    }
    public native int free_genie();
    public native int reset_genie();


    //---------------------------------------------------------------------- TTS Model Loading & Inference ---------------------------------------------------------

    private void initTTSEngine() {
        if (mTTSManager == null) {
            mTTSManager = new TTSManager(this);
        }
        mTTSManager.deInit();
        updateTTSConfig(mCurrentLanguage, mLanguagePosition);
        mTTSManager.init(MODEL_DIR);
    }
    private void isModelFileExisting() {
        boolean existing = false;
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                existing = Files.list(Paths.get(MODEL_DIR)).findAny().isPresent();
            }

        } catch (IOException e) {
            Log.e(DEBUG_TAG, "Check model file error ", e);
        }
        if (!existing) {
            Toast.makeText(this, "please push model file to \""
                    + MODEL_DIR + "\" folder", Toast.LENGTH_LONG).show();
        }
    }

    private void updateTTSConfig(String lang, int languagePosition) {
        String[] modelFiles = getResources().getStringArray(R.array.language_models);
        String modelFile = MODEL_DIR + modelFiles[languagePosition];

        mTTSManager.updateTTSConfig(lang, modelFile,
                String.valueOf(mTTSAudioEncoding),
                String.valueOf(mTTSSpeechingRate),
                String.valueOf(mTTSPitch),
                String.valueOf(mTTSVolumeGain),
                String.valueOf(mTTSSampleRate));
    }
    // TTS audio play from Text
    void play_audio(String text) {
        if (TextUtils.isEmpty(text)) {
            Toast.makeText(this, "Text input should not be empty", Toast.LENGTH_SHORT).show();
            return;
        }
        // Stop any active Whisper DSP inference before starting TTS
        whisper_asr.stopRecording();
        Log.d(INFO_TAG, "play_audio: Whisper stopped, starting TTS");
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                long startTime = System.currentTimeMillis(); // Start time

                mTTSManager.speak(text, () -> {
                    long endTime = System.currentTimeMillis(); // End time
                    long durationMs = endTime - startTime;     // Duration in ms
                    double durationSec = durationMs / 1000.0;  // Convert to seconds

                    Log.i(INFO_TAG, "on play completed");
                    Log.i(Performance_TAG, "TTS play duration: " + durationMs + " ms (" + String.format("%.2f", durationSec) + " sec)");

                    runOnUiThread(() -> {
                        summariser_btn.setEnabled(true);
                        fabReload.setEnabled(true);
                        fabReset.setEnabled(true);
                        clearAll_btn.setEnabled(true);
                    });
                });
            }
        });
    }
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        Log.i(INFO_TAG, "onRequestPermissionsResult");
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }
    @Override
    public void onResume() {
        super.onResume();

        if (!allPermissionsGranted()) {
            ActivityCompat.requestPermissions(this, permissions, 0);
        }
    }
    private final String[] permissions = {
            (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU ?
                    android.Manifest.permission.READ_MEDIA_AUDIO : Manifest.permission.READ_EXTERNAL_STORAGE),
            Manifest.permission.RECORD_AUDIO};
    private boolean allPermissionsGranted() {
        for (String permission : permissions) {
            if (ContextCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }
    //----------------------------------------------------Whisper Callback functions to show results---------------------------------------------------
    @Override
    public void onTranscriptionFinal(String fullText, String language) {
        if (mLastLanguage == null || !language.equals(mLastLanguage)) {
//            mTranscription += System.lineSeparator() + "[" + language + "]: ";
            mLastLanguage = language;
        }
        // Save all finalized transcriptions
        mTranscription = fullText + System.lineSeparator();
        // Show on screen
        mTTSInputEditText.setText(mTranscription);
        mTTSInputEditText.setVisibility(View.VISIBLE);
        mRecording = false;
        clearAll_btn.setText("Start Recording");
    }

    @Override
    public void onTranscriptionPartial(String partialText, String language) {
        SpannableString partial = new SpannableString(partialText);
        partial.setSpan(new StyleSpan(Typeface.ITALIC), 0, partialText.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        mTTSInputEditText.setText(mTranscription);
        mTTSInputEditText.append(partial);
    }

    @Override
    public void onPointerCaptureChanged(boolean hasCapture) {
        super.onPointerCaptureChanged(hasCapture);
    }
}