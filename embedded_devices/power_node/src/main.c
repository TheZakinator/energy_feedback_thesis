#include <stdlib.h>
#include <stdint.h>
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

#define GPIO0_PORT "GPIO_0"
#define RED_LED_PIN 15
#define YELLOW_LED_PIN 16
#define GREEN_LED_PIN 17
#define BTN_PIN 18

#define DEBOUNCE_DELAY_MS 100

static int64_t lastIntTime = 0;
int powerNodeState = 0;

static struct gpio_callback button_cb;
static gpio_callback_handler_t button_pressed(struct device *dev, struct gpio_callback *cb, uint32_t pins);

static gpio_callback_handler_t button_pressed(struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
  int64_t currentTime = k_uptime_get();
	printk("currentTime: %lld\n", currentTime);

	if (currentTime - lastIntTime >= DEBOUNCE_DELAY_MS) {
		const struct device* gpio_dev = device_get_binding(GPIO0_PORT);
		if (gpio_dev != NULL) {
			if (powerNodeState == 0) {
				printk("state 0\n");
				gpio_pin_set(gpio_dev, RED_LED_PIN, 0);
				gpio_pin_set(gpio_dev, YELLOW_LED_PIN, 0);
				gpio_pin_set(gpio_dev, GREEN_LED_PIN, 0);
			} else if (powerNodeState == 1) {
				printk("state 1\n");
				gpio_pin_set(gpio_dev, RED_LED_PIN, 1);
				gpio_pin_set(gpio_dev, YELLOW_LED_PIN, 0);
				gpio_pin_set(gpio_dev, GREEN_LED_PIN, 0);
			} else if (powerNodeState == 2) {
				printk("state 2\n");
				gpio_pin_set(gpio_dev, RED_LED_PIN, 0);
				gpio_pin_set(gpio_dev, YELLOW_LED_PIN, 1);
				gpio_pin_set(gpio_dev, GREEN_LED_PIN, 0);
			} else if (powerNodeState == 3) {
				printk("state 3\n");
				gpio_pin_set(gpio_dev, RED_LED_PIN, 0);
				gpio_pin_set(gpio_dev, YELLOW_LED_PIN, 0);
				gpio_pin_set(gpio_dev, GREEN_LED_PIN, 1);
			}
		}
		powerNodeState += 1;
		if (powerNodeState == 4) {
			powerNodeState = 0;
		}
	}

	lastIntTime = currentTime;
}

void get_dummy_data(const char* deviceName, PowerNodeData_t* powerNodeData, int powerState) {

	uint32_t voltageVal = 0x0000;

	if (powerState == 1) {
		// appliance standby
		powerNodeData->voltageVal = 0;
	} else if (powerState == 2) {
		// appliance on: low power
		powerNodeData->voltageVal = 0x00FFFFFF;
	} else if (powerState == 3) {
		// appliance on: high power
		powerNodeData->voltageVal = 0xFF000000;
	}

	powerNodeData->applianceOn = 1;
	powerNodeData->hysterisisLevel = 1;

	if (!strcmp(deviceName, "PWR_NODE_AIRCON")) {
		powerNodeData->nodeNum = 0;

	} else if (!strcmp(deviceName, "PWR_NODE_TV")) {
		powerNodeData->nodeNum = 1;

	} else if (!strcmp(deviceName, "PWR_NODE_WATER_HEATER")) {
		powerNodeData->nodeNum = 2;

	} else {
		printk("unknown name, get_dummy_data error\n");
	}
}

void main(void) {

	PowerNodeData_t powerNodeData;
	int powerState = 1;

	printk("esp32 start! -- power node\n");
	int err = bt_enable(bt_ready);
	if (err) {
		printk("bt_enable failed with err %d", err);
	}
	printk("bonjour\n");
	const char* btName = bt_get_name();

	const struct device* gpio_dev = device_get_binding(GPIO0_PORT);
	bool ledState = 0;

	if (gpio_dev == NULL) {
		printk("failed to get gpio device\n");
	} else {
		gpio_pin_configure(gpio_dev, RED_LED_PIN, GPIO_OUTPUT);
		gpio_pin_configure(gpio_dev, YELLOW_LED_PIN, GPIO_OUTPUT);
		gpio_pin_configure(gpio_dev, GREEN_LED_PIN, GPIO_OUTPUT);
		gpio_pin_configure(gpio_dev, BTN_PIN, GPIO_INPUT | GPIO_PULL_UP);

		err = gpio_pin_interrupt_configure(gpio_dev, BTN_PIN, GPIO_INT_EDGE_FALLING);
		if (errno != 0) {
			printk("Error %d: failed to configure interrupt\n", err);
			return;
		}

		// setup pushbutton callback
		gpio_init_callback(&button_cb, button_pressed, BIT(BTN_PIN));
		gpio_add_callback(gpio_dev, &button_cb);

	}

	// bt_mesh_reset(); -- uncomment to unprovision device

	while (1) {
		if (bt_mesh_is_provisioned()) {

			// printk("1 received power node data: \n");
			// printk("1 applianceOn: %u\n", powerNodeData.applianceOn);
			// printk("1 hysterisisLevel: %u\n", powerNodeData.hysterisisLevel);
			// printk("1 nodeNum: %u\n", powerNodeData.nodeNum);
			// printk("1 voltageVal: %u\n", powerNodeData.voltageVal);

			get_dummy_data(btName, &powerNodeData, powerNodeState);

			// indicates that the device has disconnected and shouldnt be sending any data
			if (powerNodeState != 0) {
				send_data_to_proxy(BT_MESH_MODEL_OP_NODE_TO_PROXY_UNACK, &powerNodeData);
			}

			printk("proxy data sent\n");
		}

		k_msleep(5000);
	}
}
