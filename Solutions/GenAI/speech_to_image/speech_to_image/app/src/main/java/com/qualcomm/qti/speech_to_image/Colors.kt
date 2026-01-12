//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.qualcomm.qti.speech_to_image

import androidx.compose.ui.graphics.Color
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable

val DarkPrimary = Color(0xFF1F1F1F)
val DarkPrimaryVariant = Color(0xFF121212)
val DarkSecondary = Color(0xFFBB86FC)
val DarkBackground = Color(0xFF121212)
val DarkSurface = Color(0xFF1F1F1F)
val DarkError = Color(0xFFCF6679)
val DarkOnPrimary = Color(0xFFFFFFFF)
val DarkOnSecondary = Color(0xFF000000)
val DarkOnBackground = Color(0xFFFFFFFF)
val DarkOnSurface = Color(0xFFFFFFFF)
val DarkOnError = Color(0xFF000000)

private val DarkColors = darkColorScheme(
    primary = DarkPrimary,
    onPrimary = DarkOnPrimary,
    primaryContainer = DarkPrimaryVariant,
    onPrimaryContainer = DarkOnPrimary,
    secondary = DarkSecondary,
    onSecondary = DarkOnSecondary,
    background = DarkBackground,
    onBackground = DarkOnBackground,
    surface = DarkSurface,
    onSurface = DarkOnSurface,
    error = DarkError,
    onError = DarkOnError
)

@Composable
fun DarkTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = DarkColors,
        typography = MaterialTheme.typography,
        shapes = MaterialTheme.shapes,
        content = content
    )
}