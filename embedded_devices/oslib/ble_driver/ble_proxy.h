

#ifndef BLE_PROXY_H
#define BLE_PROXY_H

#include <zephyr.h>

void bt_ready(int err);

void get_data_from_power_node(struct bt_mesh_model* model, struct bt_mesh_msg_ctx* ctx, struct net_buf_simple* buf);

void register_callbacks();

void init_node_data();

void change_data_intermittently();

#endif