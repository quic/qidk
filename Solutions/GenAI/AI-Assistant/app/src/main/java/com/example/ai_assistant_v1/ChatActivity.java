//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.example.ai_assistant_v1;

import static android.app.PendingIntent.getActivity;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.AnimationDrawable;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.text.SimpleDateFormat;
import java.util.Date;

public class ChatActivity extends AppCompatActivity implements Assistant.Handler {

    private RecyclerView messageRecyclerView;
    private MessageListAdapter messageListAdapter;

    private ImageButton chatInputButton,backButton,clearCacheButton;
    private EditText queryInputText;
    private ImageView loadingAnimationImage;

    private RelativeLayout layout_inputs;

    private Assistant assistant;

    private final SimpleDateFormat simpleDateFormat;
    private String date;
    private String sys_prompt;
    private TextView time_display;
    TextView charCountTextView;

    private String APP_TAG_DEBUG="ChatFragmentDebug";

    private int total_length=0;

    private final String TAG="GENIE_ChatActivity_DEBUG";

    public ChatActivity() {
        this.simpleDateFormat = new SimpleDateFormat("EEEE, MMMM dd");
    }



    public void initialize_layout(){
        //It'll initialize all the necessary functions and variables

        //Setting the date and time
        date=simpleDateFormat.format(new Date());

        time_display=findViewById(R.id.time_display);
        time_display.setText(date);

        // For User interaction
        chatInputButton = findViewById(R.id.button_input_query);
        backButton=findViewById(R.id.backButton);
        clearCacheButton=findViewById(R.id.clearcache_button);
        queryInputText = findViewById(R.id.layout_input_query);
        layout_inputs=findViewById(R.id.layout_inputs);
        charCountTextView = findViewById(R.id.char_count_text_view);
        // For loading animation
        loadingAnimationImage = (ImageView)findViewById(R.id.loading_animation_image);


        //initializing Message List Adapter
        messageListAdapter = new MessageListAdapter();
        messageListAdapter.setContext(this);
        messageRecyclerView =findViewById(R.id.recycler_view_chat);

        // Set message list adapter:
        final LinearLayoutManager linearLayoutManager = new LinearLayoutManager(this);
        linearLayoutManager.setStackFromEnd(true);

        messageRecyclerView.setLayoutManager(linearLayoutManager);
        messageRecyclerView.setAdapter(messageListAdapter);


    }

    @SuppressLint("MissingInflatedId")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_chat);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        initialize_layout();
        Intent intent = getIntent();
        sys_prompt = intent.getStringExtra("SYS_PROMPT");

        assistant = new Assistant(this,this,sys_prompt);



        queryInputText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @SuppressLint("SetTextI18n")
            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                    // Set query button to be the send button:
                    chatInputButton.setImageResource(R.drawable.send_button);
                    int charCount = s.length();
                    charCountTextView.setText(charCount + "/256");

            }

            @Override
            public void afterTextChanged(Editable s) {}
        });



        queryInputText.setOnClickListener(v -> {
            // open keyboard:
            queryInputText.requestFocus();
            ((InputMethodManager) this.getSystemService(Context.INPUT_METHOD_SERVICE))
                    .showSoftInput(queryInputText, InputMethodManager.SHOW_IMPLICIT);


        });



        clearCacheButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                reset_genie();
                Toast.makeText(ChatActivity.this,"GENIE Reset is successful",Toast.LENGTH_SHORT).show();
                Log.d(TAG,"GENIE Reset is successfull");
            }
        });
        chatInputButton.setOnClickListener(v -> {
                // If send mode, we handle the query:
                Log.d(APP_TAG_DEBUG, "Send query");
                String text = queryInputText.getText().toString();

                Log.d(APP_TAG_DEBUG,"Query:"+text);
                queryInputText.getText().clear();
                queryInputText.setEnabled(false);

                // change state to STOP:

                chatInputButton.setImageResource(R.drawable.send_button);

                // handle query:
                //Sending the query to the model
                handleQuery(text);

                queryInputText.setEnabled(true);

        });

        backButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startActivity(new Intent(ChatActivity.this, MainActivity.class));
            }
        });


    }
    @Override
    public void onQueryResponse(String text, boolean isEndOfResponse, boolean isAborted) {

        ((Activity) this).runOnUiThread(new Runnable() {
            @Override
            public void run() {
                messageListAdapter.appendToLatestMessage(text);
                messageRecyclerView.scrollToPosition(messageListAdapter.getItemCount() - 1);

                if(isEndOfResponse){
                    layout_inputs.setVisibility(View.VISIBLE);
                }
                if(isAborted){
                    messageListAdapter.appendToLatestMessage("\n\n*** \n\n Please start a new chat ");
                    messageRecyclerView.scrollToPosition(messageListAdapter.getItemCount() - 1);
                }


            }
        });

    }


    private void handleQuery(String text) {


        layout_inputs.setVisibility(View.INVISIBLE);

        // Display sent query message:
        Message queryMessage = new Message(MessageType.QUERY, text);

        Log.d(APP_TAG_DEBUG,"Query Message:"+queryMessage);
        messageListAdapter.addMessage(queryMessage);
        messageRecyclerView.scrollToPosition(messageListAdapter.getItemCount() - 1);

        // Model response:
        // Create new empty message before we start tasK:
        Message responseMessage = new Message(MessageType.RESPONSE, "");
        messageListAdapter.addMessage(responseMessage);
        messageRecyclerView.scrollToPosition(messageListAdapter.getItemCount() - 1);


        //number of conversations done till now
        //Inferencing the model
        assistant.query(text);


    }
    public native int reset_genie();


}