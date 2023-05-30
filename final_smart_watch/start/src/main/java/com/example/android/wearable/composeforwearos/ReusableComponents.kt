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
package com.example.android.wearable.composeforwearos

import android.app.Activity
import android.bluetooth.BluetoothAdapter
import android.content.Intent
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.rounded.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.wear.compose.material.*
import com.example.android.wearable.composeforwearos.theme.*


@Composable
fun ButtonExample(
    modifier: Modifier = Modifier,
    iconModifier: Modifier = Modifier,
) {
    var bAdapter : BluetoothAdapter
    var image = Icons.Rounded.BluetoothDisabled
    bAdapter = BluetoothAdapter.getDefaultAdapter()

    image = if (bAdapter == null) {
        Icons.Rounded.BluetoothDisabled
    } else {
        Icons.Rounded.Message
    }

    image = if (bAdapter.isEnabled) {
        Icons.Rounded.Bluetooth
    } else {
        Icons.Rounded.BluetoothSearching
    }

    Row(
        modifier = modifier,
        horizontalArrangement = Arrangement.Center
    ) {
        var checked by remember { mutableStateOf(true) }

        Button(
            enabled = true,
            onClick = {/**/},
        ) {
            Icon(
                imageVector = image,
                contentDescription = "airplane",
                modifier = Modifier
                    .size(ToggleButtonDefaults.DefaultIconSize)
                    .wrapContentSize(align = Alignment.Center),
            )
        }
    }
}

@Composable
fun BluetoothButton(modifier: Modifier = Modifier, onBluetoothEnabled: (Boolean) -> Unit) {
    val bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
    val isBluetoothEnabled = remember { mutableStateOf(bluetoothAdapter.isEnabled) }

    val launcher = rememberLauncherForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (result.resultCode == Activity.RESULT_OK) {
            isBluetoothEnabled.value = true
            onBluetoothEnabled(true)
        } else {
            isBluetoothEnabled.value = false
            onBluetoothEnabled(false)
        }
    }

    Button(
        onClick = {
            if (!isBluetoothEnabled.value) {
                // Bluetooth is not enabled, request it from the user
                launcher.launch(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE))
            } else {
                // Bluetooth is already enabled
                onBluetoothEnabled(true)
            }
        },
        modifier = modifier,
        colors = ButtonDefaults.buttonColors(
            backgroundColor = if (isBluetoothEnabled.value) {
                MaterialTheme.colors.onSecondary
            } else {
                MaterialTheme.colors.onPrimary
            }
        )
    ) {
        Icon(
            imageVector = if (isBluetoothEnabled.value) {
                Icons.Rounded.Bluetooth
            } else {
                Icons.Rounded.BluetoothDisabled
            },
            contentDescription = null,
            modifier = Modifier
                .size(ToggleButtonDefaults.DefaultIconSize)
                .wrapContentSize(align = Alignment.Center),
        )
    }
}


// TODO: Create a Text Composable
@Composable
fun TextExample(modifier: Modifier = Modifier) {
    Text(
        modifier = modifier,
        textAlign = TextAlign.Center,
        color = MaterialTheme.colors.secondaryVariant,
        text = "Device Status"
    )
}

// TODO: Create a Card (specifically, an AppCard) Composable
@Composable
fun CardExample(
    modifier: Modifier = Modifier,
    iconModifier: Modifier = Modifier
) {
    AppCard(
        modifier = modifier,
        appImage = {
            Icon(
                imageVector = Icons.Rounded.Message,
                contentDescription = "triggers open message action",
                modifier = iconModifier
            )
        },
        appName = { Text("Messages") },
        time = { Text("12m") },
        title = { Text("Kim Green") },
        onClick = { /* ... */ }
    ) {
        Text("On my way!")
    }
}

// TODO: Create a Chip Composable
@Composable
fun ChipExample(
    modifier: Modifier = Modifier,
    iconModifier: Modifier = Modifier
) {
    Chip(
        modifier = modifier,
        onClick = { /* ... */ },
        label = {
            Text(
                text = "5 minute Meditation",
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
        },
        icon = {
            Icon(
                imageVector = Icons.Rounded.SelfImprovement,
                contentDescription = "triggers meditation action",
                modifier = iconModifier
            )
        },
    )
}

fun getColorForStatus(status: Int): Color {
    return when (status) {
        2 -> statusOn       // appliance is On
        1 -> statusOff      // appliance is off
        0 -> statusDisabled // no data for appliance
        else -> Color.Transparent
    }
}

@Composable
fun ToggleChipExample(
    modifier: Modifier = Modifier,
    iconModifier: Modifier = Modifier,
    status: Int,
    nodeNum: Int,
    powerVal: Float
) {
    val statusColor = getColorForStatus(status)
    val nodeName = "Node $nodeNum"

    Chip(
        modifier = modifier
            .padding(2.dp)
            .background(Color.Transparent),
        onClick = { /* ... */ },
        label = {
            Column {
                Row(
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = nodeName,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis
                    )
                    Spacer(Modifier.weight(1f))
                    Box(
                        modifier = Modifier
                            .size(16.dp)
                            .background(statusColor, CircleShape)
                    )
                }
                Box(
                    modifier = modifier
                        .padding(top = 2.dp)
                        .height(4.dp)
                        .fillMaxWidth()
                        .clip(RoundedCornerShape(2.dp))
                        .background(Color.Gray)
                ) {
                    Canvas(
                        modifier = Modifier
                            .fillMaxWidth()
                            .fillMaxHeight()
                    ) {
                        val roundRectColor = if (statusColor == statusOn) powerMeterColor else statusDisabled
                        val roundRectWidth = if (statusColor == statusOn) size.width * powerVal else 0f
                        drawRoundRect(
                            color = roundRectColor,
                            topLeft = Offset.Zero,
                            size = size.copy(width = roundRectWidth),
                            cornerRadius = CornerRadius(2.dp.toPx())
                        )
                    }
                }
            }
        },
        icon = {
            Icon(
                imageVector = Icons.Rounded.Devices,
                contentDescription = "status of node 1",
                modifier = iconModifier
            )
        },
    )
}


// Function only used as a demo for when you start the code lab (removed as step 1).
@Composable
fun StartOnlyTextComposables() {
    Text(
        modifier = Modifier.fillMaxSize(),
        textAlign = TextAlign.Center,
        color = MaterialTheme.colors.primary,
        text = stringResource(R.string.hello_world_starter)
    )
}








