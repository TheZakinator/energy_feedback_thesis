#include <zephyr.h>

#include <stdlib.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <settings/settings.h>
#include <drivers/gpio.h>
#include <bluetooth/mesh.h>
#include <random/rand32.h>

#include <init.h>
#include <sys/util.h>

#include <device.h>
#include <devicetree.h>

#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <sys/byteorder.h>
#include <sys/printk.h>

#include "ble_proxy.h"

// ==========================================================
// GATT Characteristics --- Proxy to Watch Connection
// ==========================================================

struct bt_conn* default_conn;
bool ble_connected = false;

/* Custom Service Variables */
static struct bt_uuid_128 watch_uuid = BT_UUID_INIT_128(
    0xd0, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
    0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcb);

static struct bt_uuid_128 proxy_uuid = BT_UUID_INIT_128(
    0xd1, 0x92, 0x67, 0x35, 0x78, 0x16, 0x21, 0x91,
    0x26, 0x49, 0x60, 0xeb, 0x06, 0xa7, 0xca, 0xcb);

struct packet_response {
    int16_t tempData1;
    int16_t tempData2;
	int16_t tempData3;
};

int16_t data_request[3];
struct packet_response packetResponse;

struct packet_response allSensorData; // ?????

static ssize_t write_chrc(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       const void *buf, uint16_t len,
			       uint16_t offset, uint8_t flags)
{
	printk("chrc len %u offset %u\n", len, offset);
    // struct bt_gatt_chrc* chrc = (struct bt_gatt_chrc*) attr->user_data;
    // chrc_handle = chrc->value_handle;
	memcpy(data_request, buf, len);

	printk("\nOne: %d\n", data_request[0]);
    printk("ID: %d\n", data_request[1]);
    printk("value: %d\n", data_request[2]);
	return len;
}

static ssize_t read_chrc(struct bt_conn* conn, 
                const struct bt_gatt_attr* attr,
                void* buf, uint16_t len, uint16_t offset) {

    printk("read request received\n");

    return bt_gatt_attr_read(conn, attr, buf, len, offset, 
                        (void*) &data_request, sizeof(data_request));               
}

BT_GATT_SERVICE_DEFINE(proxy_svc,
                    BT_GATT_PRIMARY_SERVICE(&proxy_uuid),

                    BT_GATT_CHARACTERISTIC(&watch_uuid.uuid,
                                            BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ,
                                            BT_GATT_PERM_WRITE | BT_GATT_PERM_READ,
                                            read_chrc, write_chrc, NULL)
                    );

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err)
    {
        printk("Connection failed (err 0x%02x)\n", err);
        ble_connected= false;
    }
    else
    {
        printk("BLE Connected to Device\n");
        ble_connected= true;
        struct bt_le_conn_param *param = BT_LE_CONN_PARAM(6, 6, 0, 400);

        default_conn = conn;

        if (bt_conn_le_param_update(conn, param) < 0)
        {
			printk("i be stuck sadge\n");
            // while (1)
            // {
            //     printk("Connection Update Error\n");
            //     k_msleep(10);
            // }
        }
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected (reason 0x%02x)\n", reason);
    ble_connected= false;
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

static struct bt_conn_auth_cb auth_cb_display = {
    .passkey_display = auth_passkey_display,
    .passkey_entry = NULL,
    .cancel = auth_cancel,
};

// external function
void register_callbacks() {
	// bt_conn_cb_register(&conn_callbacks);
    // bt_conn_auth_cb_register(&auth_cb_display);
}

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
BT_MESH_MODEL_PUB_DEFINE(data_node_to_proxy, NULL, 5);


// static const struct bt_mesh_model_op generic_onoff_op[] = {
// 		{BT_MESH_MODEL_OP_GENERIC_ONOFF_GET, 0, generic_onoff_get},
// 		{BT_MESH_MODEL_OP_GENERIC_ONOFF_SET, 2, generic_onoff_set},
// 		{BT_MESH_MODEL_OP_GENERIC_ONOFF_SET_UNACK, 2, generic_onoff_set_unack},
// 		BT_MESH_MODEL_OP_END,
// };

// model publication context
// BT_MESH_MODEL_PUB_DEFINE(generic_onoff_pub, NULL, 2 + 1);

static const struct bt_mesh_model_op data_from_power_node_op[] = {
    {BT_MESH_MODEL_OP_NODE_TO_PROXY_UNACK, 5, get_data_from_power_node},
    BT_MESH_MODEL_OP_END,
};

BT_MESH_MODEL_PUB_DEFINE(data_from_power_node_pub, NULL, 5);

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



