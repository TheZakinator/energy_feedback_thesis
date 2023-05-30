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

import android.R
import android.app.Notification
import android.app.Notification.WearableExtender
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.graphics.BitmapFactory
import android.graphics.Color
import android.os.Bundle
import android.os.IBinder
import android.util.Log
import android.widget.RemoteViews
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.core.app.NotificationCompat
import androidx.wear.compose.material.*
import com.example.android.wearable.composeforwearos.theme.WearAppTheme
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.*

class MainActivity : ComponentActivity() {

    private lateinit var bluetoothScanService : BluetoothScanService
    private var isBluetoothBound = false
    private var nodeDataListState = mutableStateOf<List<ProxyNodeData?>>(listOf(null, null, null))

    private val bluetoothServiceConnection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
            Log.d("dev_logs", "onServiceConnected()")
            val binder = service as BluetoothScanService.BluetoothBinder
            bluetoothScanService = binder.getService()
            isBluetoothBound = true
        }

        override fun onServiceDisconnected(name: ComponentName?) {
            isBluetoothBound = false
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
    }

    override fun onStart() {
        super.onStart()
        Log.d("dev_logs", "main activity - started")
        val scanServiceIntent = Intent(this, BluetoothScanService::class.java)
//        startService(scanServiceIntent)
        startForegroundService(scanServiceIntent)
        bindService(scanServiceIntent, bluetoothServiceConnection, Context.BIND_AUTO_CREATE)

        Timer().schedule(object: TimerTask() {
            override fun run() {

                if (isBluetoothBound) {
                    bluetoothScanService.intermittentCommunication()
                    runOnUiThread {
//                        nodeDataList = bluetoothScanService.getPowerNodeDataList()
                        nodeDataListState.value = bluetoothScanService.getPowerNodeDataList().toList()
                    }
                    Log.d("dev_logs_main", "MAIN VALUES 0: applianceOn=${nodeDataListState.value[0]?.applianceOn?.toUInt()}, hysterisisLevel=${nodeDataListState.value[0]?.hysterisisLevel?.toUInt()}, nodeNum=${nodeDataListState.value[0]?.nodeNum?.toUInt()}, voltageVal=${nodeDataListState.value[0]?.voltageVal?.toUInt()}")
                    Log.d("dev_logs_main", "MAIN VALUES 1: applianceOn=${nodeDataListState.value[1]?.applianceOn?.toUInt()}, hysterisisLevel=${nodeDataListState.value[1]?.hysterisisLevel?.toUInt()}, nodeNum=${nodeDataListState.value[1]?.nodeNum?.toUInt()}, voltageVal=${nodeDataListState.value[1]?.voltageVal?.toUInt()}")
                    Log.d("dev_logs_main", "MAIN VALUES 2: applianceOn=${nodeDataListState.value[2]?.applianceOn?.toUInt()}, hysterisisLevel=${nodeDataListState.value[2]?.hysterisisLevel?.toUInt()}, nodeNum=${nodeDataListState.value[2]?.nodeNum?.toUInt()}, voltageVal=${nodeDataListState.value[2]?.voltageVal?.toUInt()}")
                }
            }
        }, 5000, 5000) // call every 5 seconds

        setContent {
            WearApp(nodeDataListState)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        if (isBluetoothBound) {
            unbindService(bluetoothServiceConnection)
            isBluetoothBound = false
        }
    }
}

@Composable
fun WearApp(nodeDataListState: MutableState<List<ProxyNodeData?>>) {

    val currentTime = remember { mutableStateOf(System.currentTimeMillis()) }

    LaunchedEffect(Unit) {
        launch {
            while (true) {
                delay(5000)
                currentTime.value = System.currentTimeMillis()
            }
        }
    }

    val applianceOnValues = remember (nodeDataListState) {
        derivedStateOf {
            nodeDataListState.value.map { it?.applianceOn ?: 0 }
        }
    }

    LaunchedEffect(nodeDataListState, currentTime) {}

    WearAppTheme {
        val listState = rememberScalingLazyListState()

        LaunchedEffect(applianceOnValues) {}

        Scaffold(
            timeText = {
                TimeText(modifier = Modifier.scrollAway(listState))
            },
            vignette = {
                // Only show a Vignette for scrollable screens. This code lab only has one screen,
                // which is scrollable, so we show it all the time.
                Vignette(vignettePosition = VignettePosition.TopAndBottom)
            },
            positionIndicator = {
                PositionIndicator(
                    scalingLazyListState = listState
                )
            }
        ) {
            val contentModifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 8.dp)
            val iconModifier = Modifier
                .size(24.dp)
                .wrapContentSize(align = Alignment.Center)

            ScalingLazyColumn(
                modifier = Modifier.fillMaxSize(),
                autoCentering = AutoCenteringParams(itemIndex = 0),
                state = listState
            ) {

                item { BluetoothButton(onBluetoothEnabled = { /* handle Bluetooth enabled */ }) }


                for (index in nodeDataListState.value.indices) {
                    val applianceOn = nodeDataListState.value[index]?.applianceOn ?: 0
                    val rawValue = nodeDataListState.value[index]?.voltageVal ?: 0
                    val powerVal = (rawValue / 4294967295.0).toFloat()
                    item {
                        ToggleChipExample(
                            contentModifier,
                            iconModifier,
                            applianceOn,
                            index + 1,
                            powerVal,
                        )
                    }
                }
            }
        }
    }
}

fun setNodeData(value: ByteArray?) {
    // check size of bytearray

    val intValue = value?.let { ByteBuffer.wrap(it).order(ByteOrder.LITTLE_ENDIAN).int }

    Log.d("dev_logs_main", "main activity - node data: $intValue")
}
