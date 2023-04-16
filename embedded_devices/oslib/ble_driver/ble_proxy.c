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

// initialises semaphore for data which goes between GATT and BLE mesh threads
K_SEM_DEFINE(power_data_sem, 1, 1);

typedef struct {
    uint8_t applianceOn;
    uint8_t hysterisisLevel;
    uint8_t nodeNum;
    uint32_t voltageVal;
} PowerNodeData_t;

PowerNodeData_t powerNodeData[3];
PowerNodeData_t savedNodeData[3];

int sendingNode = 0; // power node data currently being notified

// initialise node data to 0 and wait indefinitely for semaphore
void init_node_data() {

    printk("\ninitialise data semaphore TAKE\n");

    if (k_sem_take(&power_data_sem, K_FOREVER) != 0) {
        printk("ERROR: Could not take semepahore : init_node_data\n");
    } else {

        for (int i = 0; i < 3; i++) {
            powerNodeData[i].applianceOn = i*1;
            powerNodeData[i].hysterisisLevel = i*5;
            powerNodeData[i].nodeNum = i;
            powerNodeData[i].voltageVal = (i+1) * 140000000;

            savedNodeData[i].applianceOn = i*1;
            savedNodeData[i].hysterisisLevel = i*5;
            savedNodeData[i].nodeNum = i;
            savedNodeData[i].voltageVal = (i+1) * 140000000;

        }

        k_sem_give(&power_data_sem);
    }

    printk("\ninitialised data semaphore RETURN\n");
}

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

// int16_t data_request[3];

// static ssize_t write_chrc(struct bt_conn *conn,
// 			       const struct bt_gatt_attr *attr,
// 			       const void *buf, uint16_t len,
// 			       uint16_t offset, uint8_t flags)
// {
// 	printk("chrc len %u offset %u\n", len, offset);
//     // struct bt_gatt_chrc* chrc = (struct bt_gatt_chrc*) attr->user_data;
//     // chrc_handle = chrc->value_handle;
// 	memcpy(data_request, buf, len);

// 	printk("\nOne: %d\n", data_request[0]);
//     printk("ID: %d\n", data_request[1]);
//     printk("value: %d\n", data_request[2]);
// 	return len;
// }

static ssize_t read_chrc(struct bt_conn* conn, 
                const struct bt_gatt_attr* attr,
                void* buf, uint16_t len, uint16_t offset) {

    printk("read request received\n");

    // copy all savedData and transmit it. If semaphore cannot be 
    // acquired, send old data instead.
    if (k_sem_take(&power_data_sem, K_MSEC(100)) != 0) {
        printk("ERROR: Could not take semepahore : read_chrc\n");
    } else {
        memcpy(&savedNodeData[0], &powerNodeData[0], sizeof(savedNodeData[0]));
        k_sem_give(&power_data_sem);
    }


    return bt_gatt_attr_read(conn, attr, buf, len, offset, 
                        (void*) &savedNodeData[0], sizeof(savedNodeData[0]));               
}

// static void notify_func(struct bt_conn* conn, 
//                 const struct bt_gatt_attr* attr,
//                 uint16_t value) 
// {

// }

// static struct bt_uuid_128 proxy_desc = BT_UUID_INIT_128(
//     0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00);

static struct bt_uuid_128 proxy_desc = BT_UUID_INIT_128(
0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 
0x00, 0x10, 0x00, 0x00, 0x02, 0x29, 0x00, 0x00);


static bool en_notification;

static void proxy_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	en_notification = value == BT_GATT_CCC_NOTIFY;
    printk("proxy_ccc_cfg_changed: %d\n", value);
}

uint16_t proxy_desc_val = 0x0000;

static ssize_t read_proxy_desc(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{

    printk("read_proxy_desc\n");
    printk("len: %d\n", len);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, (void*) &proxy_desc_val,
				 sizeof(proxy_desc_val));
}

static ssize_t write_proxy_ccc(struct bt_conn* conn, 
                    const struct bt_gatt_attr* attr, const void* buf, 
                    uint16_t len, uint16_t offset, uint8_t flags) 
{
    const uint16_t value = *(uint16_t *)buf;

    if (len != sizeof(value)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    en_notification = value == BT_GATT_CCC_NOTIFY;
    printk("proxy_ccc_cfg_changed: %d\n", value);

    return len;

}

BT_GATT_SERVICE_DEFINE(proxy_svc,

                    BT_GATT_PRIMARY_SERVICE(&proxy_uuid),

                    BT_GATT_CHARACTERISTIC(&watch_uuid.uuid,
                                            BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                                            BT_GATT_PERM_READ,
                                            read_chrc, NULL, &powerNodeData[0]),

                    BT_GATT_DESCRIPTOR(BT_UUID_GATT_CCC, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                                            NULL, write_proxy_ccc, &proxy_desc_val)                
                    );

void change_data_intermittently() {

    int notify_result;

    if (k_sem_take(&power_data_sem, K_MSEC(100)) != 0) {
        printk("ERROR: Could not take semepahore : change_data\n");
    } else {
        memcpy(&savedNodeData[sendingNode], &powerNodeData[sendingNode], sizeof(savedNodeData[0]));

        // check if remote GATT client has enabled notifications
        if (en_notification) {
            printk("notifying...\n");
            notify_result = bt_gatt_notify(default_conn, &proxy_svc.attrs[2], 
                &powerNodeData[sendingNode], sizeof(powerNodeData[sendingNode]));
            
            if (notify_result < 0) {
                printk("Notification error: %d\n", notify_result);
            } else {
                printk("Notification sent\n");
            }
        }

        // cycle through connected power nodes
        sendingNode++;
        if (sendingNode == 3) {
            sendingNode = 0;
        }

        k_sem_give(&power_data_sem);
    }    

    // // check if remote GATT client has enabled notifications
    // if (en_notification) {
    //     printk("notifying...\n");
    //     notify_result = bt_gatt_notify(default_conn, &proxy_svc.attrs[2], 
    //         &powerNodeData[0], sizeof(powerNodeData[0]));
        
    //     if (notify_result < 0) {
    //         printk("Notification error: %d\n", notify_result);
    //     } else {
    //         printk("Notification sent\n");
    //     }
    // }
}

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
        struct bt_le_conn_param *param = BT_LE_CONN_PARAM(20, 200, 0, 400);

        default_conn = conn;
 
        if (bt_conn_le_param_update(conn, param) < 0)
        {
			printk("ERROR - connected() - should not reach here\n");
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
    en_notification = false; // reset notifications
    ble_connected= false; // reset connected status
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
	bt_conn_cb_register(&conn_callbacks);
    // bt_conn_auth_cb_register(&auth_cb_display);
}

// ======================== Miscellaneous ============================================//
static void attention_on(struct bt_mesh_model *model) { printk("attention_on()\n"); }

static void attention_off(struct bt_mesh_model *model) { printk("attention_off()\n"); }

// device UUID
static uint8_t dev_uuid[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,0x09, 0x10, 0x11, 0x12,0x13, 0x14, 0x15, 0x16 };

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


// ======================== Generic model definitions ============================================//

// 10 bytes + 1
BT_MESH_MODEL_PUB_DEFINE(data_node_to_proxy, NULL, 5);

static const struct bt_mesh_model_op data_from_power_node_op[] = {
    {BT_MESH_MODEL_OP_NODE_TO_PROXY_UNACK, 7, get_data_from_power_node},
    BT_MESH_MODEL_OP_END,
};

BT_MESH_MODEL_PUB_DEFINE(data_from_power_node_pub, NULL, 7);

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

    PowerNodeData_t tempData;

    printk("get data from power node\n");

    tempData.applianceOn = net_buf_simple_pull_u8(buf);
    tempData.hysterisisLevel = net_buf_simple_pull_u8(buf);
    tempData.nodeNum = net_buf_simple_pull_u8(buf);

    tempData.voltageVal = net_buf_simple_pull_le32(buf);

    if (k_sem_take(&power_data_sem, K_MSEC(50)) != 0) {
        printk("ERROR: Could not take semepahore : get_data_from_power_node\n");
    } else {
        // copy one instance of PowerNodeData_t with size of one instance
        memcpy(&powerNodeData[tempData.nodeNum], &tempData, sizeof(powerNodeData[0]));

        for (int i = 0; i < 3; i++) {
            
            printk("\nreceived power node data: \n");
            printk("    applianceOn[%d] : %u\n", i, powerNodeData[i].applianceOn);
            printk("hysterisisLevel[%d] : %u\n", i, powerNodeData[i].hysterisisLevel);
            printk("        nodeNum[%d] : %u\n", i, powerNodeData[i].nodeNum);
            printk("     voltageVal[%d] : %u\n", i, powerNodeData[i].voltageVal);
        }
        k_sem_give(&power_data_sem);
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