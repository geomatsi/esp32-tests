/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

#define STACK_SIZE 3584

struct blink_led {
	gpio_num_t pin;
	uint32_t period_ms;
	char *name;
};

struct blink_led leds[] = {
	{
		.pin = CONFIG_GPIO_LED_RED,
		.period_ms = CONFIG_BLINK_PERIOD * 1,
		.name = "RED",
	},
	{
		.pin = CONFIG_GPIO_LED_BLUE,
		.period_ms = CONFIG_BLINK_PERIOD * 2,
		.name = "BLUE",
	},
	{
		.pin = CONFIG_GPIO_LED_GREEN,
		.period_ms = CONFIG_BLINK_PERIOD * 3,
		.name = "GREEN",
	},
};

static const char *TAG = "blink";

void vTaskLed(void *pvParameters)
{
	struct blink_led *led = (struct blink_led *)pvParameters;
	uint8_t state = 0;

	gpio_reset_pin(led->pin);
	gpio_set_direction(led->pin, GPIO_MODE_OUTPUT);

	for ( ;; ) {
		ESP_LOGI(TAG, "Turning the %s LED %s!", led->name, state ? "ON" : "OFF");
		gpio_set_level(led->pin, state);
		state = !state;
		vTaskDelay(led->period_ms / portTICK_PERIOD_MS);

	}
}

void app_main(void)
{
	TaskHandle_t xGreenHandle = NULL;
	TaskHandle_t xBlueHandle = NULL;
	TaskHandle_t xRedHandle = NULL;
	esp_chip_info_t chip_info;
	unsigned int minor_rev;
	unsigned int major_rev;
	uint32_t flash_size;

	/* Print ESP32 chip information */
	esp_chip_info(&chip_info);
	ESP_LOGI(TAG, "This is %s chip with %d CPU core(s), WiFi%s%s%s, ",
			CONFIG_IDF_TARGET,
			chip_info.cores,
			(chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
			(chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
			(chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

	major_rev = chip_info.revision / 100;
	minor_rev = chip_info.revision % 100;
	ESP_LOGI(TAG, "silicon revision v%d.%d, ", major_rev, minor_rev);
	if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
		ESP_LOGW(TAG, "Get flash size failed");
		return;
	}

	ESP_LOGI(TAG, "%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
			(chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
	ESP_LOGI(TAG, "Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

	/* Create LED tasks */

	xTaskCreate(vTaskLed, "RedLED", STACK_SIZE, &leds[0], tskIDLE_PRIORITY, &xRedHandle);
	if (!xRedHandle) {
		ESP_LOGE(TAG, "Failed to create task to blink red LED");
	}

	xTaskCreate(vTaskLed, "BlueLED", STACK_SIZE, &leds[1], tskIDLE_PRIORITY, &xBlueHandle);
	if (!xBlueHandle) {
		ESP_LOGE(TAG, "Failed to create task to blink blue LED");
	}

	xTaskCreate(vTaskLed, "GreenLED", STACK_SIZE, &leds[2], tskIDLE_PRIORITY, &xGreenHandle);
	if (!xGreenHandle) {
		ESP_LOGE(TAG, "Failed to create task to blink green LED");
	}

	/* Main blink cycle */
	while (1) {
		vTaskDelay(CONFIG_BLINK_PERIOD * 5 / portTICK_PERIOD_MS);
		ESP_LOGI(TAG, "....");
	}
}
