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
#include "main.h"

PowerNodeData_t powerNodeData;

int8_t dataNodeToProxy[5] = {
	0x00,
	0x10,
	0x20,
	0x30,
	0x40,
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

// #define BT_MESH_MODEL_OP_NODE_TO_PROXY_UNACK		BT_MESH_MODEL_OP_2(0x82, 0x40)

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
// 1 + 1 + 4 (size of PowerNodeData_t struct)
// 
BT_MESH_MODEL_PUB_DEFINE(powerNodeDataModel, NULL, 7);



static struct bt_mesh_model sig_models[] = {
    // BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_XYL_CLI, NULL, &powerNodeDataModel, &powerNodeData),
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

int send_data_to_proxy(uint16_t messageType, PowerNodeData_t* data) {
	int err;

	memcpy(&powerNodeData, data, sizeof(powerNodeData));

	printk("applianceOn: %d\n", powerNodeData.applianceOn);
	printk("hysterisisLevel: %d\n", powerNodeData.hysterisisLevel);
	printk("nodeNum: %d\n", powerNodeData.nodeNum);
	printk("voltageVal: %d\n", powerNodeData.voltageVal);


	struct bt_mesh_model* model = &sig_models[2];
	printk("sending data to proxy...\n");
	if (model->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
		printk("No publish address associated with the light HSL client model - \
					add one with a configuration app like nRF Mesh\n");
		return -1;
	}

	struct net_buf_simple* msg = model->pub->msg;
	bt_mesh_model_msg_init(msg, messageType);

	// for (int i = 0; i < 5; i++) {
	// 	net_buf_simple_add_u8(msg, dataNodeToProxy[i]);
	// }

	net_buf_simple_add_u8(msg, powerNodeData.applianceOn);
	net_buf_simple_add_u8(msg, powerNodeData.hysterisisLevel);
	net_buf_simple_add_u8(msg, powerNodeData.nodeNum);
	net_buf_simple_add_le32(msg, powerNodeData.voltageVal);	

	printk("publishing data from power node to proxy node\n");
	err = bt_mesh_model_publish(model);
	if (err) {
		printk("bt_mesh_model_publish err %d\n", err);
	}

	return err;
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



