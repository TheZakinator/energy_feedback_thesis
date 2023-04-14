

#ifndef BLE_POWER_H
#define BLE_POWER_H

#include <zephyr.h>
#include "main.h"

#define BT_MESH_MODEL_OP_NODE_TO_PROXY_UNACK		BT_MESH_MODEL_OP_2(0x82, 0x40)

void bt_ready(int err);

int send_data_to_proxy(uint16_t messageType, PowerNodeData_t* powerNodeData);


#endif