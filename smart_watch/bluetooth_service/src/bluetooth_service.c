#include <tizen.h>
#include <service_app.h>
#include "bluetooth_service.h"
#include <stdio.h>
#include <string.h>

#include <message_port.h>

#include <device/haptic.h>

void message_port_cb(int local_port_id, const char* remote_app_id, const char* remote_port,
				bool trusted_remote_port, bundle* message, void* user_data) {

	haptic_device_h handle;
	haptic_effect_h effect;

	dlog_print(DLOG_INFO, "MESSAGE_PORT", "hello");

//	char* key = NULL;
//	char* data = NULL;
	char* command = NULL;
	char* data = NULL;
	char* byteData = NULL;

	bundle_get_str(message, "command", &command);
	bundle_get_str(message, "data", &data);
	bundle_get_str(message, "byteData", &byteData);

//	bundle_get_str(message, "key", &key);
//	bundle_get_str(message, "value", &data);

	dlog_print(DLOG_INFO, "MESSAGE_PORT", "helloplease: %s %s %s", command, data, byteData);


	/* TODO: add reply functionality to send data back to main App
	bundle* reply = bundle_create();
	bundle_add_str(reply, "result", "got it, yay?");
	ret = message_port_send_message(remote_app_id, remote_port, reply);
	bundle_free(reply);
	*/


}

bool service_app_create(void *data)
{
    // Todo: add your code here.
	message_port_register_local_port("BT_SERVICE_PORT", message_port_cb, NULL);

    return true;
}

void service_app_terminate(void *data)
{
    // Todo: add your code here.
    return;
}

void service_app_control(app_control_h app_control, void *data)
{
    // Todo: add your code here.
//	haptic_device_h handle;
//	haptic_effect_h effect;
//
//	device_haptic_open(0, &handle);
//	device_haptic_vibrate(handle, 100, 10, &effect);
//	device_haptic_vibrate(handle, 1000, 20, &effect);
//	device_haptic_vibrate(handle, 1500, 50, &effect);
//	device_haptic_vibrate(handle, 3000, 75, &effect);

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
