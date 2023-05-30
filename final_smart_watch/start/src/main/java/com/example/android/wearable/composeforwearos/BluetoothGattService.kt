package com.example.android.wearable.composeforwearos

import android.annotation.SuppressLint
import android.bluetooth.*
import android.util.Log
import com.example.android.wearable.composeforwearos.BleMeshProxyService.Companion.SVC_CCC_DESCRIPTOR
import com.example.android.wearable.composeforwearos.BleMeshProxyService.Companion.SVC_PROXY_CHARACTERSTIC
import java.nio.ByteBuffer
import java.nio.ByteOrder


private var gattConnected : Boolean = false
private var serviceDiscovered : Boolean = false
private var savedService : android.bluetooth.BluetoothGattService? = null
private var savedGatt : BluetoothGatt? = null
private var savedNodeDataList = MutableList<ProxyNodeData?>(3) {null}


class BluetoothGattService(private val bluetoothScanService: BluetoothScanService) : BluetoothGattCallback() {

//    private val value = byteArrayOf(0x01, 0x02, 0x03, 0x04)

    @SuppressLint("MissingPermission")
    override fun onConnectionStateChange(gatt: BluetoothGatt?, status: Int, newState: Int) {
        super.onConnectionStateChange(gatt, status, newState)

        when (newState) {
            BluetoothProfile.STATE_CONNECTED -> {
                // connection established, discover gatt services
                Log.d("dev_logs_gatt", "gatt connection successful")
                gattConnected = true
                savedGatt = gatt
                gatt?.discoverServices()
            }
            BluetoothProfile.STATE_DISCONNECTED -> {
                // connection lost, clean up resources
                Log.d("dev_logs_gatt", "gatt connection lost")
                gattConnected = false
                serviceDiscovered = false
                savedService = null
                gatt?.close()
            }
        }
    }

//    @RequiresApi(Build.VERSION_CODES.TIRAMISU)
    @SuppressLint("MissingPermission")
    override fun onServicesDiscovered(gatt: BluetoothGatt?, status: Int) {
        super.onServicesDiscovered(gatt, status)

        when (status) {
            BluetoothGatt.GATT_SUCCESS -> {
                // discovery successful
                Log.d("dev_logs_gatt", "discovery successful")

                val services = gatt?.services
                Log.d("dev_logs_gatt", "services: $services")

                if (services != null) {
                    for (service in services) {
                        val uuid = service.uuid.toString()
                        val type = service.type
                        val characteristics = service.characteristics

                        Log.d("dev_logs_gatt", "Service Info: UUID: $uuid, Type: $type")

                        for (characteristic in characteristics) {
                            val charUUID = characteristic.uuid.toString()
                            val properties = characteristic.properties
                            val permissions = characteristic.permissions

                            Log.d("dev_logs_gatt","Char Info: UUID: $charUUID, " +
                                    "Properties: $properties" + "Permissions: $permissions")

                            if (charUUID == SVC_PROXY_CHARACTERSTIC.toString()) {
                                Log.d("dev_logs_gatt","Hmmmmm UUID: $charUUID, " +
                                        "svc_char: $SVC_PROXY_CHARACTERSTIC")

                                val chrc = service.getCharacteristic(SVC_PROXY_CHARACTERSTIC)

                                gatt.setCharacteristicNotification(chrc, true)

                                val cccDescriptor = chrc.getDescriptor(SVC_CCC_DESCRIPTOR)
//                                Log.d("dev_logs_gatt", "ccc desc: ${cccDescriptor.uuid} and " +
//                                        "${cccDescriptor.characteristic} and ${cccDescriptor.permissions}")

                                cccDescriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                                gatt.writeDescriptor(cccDescriptor)


                                // Device with correct service, and, service with correct
                                // characterstic has been detected
                                serviceDiscovered = true
                                savedService = service

                            }
                        }
                    }
                }
            }
            else -> {
                // discovery failed
            }
        }
    }

    override fun onDescriptorWrite(gatt: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {

        super.onDescriptorWrite(gatt, descriptor, status)

        if (status == BluetoothGatt.GATT_SUCCESS) {
            // Notifications enabled successfully
            Log.d("dev_logs_gatt", "notifications enabled successfully")
        } else {
            // Error enabling notifications
            Log.d("dev_logs_gatt", "notification enable FAILED")
        }
    }


    @Deprecated("Deprecated in Java")
    override fun onCharacteristicRead(
        gatt: BluetoothGatt?,
        characteristic: BluetoothGattCharacteristic?,
        status: Int
    ) {
        super.onCharacteristicRead(gatt, characteristic, status)

        Log.d("dev_logs_gatt", "ONCHARARACTERSTICREAD")

        when (status) {
            BluetoothGatt.GATT_SUCCESS -> {
                // characterstic read successful
                Log.d("dev_logs_gatt", "read successful")
                val value = characteristic?.value
                setNodeData(value)
            }
            else -> {
                Log.d("dev_logs_gatt", "read failed")
                // read failed
            }
        }
    }

    override fun onCharacteristicWrite(
        gatt: BluetoothGatt?,
        characteristic: BluetoothGattCharacteristic?,
        status: Int
    ) {
        super.onCharacteristicWrite(gatt, characteristic, status)

        when (status) {
            BluetoothGatt.GATT_SUCCESS -> {
                Log.d("dev_logs_gatt", "write successful")
                // Characteristic write successful
            }
            else -> {
                Log.d("dev_logs_gatt", "write failed")
                // Characteristic write failed
            }
        }
    }

    @Deprecated("Deprecated in Java")
    override fun onCharacteristicChanged(
        gatt: BluetoothGatt,
        characteristic: BluetoothGattCharacteristic
    ) {
        super.onCharacteristicChanged(gatt, characteristic)
        Log.d("dev_logs_gatt", "chrc changed")

        val value = characteristic.value
        sortNodeData(parseProxyNodeData(value))
    }

    // returns the state of the GATT connection
    fun getConnectionStatus() : Boolean {
        return gattConnected
    }

    // returns if the device which has connected via GATT has the correct service and
    // characteristics
    fun getServiceDiscoveredStatus() : Boolean {
        return serviceDiscovered
    }

    // write data to the proxy device based on the characteristic which is defined in
    // readWriteCharacteristic
    // returns 1 on success
    @SuppressLint("MissingPermission")
    fun writeDataToProxy(value: ByteArray) : Boolean? {
        val characteristic = savedService?.getCharacteristic(SVC_PROXY_CHARACTERSTIC)
        characteristic?.value = value
        return savedGatt?.writeCharacteristic(characteristic)
    }

    // read data to the proxy device based on the characteristic which is defined in
    // readWriteCharacteristic
    @SuppressLint("MissingPermission")
    fun readDataFromProxy() : Boolean? {
        val characteristic = savedService?.getCharacteristic(SVC_PROXY_CHARACTERSTIC)
        return savedGatt?.readCharacteristic(characteristic)
    }

    private fun sortNodeData(data : ProxyNodeData?) {
        if (data != null) {
            when (data.nodeNum) {
                0, 1, 2 -> {
                    savedNodeDataList[data.nodeNum] = data
                }
            }
        }
    }

    fun getPowerNodeDataList() : MutableList<ProxyNodeData?> {
        return savedNodeDataList
    }

    private fun parseProxyNodeData(bytes: ByteArray): ProxyNodeData {

//        Log.d("dev_logs_gatt", "Received bytes size: ${bytes.size}")

        val buffer = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)

        val applianceOn = buffer.get().toInt() and 0xFF
        val hysterisisLevel = buffer.get().toInt() and 0xFF
        val nodeNum = buffer.get().toInt() and 0xFF
        buffer.get() // garbage value

        // Read voltageVal as a 32-bit value
        val voltageVal = buffer.int.toLong() and 0xFFFFFFFFL

//        Log.d("dev_logs_gatt", "Parsed values 0: applianceOn=${applianceOn.toUInt()}, hysterisisLevel=${hysterisisLevel.toUInt()}, nodeNum=${nodeNum.toUInt()}, voltageVal=${voltageVal.toUInt()}")


        return ProxyNodeData(applianceOn, hysterisisLevel, nodeNum, voltageVal)
    }
}

data class ProxyNodeData(
    val applianceOn: Int,
    val hysterisisLevel: Int,
    val nodeNum: Int,
    val voltageVal: Long
    )


