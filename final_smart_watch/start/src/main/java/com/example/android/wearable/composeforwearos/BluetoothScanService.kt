package com.example.android.wearable.composeforwearos

import android.R
import android.annotation.SuppressLint
import android.app.*
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothManager
import android.bluetooth.le.*
import android.content.Context
import android.content.Intent
import android.graphics.Color
import android.os.Binder
import android.os.Build
import android.os.IBinder
import android.os.ParcelUuid
import android.util.Log
import android.widget.ImageView
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.rounded.AddAlert
import androidx.core.app.NotificationCompat
import java.util.*

class BluetoothScanService : Service() {

    private val binder = BluetoothBinder()

    private lateinit var bluetoothAdapter : BluetoothAdapter
    private lateinit var bluetoothManager : BluetoothManager

    private var attemptConnection : Boolean = false
    private var device : BluetoothDevice? = null
    private var bleScanner : BluetoothLeScanner? = null
    private val scanResults : MutableList<ScanResult> = mutableListOf()

    private val gattCallback = BluetoothGattService(this)
    private var gatt : BluetoothGatt? = null
    private var savedNodeDataList = MutableList<ProxyNodeData?>(3) {null}

    private fun setupIndividualNotificationChannel(channelId: String, channelName: String, vibrationStrength: Int): NotificationChannel {

        val name = "Gatt Events Channel"
        val importance = NotificationManager.IMPORTANCE_HIGH

        val channel = NotificationChannel(channelId, channelName, importance)

        channel.enableLights(true)
        channel.lightColor = Color.RED
        if (vibrationStrength == 0) {
            channel.vibrationPattern = longArrayOf(0, 1000, 500, 1000)
            channel.description = "low vibration strength"
        } else if (vibrationStrength == 1) {
            channel.vibrationPattern = longArrayOf(0, 750, 400, 750, 750, 400)
            channel.description = "medium vibration strength"
        } else if (vibrationStrength == 2) {
            channel.vibrationPattern = longArrayOf(0, 400, 400, 400, 400, 400)
            channel.description = "high vibration strength"
        }
        channel.enableVibration(true)
        return channel
    }

    fun createNotificationChannels() {
        val name = "Gatt Events Channel"
        val importance = NotificationManager.IMPORTANCE_HIGH

        val channel_n1_low = setupIndividualNotificationChannel(CHANNEL_ID_NODE_1_LOW, name + "n1 low", 0)
        val channel_n1_med = setupIndividualNotificationChannel(CHANNEL_ID_NODE_1_MED, name + "n1 med", 1)
        val channel_n1_high = setupIndividualNotificationChannel(CHANNEL_ID_NODE_1_HIGH, name + "n1 high", 2)

        val channel_n2_low = setupIndividualNotificationChannel(CHANNEL_ID_NODE_2_LOW, name + "n2 low", 0)
        val channel_n2_med = setupIndividualNotificationChannel(CHANNEL_ID_NODE_2_MED, name + "n2 med", 1)
        val channel_n2_high = setupIndividualNotificationChannel(CHANNEL_ID_NODE_2_HIGH, name + "n2 high", 2)

        val channel_n3_low = setupIndividualNotificationChannel(CHANNEL_ID_NODE_3_LOW, name + "n3 low", 0)
        val channel_n3_med = setupIndividualNotificationChannel(CHANNEL_ID_NODE_3_MED, name + "n3 med", 1)
        val channel_n3_high = setupIndividualNotificationChannel(CHANNEL_ID_NODE_3_HIGH, name + "n3 high", 2)


        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.createNotificationChannel(channel_n1_low)
        notificationManager.createNotificationChannel(channel_n1_med)
        notificationManager.createNotificationChannel(channel_n1_high)

        notificationManager.createNotificationChannel(channel_n2_low)
        notificationManager.createNotificationChannel(channel_n2_med)
        notificationManager.createNotificationChannel(channel_n2_high)

        notificationManager.createNotificationChannel(channel_n3_low)
        notificationManager.createNotificationChannel(channel_n3_med)
        notificationManager.createNotificationChannel(channel_n3_high)
    }

    private fun showGattEventNotification(title: String, message: String, channelId: String, notificationId: Int) {
        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager

        val notification = NotificationCompat.Builder(this, channelId)
            .setDefaults(Notification.DEFAULT_VIBRATE)
//            .setWhen(System.currentTimeMillis())
//            .setTicker("Hearty365")
            .setContentTitle(title)
            .setContentText(message)
            .setSmallIcon(android.R.drawable.ic_dialog_alert)
            .setAutoCancel(true)
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .setCategory(NotificationCompat.CATEGORY_ALARM)
            .build()

        notificationManager.notify(NOTIFICATION_ID + notificationId, notification)
    }

    private val scanCallback = object : ScanCallback() {

        override fun onScanResult(callbackType: Int, result: ScanResult?) {
            super.onScanResult(callbackType, result)
            result?.let { scanResult ->

                if (!scanResults.contains(scanResult)) {
                    scanResults.add(scanResult)
                    Log.d("dev_logs", "result: $scanResult")
                }

                val serviceUuids = result.scanRecord?.serviceUuids as? List<ParcelUuid>
                val bleMeshProxyServiceUuid = ParcelUuid.fromString(BleMeshProxyService.SERVICE_UUID.toString())

                if (serviceUuids != null && serviceUuids.contains(bleMeshProxyServiceUuid)) {
                    // attempt to connect to device
                    Log.d("dev_logs_gatt", "result: $scanResult")

                    // let MainActivity know that a connection is ready to be established
                    attemptConnection = true
                    device = result.device

                }
            }
        }

        override fun onScanFailed(errorCode: Int) {
            super.onScanFailed(errorCode)
            when (errorCode) {
                SCAN_FAILED_ALREADY_STARTED -> Log.e("dev_logs", "Scan already started.")
                SCAN_FAILED_APPLICATION_REGISTRATION_FAILED -> Log.e("dev_logs", "Application registration failed.")
                SCAN_FAILED_FEATURE_UNSUPPORTED -> Log.e("dev_logs", "Feature unsupported.")
                SCAN_FAILED_INTERNAL_ERROR -> Log.e("dev_logs", "Internal error.")
            }
        }
    }

    override fun onBind(intent: Intent?): IBinder {
        return binder
    }

    override fun onCreate() {
        super.onCreate()
        Log.d("dev_logs", "scan service - created")

        setBluetoothAdapter()

    }

    private fun createNotificationChannel() {
        val channelName = "Bluetooth Scan Service"
        val importance = NotificationManager.IMPORTANCE_LOW
        val channel = NotificationChannel(CHANNEL_ID_FOREGROUND_SERVICE, channelName, importance)

        val notificationManager = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.createNotificationChannel(channel)
    }

    private fun buildForegroundNotification(): Notification {
        val notificationIntent = Intent(this, MainActivity::class.java)
        val pendingIntentFlags =
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        val pendingIntent = PendingIntent.getActivity(
            this, 0, notificationIntent, pendingIntentFlags
        )

        return NotificationCompat.Builder(this, CHANNEL_ID_FOREGROUND_SERVICE)
            .setContentTitle("Bluetooth Scan Service")
            .setContentText("Scanning for Bluetooth devices")
            .setSmallIcon(R.mipmap.sym_def_app_icon)
            .setContentIntent(pendingIntent)
            .build()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        Log.d("dev_logs", "scan service - start command")

        createNotificationChannels()

        createNotificationChannel()
        val foregroundNotification = buildForegroundNotification()
        startForeground(NOTIFICATION_ID_FOREGROUND, foregroundNotification)

        if (bluetoothAdapter.isEnabled) {
            Log.d("dev_logs", "bluetoothAdapter is enabled, startScan()")
            startScan()
        } else {
            Log.e("dev_logs", "Bluetooth is disabled")
        }
        return START_STICKY
    }

    override fun onDestroy() {
        super.onDestroy()
        Log.e("dev_logs", "scan service - onDestroy()")
        gatt = null
        stopSelf() // destroy the foreground service is the application is closed
    }

    @SuppressLint("MissingPermission")
    private fun startScan() {
        Log.d("dev_logs", "Starting Scan")

        val scanSettings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()

        val scanFilter = ScanFilter.Builder()
            .setServiceUuid(ParcelUuid.fromString(BleMeshProxyService.SERVICE_UUID.toString()))
            .build()

        bleScanner = bluetoothAdapter.bluetoothLeScanner

        bluetoothAdapter.bluetoothLeScanner.startScan(listOf(scanFilter), scanSettings, scanCallback)
    }

    @SuppressLint("MissingPermission")
    private fun stopScan() {
        Log.d("dev_logs", "stopping scan")
        bluetoothAdapter.bluetoothLeScanner.stopScan(scanCallback)
    }

    @SuppressLint("MissingPermission")
    fun intermittentCommunication(): Boolean {
        Log.d("dev_logs_comm", "intermittent communication")

        if (!gattCallback.getConnectionStatus()) {
            // attempt connection only if connection does not exist
            Log.d("dev_logs_comm", "attempting gatt connection")
            gatt = device?.connectGatt(applicationContext, false, gattCallback)
        }

        if (gatt == null) {
            // Connection failed
            Log.d("dev_logs_comm", "gatt connection failed")
            return false

        } else {
            // Connection successful (or still connected)
            Log.d("dev_logs_comm", "gatt connected")
            if (gattCallback.getServiceDiscoveredStatus()) {
                // device discovered has correct service and characteristics
                bleScanner?.stopScan(scanCallback)

                savedNodeDataList = gattCallback.getPowerNodeDataList()

                for (node in savedNodeDataList) {
                    if (node != null) {
                        val powerVal = (node.voltageVal / 2147483648.0).toFloat()
                        if (node.hysterisisLevel > 0) {
                            Log.d("dev_logs_gatt", "hysterisis > 0")

                            if (node.nodeNum == 1) {
                                Log.d("dev_logs_gatt", "nodenum 1")
                                showGattEventNotification("Node 1", "Elec Usage: $powerVal", CHANNEL_ID_NODE_1_LOW, 0)
                            } else if (node.nodeNum == 2) {
                                Log.d("dev_logs_gatt", "nodenum 2")
                                showGattEventNotification("Node 2", "Elec Usage: $powerVal", CHANNEL_ID_NODE_2_MED, 2)
                            } else if (node.nodeNum == 3) {
                                Log.d("dev_logs_gatt", "nodenum 3")
                                showGattEventNotification("Node 3", "Elec Usage: $powerVal", CHANNEL_ID_NODE_3_HIGH, 1)
                            }
                        }
                    }
                }


            } else {
                // discover services
                gatt?.discoverServices()
            }

        }
        return true
    }

    fun getPowerNodeDataList() : MutableList<ProxyNodeData?> {
        return savedNodeDataList
    }

    private fun setBluetoothAdapter() {
        bluetoothManager = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothAdapter = bluetoothManager.adapter
    }

    inner class BluetoothBinder : Binder() {
        fun getService(): BluetoothScanService {
            return this@BluetoothScanService
        }
    }

    companion object {
        private const val CHANNEL_ID_FOREGROUND_SERVICE = "foreground_service_channel"
        private const val CHANNEL_ID_NODE_1_LOW = "gatt_event_n1_low"
        private const val CHANNEL_ID_NODE_1_MED = "gatt_event_n1_med"
        private const val CHANNEL_ID_NODE_1_HIGH = "gatt_event_n1_high"
        private const val CHANNEL_ID_NODE_2_LOW = "gatt_event_n2_low"
        private const val CHANNEL_ID_NODE_2_MED = "gatt_event_n2_med"
        private const val CHANNEL_ID_NODE_2_HIGH = "gatt_event_n2_high"
        private const val CHANNEL_ID_NODE_3_LOW = "gatt_event_n3_low"
        private const val CHANNEL_ID_NODE_3_MED = "gatt_event_n3_med"
        private const val CHANNEL_ID_NODE_3_HIGH = "gatt_event_n3_high"

        private const val NOTIFICATION_ID = 2
        private const val NOTIFICATION_ID_FOREGROUND = 1
    }
}

interface BleMeshProxyService {
    companion object {
        // bluetooth mesh proxy service advertisement UUID
        val SERVICE_UUID: UUID = UUID.fromString("00001828-0000-1000-8000-00805f9b34fb")

        // proxy - mesh GATT service UUID
        val SVC_PROXY_CHARACTERSTIC: UUID = UUID.fromString("cbcaa706-eb60-4926-9121-1678356792d0")

        val SVC_CCC_DESCRIPTOR: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

    }
}