#include <stdlib.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <settings/settings.h>
#include <drivers/gpio.h>
#include <bluetooth/mesh.h>
#include <random/rand32.h>

#include <zephyr.h>
#include <sys/printk.h>

#include "ble_power.h"


void main(void) {

	printk("esp32 start! -- power node\n");
	int err = bt_enable(bt_ready);
	if (err) {
		printk("bt_enable failed with err %d", err);
	}
	printk("bonjour\n");
	const char* btName = bt_get_name();

	// bt_mesh_reset();

	while (1) {
		if (bt_mesh_is_provisioned()) {
			printk("hello friendo\n");
			send_data_to_proxy(BT_MESH_MODEL_OP_NODE_TO_PROXY_UNACK);
			printk("proxy data sent\n");
		}
		k_msleep(5000);
	}
}
