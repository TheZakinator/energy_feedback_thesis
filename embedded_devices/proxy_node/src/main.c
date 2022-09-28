#include <stdlib.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <settings/settings.h>
#include <drivers/gpio.h>
#include <bluetooth/mesh.h>
#include <random/rand32.h>
#include <bluetooth/conn.h>

#include <zephyr.h>
#include <sys/printk.h>

#include "ble_proxy.h"


void main(void) {

	printk("esp32 start! -- proxy node\n");
	int err = bt_enable(bt_ready);
	if (err) {
		printk("bt_enable failed with err %d", err);
	}

	register_callbacks();
	
	// bt_mesh_reset();

	while (1) {
		if (bt_mesh_is_provisioned()) {
			
		}
		k_msleep(2000);
	}
}
