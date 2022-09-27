#include <stdlib.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <settings/settings.h>
#include <drivers/gpio.h>
#include <bluetooth/mesh.h>
#include <random/rand32.h>

#include <zephyr.h>
#include <sys/printk.h>

#include "ble_proxy.h"

int8_t dataNodeToProxy[5] = {
	0x00,
	0x10,
	0x20,
	0x30,
	0x40
};

// ======================== Miscellaneous ============================================//
static void attention_on(struct bt_mesh_model *model) { printk("attention_on()\n"); }

static void attention_off(struct bt_mesh_model *model) { printk("attention_off()\n"); }

// device UUID
static uint8_t dev_uuid[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00 };

void gen_uuid() {
    uint32_t rnd1 = sys_rand32_get();
    uint32_t rnd2 = sys_rand32_get();
    uint32_t rnd3 = sys_rand32_get();
    uint32_t rnd4 = sys_rand32_get();

    dev_uuid[15] = (rnd1 >> 24) & 0x0FF;
    dev_uuid[14] = (rnd1 >> 16) & 0x0FF;
    dev_uuid[13] = (rnd1 >>  8) & 0x0FF;
    dev_uuid[12] =  rnd1 & 0x0FF;

    dev_uuid[11] = (rnd2 >> 24) & 0x0FF;
    dev_uuid[10] = (rnd2 >> 16) & 0x0FF;
    dev_uuid[9] = (rnd2 >>  8) & 0x0FF;
    dev_uuid[8] =  rnd2 & 0x0FF;

    dev_uuid[7] = (rnd3 >> 24) & 0x0FF;
    dev_uuid[6] = (rnd3 >> 16) & 0x0FF;
    dev_uuid[5] = (rnd3 >>  8) & 0x0FF;
    dev_uuid[4] =  rnd3 & 0x0FF;

    dev_uuid[3] = (rnd4 >> 24) & 0x0FF;
    dev_uuid[2] = (rnd4 >> 16) & 0x0FF;
    dev_uuid[1] = (rnd4 >>  8) & 0x0FF;
    dev_uuid[0] =  rnd4 & 0x0FF;

    /* Set 4 MSB bits of time_hi_and_version field */
    dev_uuid[6] &= 0x0f;
    dev_uuid[6] |= 4 << 4;

    /* Set 2 MSB of clock_seq_hi_and_reserved to 10 */
    dev_uuid[8] &= 0x3f;
    dev_uuid[8] |= 0x80;

}


// ======================== Message Opcodes ============================================//

// data receive from power node
#define BT_MESH_MODEL_OP_NODE_TO_PROXY_UNACK		BT_MESH_MODEL_OP_2(0x82, 0x40)

// #define BT_MESH_MODEL_OP_GENERIC_ONOFF_GET BT_MESH_MODEL_OP_2(0x82, 0x01)
// #define BT_MESH_MODEL_OP_GENERIC_ONOFF_SET BT_MESH_MODEL_OP_2(0x82, 0x02)
// #define BT_MESH_MODEL_OP_GENERIC_ONOFF_SET_UNACK BT_MESH_MODEL_OP_2(0x82, 0x03)
// #define BT_MESH_MODEL_OP_GENERIC_ONOFF_STATUS BT_MESH_MODEL_OP_2(0x82, 0x04)

// ======================== Health Models ============================================//

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};


// ======================== Provisioning ============================================//

static int provisioning_output_pin(bt_mesh_output_action_t action, uint32_t number) {
	printk("OOB Number: %u\n", number);
	return 0;
}

static void provisioning_complete(uint16_t net_idx, uint16_t addr) {
    printk("Provisioning completed\n");
}

static void provisioning_reset(void) {
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

// provisioning properties and capabilities
static const struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.output_size = 4,
	.output_actions = BT_MESH_DISPLAY_NUMBER,
	.output_number = provisioning_output_pin,
	.complete = provisioning_complete,
	.reset = provisioning_reset,
};


// ======================== Generic Function Callbacks ============================================//

/*
static void generic_onoff_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, 
			struct net_buf_simple *buf)
{
	// do things here :)
}

static void generic_onoff_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	// do more things here :)
}

static void generic_onoff_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,	
			struct net_buf_simple *buf)
{
	// even more things :)
}
*/

// ======================== Generic model definitions ============================================//

// 10 bytes + 1
BT_MESH_MODEL_PUB_DEFINE(data_node_to_proxy, NULL, 11);


// static const struct bt_mesh_model_op generic_onoff_op[] = {
// 		{BT_MESH_MODEL_OP_GENERIC_ONOFF_GET, 0, generic_onoff_get},
// 		{BT_MESH_MODEL_OP_GENERIC_ONOFF_SET, 2, generic_onoff_set},
// 		{BT_MESH_MODEL_OP_GENERIC_ONOFF_SET_UNACK, 2, generic_onoff_set_unack},
// 		BT_MESH_MODEL_OP_END,
// };

// model publication context
// BT_MESH_MODEL_PUB_DEFINE(generic_onoff_pub, NULL, 2 + 1);

static const struct bt_mesh_model_op data_from_power_node_op[] = {
    {BT_MESH_MODEL_OP_NODE_TO_PROXY_UNACK, 10, get_data_from_power_node},
    BT_MESH_MODEL_OP_END,
};

BT_MESH_MODEL_PUB_DEFINE(data_from_power_node_pub, NULL, 11);

static struct bt_mesh_model sig_models[] = {
    // BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_XYL_SRV, data_from_power_node_op, &data_from_power_node_pub, NULL),
	//BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, generic_onoff_op, &generic_onoff_pub, NULL),
};

// node contains elements.note that BT_MESH_MODEL_NONE means "none of this type" ands here means "no vendor models"
static struct bt_mesh_elem elements[] = {
		BT_MESH_ELEM(0, sig_models, BT_MESH_MODEL_NONE),
};

// node
static const struct bt_mesh_comp comp = {
		.cid = 0xFFFF,
		.elem = elements,
		.elem_count = ARRAY_SIZE(elements),
};




// ======================== Externally Used Functions ============================================//

void get_data_from_power_node(struct bt_mesh_model* model, struct bt_mesh_msg_ctx* ctx, struct net_buf_simple* buf) {

    int8_t recMsg[5];

    for (int i = 0; i < 5; i++) {
        recMsg[i] = net_buf_simple_pull_u8(buf);
        printk("received[%d]: %d\n", i, recMsg[i]);
    }
}

void bt_ready(int err) {

	if (err) {
		printk("bt_enable init failed with err %d", err);
		return;
	}
    printk("Bluetooth initialised OK\n");

	gen_uuid();

    printk("\n%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n\n",
            dev_uuid[15], dev_uuid[14], dev_uuid[13], dev_uuid[12],dev_uuid[11], dev_uuid[10], dev_uuid[9], dev_uuid[8],
            dev_uuid[7], dev_uuid[6], dev_uuid[5], dev_uuid[4],dev_uuid[3], dev_uuid[2], dev_uuid[1], dev_uuid[0]);

	err = bt_mesh_init(&prov, &comp);

	if (err) {
		printk("bt_mesh_init failed with err %d", err);
		return;
	}

	printk("Mesh initialised OK: 0x%04x\n",elements[0].addr);

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	    printk("Settings loaded: 0x%04x\n",elements[0].addr);
	}

    if (!bt_mesh_is_provisioned()) {
    	printk("Node has not been provisioned - beaconing\n");
		bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
	} else {
    	printk("Node has already been provisioned\n");
	    printk("Node unicast address: 0x%04x\n",elements[0].addr);
	}
}



