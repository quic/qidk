//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.qualcomm.qti.speech_to_image

import android.graphics.BitmapFactory
import android.media.MediaPlayer
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectTransformGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import java.io.File
import java.io.IOException
import androidx.compose.ui.text.font.FontStyle
import com.qualcomm.qti.speech_to_image.R

interface Message {
    val content: String

    @Composable
    fun messageView()
}

class MessageText(override val content: String) : Message {
    @Composable
    override fun messageView(){
        val alignment = Alignment.End
        val backgroundColor =  MaterialTheme.colorScheme.primary

        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(8.dp),
            horizontalAlignment = alignment
        ) {
            Surface(
                shape = MaterialTheme.shapes.medium,
                shadowElevation = 1.dp,
                color = backgroundColor
            ) {
                Text(
                    text = content,
                    fontSize = 16.sp,
                    color = Color.White,
                    modifier = Modifier.padding(12.dp)
                )
            }
        }
    }
}

class MessageTranscription(override val content: String) : Message {
    @Composable
    override fun messageView(){
        val alignment = Alignment.End
        val backgroundColor =  MaterialTheme.colorScheme.primary

        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(8.dp),
            horizontalAlignment = alignment
        ) {
            Surface(
                shape = MaterialTheme.shapes.medium,
                shadowElevation = 1.dp,
                color = backgroundColor
            ) {
                Text(
                    text = content,
                    fontSize = 16.sp,
                    fontStyle = FontStyle.Italic,
                    color = Color.White,
                    modifier = Modifier.padding(12.dp)
                )
            }
        }
    }
}

class MessageImage(override val content: String): Message {
    @Composable
    override fun messageView(){
        val alignment = Alignment.Start
        var showFullSizeImage by remember { mutableStateOf(false) }

        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(8.dp),
            horizontalAlignment = alignment
        ) {
            Surface(
                shape = MaterialTheme.shapes.medium,
                shadowElevation = 1.dp,
                modifier = Modifier.clickable { showFullSizeImage = true }
            ) {
                DisplayImageView(content)
            }

            if (showFullSizeImage) {
                DisplayImageFullSizeDialog(imagePath = content) {
                    showFullSizeImage = false
                }
            }
        }
    }

    @Composable
    private fun DisplayImageView(imagePath: String){
        val imageFile = File(cacheDirPath, imagePath)
        // Load the image as a Bitmap
        val bitmap = remember {
            if (imageFile.exists()) {
                BitmapFactory.decodeFile(imageFile.absolutePath)
            } else {
                null
            }
        }

        // Visualizza l'immagine se il bitmap non è nullo
        bitmap?.let {
            Image(
                bitmap = it.asImageBitmap(),
                contentDescription = stringResource(id = R.string.generated_image_description)
            )
        }
    }

    @Composable
    private fun DisplayImageFullSizeDialog(imagePath: String, onClose: () -> Unit) {
        Dialog(onDismissRequest = onClose) {
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .background(Color.Black)
            ) {
                val imageFile = File(cacheDirPath, imagePath)
                val bitmap = remember {
                    if (imageFile.exists()) {
                        BitmapFactory.decodeFile(imageFile.absolutePath)
                    } else {
                        null
                    }
                }

                var scale by remember { mutableStateOf(1f) }
                var offset by remember { mutableStateOf(Offset.Zero) }

                bitmap?.let {
                    Image(
                        bitmap = it.asImageBitmap(),
                        contentDescription = stringResource(id = R.string.generated_image_description),
                        modifier = Modifier
                            .fillMaxSize()
                            .padding(16.dp)
                            .graphicsLayer(
                                scaleX = scale,
                                scaleY = scale,
                                translationX = offset.x,
                                translationY = offset.y
                            )
                            .pointerInput(Unit) {
                                detectTransformGestures { _, pan, zoom, _ ->
                                    scale *= zoom
                                    offset += pan
                                }
                            },
                        contentScale = ContentScale.Fit // or ContentScale.Inside
                    )
                }

                IconButton(
                    onClick = onClose,
                    modifier = Modifier
                        .align(Alignment.TopEnd)
                        .padding(16.dp)
                ) {
                    Icon(
                        imageVector = Icons.Default.Close,
                        contentDescription = "Close",
                        tint = Color.White
                    )
                }
            }
        }
    }
}


class MessageAudio(override val content: String): Message {
    @Composable
    override fun messageView() {
        var isPlaying by remember { mutableStateOf(false) }
        val mediaPlayer = remember { MediaPlayer() }

        LaunchedEffect(isPlaying) {
            if (isPlaying) {
                try {
                    mediaPlayer.reset()
                    mediaPlayer.setDataSource(content)
                    mediaPlayer.prepare()
                    mediaPlayer.start()
                    mediaPlayer.setOnCompletionListener {
                        isPlaying = false
                    }
                } catch (e: IOException) {
                    e.printStackTrace()
                }
            } else {
                if (mediaPlayer.isPlaying) {
                    mediaPlayer.stop()
                }
            }
        }

        DisposableEffect(Unit) {
            onDispose {
                mediaPlayer.release()
            }
        }

        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(8.dp),
            horizontalAlignment = Alignment.End
        ) {
            Surface(
                shape = MaterialTheme.shapes.medium,
                shadowElevation = 1.dp,
                color = MaterialTheme.colorScheme.primary
            ) {
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    modifier = Modifier
                        .padding(8.dp),
                ) {
                    Text(
                        text = if (isPlaying) "Stop" else "Audio",
                        color = MaterialTheme.colorScheme.onPrimary,
                        style = MaterialTheme.typography.bodyLarge
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                    IconButton(onClick = { isPlaying = !isPlaying }) {
                        Icon(
                            imageVector = if (isPlaying) Icons.Default.Close else Icons.Default.PlayArrow,
                            contentDescription = if (isPlaying) "Stop" else "Play",
                            tint = MaterialTheme.colorScheme.onPrimary
                        )
                    }
                }
            }
        }

    }
}


class MessageLoading(override val content: String) : Message {
    @Composable
    override fun messageView() {
        Box(
            contentAlignment = Alignment.BottomStart,
            modifier = Modifier
                .padding(top = 75.dp, start = 8.dp, end = 8.dp, bottom = 75.dp)
                .fillMaxWidth(0.5f)
        ) {
            LinearProgressIndicator()
        }
    }
}