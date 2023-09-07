/* This example code is in the Public Domain (or CC0 licensed, at your option.)
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */

#include <string.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "common.h"

#define DEFAULT_SCAN_LIST_SIZE     CONFIG_EXAMPLE_SCAN_LIST_SIZE
#define DEFAULT_WIFI_SSID          CONFIG_EXAMPLE_WIFI_SSID
#define DEFAULT_WIFI_PASS          CONFIG_EXAMPLE_WIFI_PASS
#define DEFAULT_STACK_SIZE         CONFIG_ESP_MAIN_TASK_STACK_SIZE

ESP_EVENT_DEFINE_BASE(SYSTEM_EVENTS);

static const char *TAG = "mod:main";

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
			       int32_t event_id, void *event_data)
{
	if (event_base != WIFI_EVENT) {
		ESP_LOGE(TAG, "%s: unexpected event_base: %s\n", __func__, event_base);
		return;
	}

	switch (event_id) {
	case WIFI_EVENT_SCAN_DONE:
		wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
		uint16_t number = DEFAULT_SCAN_LIST_SIZE;
		uint16_t ap_count = 0;

		memset(ap_info, 0, sizeof(ap_info));

		ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
		ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
		ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);

		for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
			ESP_LOGI(TAG, "AP: SSID[%s] RSSI[%d] CHAN[%d]\n",
					ap_info[i].ssid, ap_info[i].rssi, ap_info[i].primary);
		}

		ESP_LOGI(TAG, "STA scan completed...  Connecting...");
        	esp_wifi_connect();
		break;
	case WIFI_EVENT_STA_START:
		ESP_LOGI(TAG, "STA started...  Scanning...");
		esp_wifi_scan_start(NULL, true);
		break;
	case WIFI_EVENT_STA_DISCONNECTED:
		wifi_event_sta_disconnected_t * event = (wifi_event_sta_disconnected_t *) event_data;
		ESP_LOGI(TAG, "STA disconnected with reason %d...  Trying to reconnect...", event->reason);
		esp_wifi_connect();
		break;
	default:
		ESP_LOGI(TAG, "Unhandled event: %s:%ld\n", event_base, event_id);
		break;
	}
}

void app_main(void)
{
	/* initialize nvs */

	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}

	ESP_ERROR_CHECK( ret );

	/* init camera */

	camera_init();

	/* init wifi */ 

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
	assert(sta_netif);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
			ESP_EVENT_ANY_ID,
			&wifi_event_handler,
			NULL,
			NULL));

	/* init sidecar tasks */

	xTaskCreate(heartbeat_task, "heartbeat_task", DEFAULT_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(http_task, "http_task", DEFAULT_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

	/* start wifi */

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = DEFAULT_WIFI_SSID,
			.password = DEFAULT_WIFI_PASS,
		},
	};

	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_start() );
}
