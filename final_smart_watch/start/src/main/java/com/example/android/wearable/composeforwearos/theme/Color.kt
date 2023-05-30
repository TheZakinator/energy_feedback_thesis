/*
 * Copyright 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.example.android.wearable.composeforwearos.theme

import androidx.compose.ui.graphics.Color
import androidx.wear.compose.material.Colors

//val Purple200 = Color(0xFFEE7D1A)
val Purple200 = Color(0xFF002688)
val Purple500 = Color(0xFF6200EE)
val Purple700 = Color(0xFF3700B3)
val Teal200 = Color(0xFFFCB156)
val Red400 = Color(0xFFCF6679)

val statusOn = Color(0xFF31B800)
val statusStandby = Color(0xFF0055FF)
val statusDisabled = Color(0xFF535353)
val statusOff = Color(0xFFE00000)

val powerMeterColor = Color(0xFF31B800)

//val grey = Color(0xFF535353)
val grey = Color(0xFFE9E5E5)
val btBlue = Color(0xFF0082FC)

val WearAppColorPalette: Colors = Colors(
    primary = Purple200,
    primaryVariant = Purple700,
    secondary = Teal200,
    secondaryVariant = Teal200,
    error = Red400,
    onPrimary = grey,
    onSecondary = btBlue,
    onError = Color.Black

)
