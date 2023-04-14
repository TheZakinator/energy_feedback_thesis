#include <tizen.h>
#include <service_app.h>
#include "bluetooth_service.h"
#include <bluetooth.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <message_port.h>
#include <device/haptic.h>
#include <app_control.h>
#include <glib.h>

typedef enum {
	BT_ACTIVATE,
	BT_DEACTIVATE,
	EXIT_SERVICE,
	NORMAL_OPERATION,
	DO_NOTHING

} app_control_state_t;

typedef enum {
	BT_ENABLED_MSG,
	BT_DISABLED_MSG,
	NO_MSG
} service_to_app_msg_type_t;

app_control_state_t app_control_state = DO_NOTHING;
service_to_app_msg_type_t service_to_app_msg_type = NO_MSG;


void message_port_cb(int local_port_id, const char* remote_app_id, const char* remote_port,
				bool trusted_remote_port, bundle* message, void* user_data) {


	char* command = NULL;


	bundle_get_str(message, "command", &command);

	if (!strcmp(command, "SERVICE_ON")) {
		dlog_print(DLOG_INFO, "MESSAGE_PORT", "helloplease: i am ON :D");
		app_control_state = NORMAL_OPERATION;

	} else if (!strcmp(command, "SERVICE_OFF")) {
		dlog_print(DLOG_INFO, "MESSAGE_PORT", "helloplease: i am off :(");
		app_control_state = EXIT_SERVICE;

	} else if (!strcmp(command, "BT_ACTIVATED")) {
		dlog_print(DLOG_INFO, "MESSAGE_PORT", "helloplease: BT_ACTIVATE ");
		app_control_state = BT_ACTIVATE;

	} else if (!strcmp(command, "BT_DEACTIVATED")) {
		dlog_print(DLOG_INFO, "MESSAGE_PORT", "helloplease: BT_DEACTIVATED ");
		app_control_state = BT_DEACTIVATE;
	}
}

int bt_adapter_state() {
	bt_adapter_state_e adapterState;

	int ret = bt_adapter_get_state(&adapterState);
	if (ret != BT_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "[bt_adapter_get_state] failed");
		return -1;
	}

	if (adapterState == BT_ADAPTER_DISABLED) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Bluetooth adapter is not enabled.");
		return 1;
	}

	return 0;
}

int bt_onoff_operation(void) {
	int ret = 0;
	app_control_h service = NULL;
	app_control_create(&service);
	if (service == NULL) {
		dlog_print(DLOG_INFO, LOG_TAG, "service_create failed!\n");

		return 0;
	}
	app_control_set_operation(service, APP_CONTROL_OPERATION_SETTING_BT_ENABLE);
	ret = app_control_send_launch_request(service, NULL, NULL);

	app_control_destroy(service);
	if (ret == APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_INFO, LOG_TAG, "Succeeded to Bluetooth On/Off app!\n");

		return 0;
	} else {
		dlog_print(DLOG_INFO, LOG_TAG, "Failed to relaunch Bluetooth On/Off app!\n");

		return -1;
	}

	return 0;
}

void send_message(service_to_app_msg_type_t msgType) {
	int ret;
	bundle* b = bundle_create();
	switch (msgType) {
	case BT_ENABLED_MSG:
		bundle_add_str(b, "command", "BT_ENABLED");
		break;
	case BT_DISABLED_MSG:
		bundle_add_str(b, "command", "BT_DISABLED");
		break;
	case NO_MSG:
		break;
	}

	ret = message_port_send_message("t4klDIiLoQ.uiTest", "APP_PORT", b);
	if (ret != MESSAGE_PORT_ERROR_NONE) {
		dlog_print(DLOG_INFO, LOG_TAG, "helloplease: error sending msg with local port");
	} else {
		dlog_print(DLOG_INFO, LOG_TAG, "helloplease: msg sent successfully");
	}
	bundle_free(b);
}

bool service_app_create(void *data)
{
    // Todo: add your code here.
	haptic_device_h handle;
	haptic_effect_h effect;

	device_haptic_open(0, &handle);
	device_haptic_vibrate(handle, 500, 50, &effect);

	message_port_register_local_port("BT_SERVICE_PORT", message_port_cb, NULL);


	bt_error_e ret;
	ret = bt_initialize();
	if (ret != BT_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "[bt initialize] failed.");
	}

    return true;
}

void service_app_terminate(void *data)
{
    // Todo: add your code here.
		haptic_device_h handle;
		haptic_effect_h effect;

		device_haptic_open(1, &handle);
		device_haptic_vibrate(handle, 500, 50, &effect);
//		device_haptic_vibrate(handle, 1000, 20, &effect);
//		device_haptic_vibrate(handle, 1500, 50, &effect);
//		device_haptic_vibrate(handle, 3000, 75, &effect);

    return;
}

/**
 * https://docs.tizen.org/application/native/guides/connectivity/bluetooth/
 */
void adapter_device_discovery_state_changed_cb(int result, bt_adapter_device_discovery_state_e discovery_state,
				bt_adapter_device_discovery_info_s * discovery_info, void* user_data) {

	if (result != BT_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "bonjour failed! result(%d).", result);
		return;
	}

	GList** searched_device_list = (GList**)user_data;
	switch (discovery_state) {
	case BT_ADAPTER_DEVICE_DISCOVERY_STARTED:
		dlog_print(DLOG_INFO, LOG_TAG, "bonjour BT_ADAPTER_DEVICE_DISCOVERY_STARTED");
		break;
	case BT_ADAPTER_DEVICE_DISCOVERY_FINISHED:
		dlog_print(DLOG_INFO, LOG_TAG, "bonjour BT_ADAPTER_DEVICE_DISCOVERY_FINISHED");
		break;
	case BT_ADAPTER_DEVICE_DISCOVERY_FOUND:
		dlog_print(DLOG_INFO, LOG_TAG, "bonjour BT_ADAPTER_DEVICE_DISCOVERY_FOUND");
		if (discovery_info != NULL) {
			dlog_print(DLOG_INFO, LOG_TAG, "bonjour Device Address: %s", discovery_info->remote_address);
			dlog_print(DLOG_INFO, LOG_TAG, "bonjour Device Name is: %s", discovery_info->remote_name);
			bt_adapter_device_discovery_info_s * new_device_info = malloc(sizeof(bt_adapter_device_discovery_info_s));
			if (new_device_info != NULL) {
				memcpy(new_device_info, discovery_info, sizeof(bt_adapter_device_discovery_info_s));
				new_device_info->remote_address = strdup(discovery_info->remote_address);
				new_device_info->remote_name = strdup(discovery_info->remote_name);
				*searched_device_list = g_list_append(*searched_device_list, (gpointer)new_device_info);
			}
		}
		break;
	}

}


void __bt_adapter_le_scan_result_cb(int result, bt_adapter_le_device_scan_result_info_s *info, void *user_data) {

	int i;

    bt_adapter_le_packet_type_e pkt_type = BT_ADAPTER_LE_PACKET_ADVERTISING;

    if (info == NULL) {
        dlog_print(DLOG_INFO, LOG_TAG, "scanresult No discovery_info!");

        return;
    }

    if (info->adv_data_len > 31 || info->scan_data_len > 31) {
        dlog_print(DLOG_INFO, LOG_TAG, "scanresult Data length exceeds 31");
        bt_adapter_le_stop_scan();
        dlog_print(DLOG_INFO, LOG_TAG, "scanresult Scanning stopped");

        return;
    }

    for (i = 0; i < 2; i++) {
        char **uuids;
        char *device_name;
        int tx_power_level;
        bt_adapter_le_service_data_s *data_list;
        int appearance;
        int manufacturer_id;
        char *manufacturer_data;
        int manufacturer_data_len;
        int count;

        pkt_type += i;
        if (pkt_type == BT_ADAPTER_LE_PACKET_ADVERTISING && info->adv_data == NULL)
            continue;
        if (pkt_type == BT_ADAPTER_LE_PACKET_SCAN_RESPONSE && info->scan_data == NULL)
            break;

        if (bt_adapter_le_get_scan_result_service_uuids(info, pkt_type, &uuids, &count) == BT_ERROR_NONE) {
            int i;
            for (i = 0; i < count; i++) {
                dlog_print(DLOG_INFO, LOG_TAG, "scanresult UUID[%d] = %s", i + 1, uuids[i]);
                g_free(uuids[i]);
            }
            g_free(uuids);
        }
        if (bt_adapter_le_get_scan_result_device_name(info, pkt_type, &device_name) == BT_ERROR_NONE) {
            dlog_print(DLOG_INFO, LOG_TAG, "scanresult Device name = %s", device_name);
            g_free(device_name);
        }
        if (bt_adapter_le_get_scan_result_tx_power_level(info, pkt_type, &tx_power_level) == BT_ERROR_NONE)
            dlog_print(DLOG_INFO, LOG_TAG, "scanresult TX Power level = %d", tx_power_level);
        if (bt_adapter_le_get_scan_result_service_solicitation_uuids(info, pkt_type, &uuids, &count) == BT_ERROR_NONE) {
            int i;
            for (i = 0; i < count; i++) {
                dlog_print(DLOG_INFO, LOG_TAG, "scanresult Solicitation UUID[%d] = %s", i + 1, uuids[i]);
                g_free(uuids[i]);
            }
            g_free(uuids);
        }
        if (bt_adapter_le_get_scan_result_service_data_list(info, pkt_type, &data_list, &count) == BT_ERROR_NONE) {
            int i;
            for (i = 0; i < count; i++) {
                dlog_print(DLOG_INFO, LOG_TAG, "scanresult Service Data[%d] = [0x%2.2X%2.2X:0x%.2X...]", i + 1,
                           data_list[i].service_uuid[0], data_list[i].service_uuid[1], data_list[i].service_data[0]);
            }
            bt_adapter_le_free_service_data_list(data_list, count);
        }
        if (bt_adapter_le_get_scan_result_appearance(info, pkt_type, &appearance) == BT_ERROR_NONE)
            dlog_print(DLOG_INFO, LOG_TAG, "scanresult Appearance = %d", appearance);
        if (bt_adapter_le_get_scan_result_manufacturer_data(info, pkt_type, &manufacturer_id,
                                                            &manufacturer_data, &manufacturer_data_len) == BT_ERROR_NONE) {
            dlog_print(DLOG_INFO, LOG_TAG, "scanresult Manufacturer data[ID:%.4X, 0x%.2X%.2X...(len:%d)]",
                       manufacturer_id, manufacturer_data[0], manufacturer_data[1], manufacturer_data_len);
            g_free(manufacturer_data);
        }
    }
}



void service_app_control(app_control_h appControl, void *data) {
    // Todo: add your code here.

	int ret = 0;
	int btState;

	switch (app_control_state) {
	case BT_ACTIVATE:
		dlog_print(DLOG_INFO, LOG_TAG, "helloplease: app control -> activate");

		ret = bt_onoff_operation();
		// check if user enabled bluetooth, then send result to UI app.
		btState = bt_adapter_state();
		dlog_print(DLOG_INFO, LOG_TAG, "helloplease: BT STATE: %d", btState);

		if (btState == 0) {
			send_message(BT_ENABLED_MSG);
		} else if (btState == 1){
			send_message(BT_DISABLED_MSG);
		}

		break;

	case BT_DEACTIVATE:
		dlog_print(DLOG_INFO, LOG_TAG, "helloplease: app control -> deactivate");
		ret = bt_onoff_operation();
		// check if user enabled bluetooth, then send result to UI app.
		btState = bt_adapter_state();

		if (btState == 0) {
			send_message(BT_ENABLED_MSG);
		} else if (btState == 1){
			send_message(BT_DISABLED_MSG);
		}
		break;

	case EXIT_SERVICE:
		service_app_exit();
		break;

	case NORMAL_OPERATION:
		if (bt_adapter_state() == 0) {
			// if bluetooth is enabled -> check to see if device has been discovered
			GList* devices_list = NULL;
			ret = bt_adapter_set_device_discovery_state_changed_cb(adapter_device_discovery_state_changed_cb, (void*)&devices_list);
			if (ret != BT_ERROR_NONE) {
				dlog_print(DLOG_ERROR, LOG_TAG, "bonjour [bt_adapter_set_device_discovery_state_changed_cb] failed.");
			}

			bt_adapter_le_start_scan(__bt_adapter_le_scan_result_cb, NULL);

			if (ret != BT_ERROR_NONE) {
				dlog_print(DLOG_ERROR, LOG_TAG, "bonjour [bt_adapter_start_device_discovery] failed.");
			}

		}
		break;

	case DO_NOTHING:
		break;

	default:
		break;

	}

	app_control_state = DO_NOTHING; // reset control state


    return;
}

static void
service_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	return;
}

static void
service_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
service_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
service_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int main(int argc, char* argv[])
{
    char ad[50] = {0,};
	service_app_lifecycle_callback_s event_callback;
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = service_app_create;
	event_callback.terminate = service_app_terminate;
	event_callback.app_control = service_app_control;

	service_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, service_app_low_battery, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, service_app_low_memory, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, service_app_lang_changed, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, service_app_region_changed, &ad);

	return service_app_main(argc, argv, &event_callback, ad);
}
