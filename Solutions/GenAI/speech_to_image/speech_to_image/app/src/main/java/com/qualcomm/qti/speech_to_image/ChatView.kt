//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.qualcomm.qti.speech_to_image

import android.annotation.SuppressLint
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.imePadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.Send
import androidx.compose.material.icons.filled.Clear
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material3.Button
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextFieldDefaults
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.input.TextFieldValue
import androidx.compose.ui.unit.dp
import com.qualcomm.qti.speech_to_image.R
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext


@SuppressLint("MissingPermission")
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ChatView() {
    var messageText by remember { mutableStateOf(TextFieldValue("")) }
    var messages by remember { mutableStateOf(listOf<Message>()) }

    var isGenerating by remember { mutableStateOf(false) }
    var isRecording by remember { mutableStateOf(false) }

    val coroutineScope = rememberCoroutineScope()

    var imageNum by remember { mutableStateOf(0) }
    var audioNum by remember { mutableStateOf(0) }

    var context = LocalContext.current
    var toast by remember { mutableStateOf (ToastUtil(context)) }

    val scope = CoroutineScope(Dispatchers.Default)

    var currentIcon by remember { mutableStateOf(Icons.AutoMirrored.Filled.Send) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Images generator") },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.primaryContainer,
                    titleContentColor = MaterialTheme.colorScheme.onPrimaryContainer,
                ),
                actions = {
                    IconButton(onClick = {
                        if(!isGenerating) {
                            messages = listOf()
                            imageNum = 0
                            audioNum = 0
                            clearCacheDir(getCacheDir());
                        } else {
                            toast.launch("You can't clear the chat while generating an image.");
                        }
                    }) {
                        Icon(Icons.Default.Delete, contentDescription = "Clear Chat")
                    }
                }
            )

        },
        content = { paddingValues ->
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(paddingValues)
                    .imePadding()
                    .padding(8.dp)
            ) {
                LazyColumn(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxWidth(),
                    contentPadding = PaddingValues(8.dp)
                ) {
                    items(messages) { message ->
                        message.messageView()
                    }
                }

                Row(
                    modifier = Modifier.fillMaxWidth(),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    OutlinedTextField(
                        value = messageText,
                        onValueChange = { messageText = it },
                        label = { Text("Insert Prompt:") },
                        modifier = Modifier.weight(1f),
                        singleLine = true,
                        shape = RoundedCornerShape(16.dp),
                        colors = TextFieldDefaults.outlinedTextFieldColors(
                            focusedBorderColor = DarkSecondary,
                            unfocusedBorderColor = DarkSecondary,
                            cursorColor = DarkSecondary,
                            containerColor = Color.Black,
                            focusedPlaceholderColor = Color.Gray,
                            unfocusedPlaceholderColor = Color.Gray,
                            focusedLabelColor = DarkSecondary,
                            unfocusedLabelColor = DarkSecondary
                        )
                    )

                    Spacer(modifier = Modifier.width(8.dp))

                    Button(
                        onClick = {
                                if (!isGenerating) {
                                    if (messageText.text.isNotBlank()) {
                                        val curPrompt = messageText.text
                                        messages = messages + MessageText(curPrompt)
                                        messageText = TextFieldValue("")

                                        scope.launch {
                                            currentIcon = Icons.Default.Clear
                                            isGenerating = true
                                            messages = messages + MessageLoading("")

                                            if(!SpeechToImageNativeJNI.generateImage(curPrompt, "$imageNum.png")){
                                                isGenerating = false
                                                return@launch
                                            }

                                            if(isGenerating) {
                                                withContext(Dispatchers.Main) {
                                                    messages = messages.dropLast(1)
                                                    messages = messages + MessageImage("$imageNum.png")
                                                    imageNum += 1
                                                }


                                                isGenerating = false
                                            }
                                            currentIcon = Icons.AutoMirrored.Filled.Send
                                        }
                                    }else {
                                        toast.launch("Error: The prompt cannot be blank. Please enter a valid prompt.")
                                    }
                                } else {
                                    currentIcon = Icons.AutoMirrored.Filled.Send
                                    isGenerating = false
                                    SpeechToImageNativeJNI.stopStableDiffusion();
                                    messages = messages.dropLast(1)
                                    messages = messages + MessageTranscription("Execution Interrupted")
                                }
                            }
                    ) {
                        Icon(
                            imageVector = currentIcon,
                            contentDescription = "Send",
                            tint = MaterialTheme.colorScheme.secondary,
                        )
                    }

                    Spacer(modifier = Modifier.width(8.dp))

                    Box(
                        modifier = Modifier
                            .background(
                                if (isRecording) Color.Red else MaterialTheme.colorScheme.primary,
                                shape = CircleShape
                            )
                            .padding(8.dp)
                            .pointerInput(Unit) {
                                detectTapGestures(
                                    onPress = {
                                        if (!isGenerating) {
                                            isRecording = true

                                            toast.launch("Recording started...")
                                            WAV.startRecordingWAV("$cacheDirPath/$audioNum.wav")

                                            tryAwaitRelease()
                                            if (isRecording) {
                                                WAV.stopRecordingWAV()
                                                isRecording = false
                                            }

                                            val transcription =
                                                SpeechToImageNativeJNI.speechToText("$cacheDirPath/$audioNum.wav")

                                            messages =
                                                messages + MessageAudio("$cacheDirPath/$audioNum.wav")

                                            if (transcription.isNotBlank()) {
                                                messages =
                                                    messages + MessageTranscription(transcription)

                                                audioNum += 1


                                                scope.launch {
                                                    currentIcon = Icons.Default.Clear

                                                    isGenerating = true
                                                    messages = messages + MessageLoading("")

                                                    if (!SpeechToImageNativeJNI.generateImage(
                                                            transcription,
                                                            "$imageNum.png"
                                                        )
                                                    ) {
                                                        isGenerating = false
                                                        return@launch
                                                    }


                                                    if(isGenerating) {
                                                        withContext(Dispatchers.Main) {
                                                            messages = messages.dropLast(1)
                                                            messages = messages + MessageImage("$imageNum.png")
                                                            imageNum += 1
                                                        }
                                                    }

                                                    currentIcon = Icons.AutoMirrored.Filled.Send
                                                    isGenerating = false
                                                }
                                            } else {
                                                currentIcon = Icons.AutoMirrored.Filled.Send
                                                isGenerating = false
                                                SpeechToImageNativeJNI.stopStableDiffusion();
                                                messages = messages.dropLast(1)
                                                messages = messages + MessageTranscription("Execution Interrupted")
                                            }
                                        } else {
                                            toast.launch("An image is already being generated. Please wait.");
                                        }
                                    }
                                )
                            }
                    ) {
                        Icon(
                            painter = painterResource(id = R.drawable.mic),
                            contentDescription = "Microphone",
                            tint = MaterialTheme.colorScheme.secondary,
                            modifier = Modifier.size(24.dp)
                        )
                    }
                }
            }
        }
    )
}