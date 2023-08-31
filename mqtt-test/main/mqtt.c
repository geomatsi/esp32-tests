#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "mqtt_client.h"

#include "common.h"

#define DEFAULT_MQTT_BROKER_URL   CONFIG_EXAMPLE_BROKER_URL

static const char *TAG = "wifi-mqtt";

static esp_mqtt_client_handle_t client;
static char heartbeat_message[64];
static char button_message[64];
static bool mqtt_active = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_mqtt_event_handle_t event = event_data;
	esp_mqtt_client_handle_t client = event->client;
	int msg_id;

	switch ((esp_mqtt_event_id_t)event_id) {
	case MQTT_EVENT_CONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		mqtt_active = true;
		msg_id = esp_mqtt_client_publish(client, "/topic/test", "READY", 0, 1, 0);
		ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
		break;
	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		mqtt_active = false;
		break;
	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_ERROR:
		ESP_LOGI(TAG, "MQTT_EVENT_ERROR type %d", event->error_handle->error_type);
		break;
	default:
		ESP_LOGI(TAG, "Other event id:%d", event->event_id);
		break;
	}
}

static void connect_handler(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data)
{
	ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;

	ESP_LOGI(TAG, "STA got ipaddr:" IPSTR, IP2STR(&event->ip_info.ip));
	esp_mqtt_client_start(client);
}

static void disconnect_handler(void *arg, esp_event_base_t event_base,
			       int32_t event_id, void *event_data)
{
	if (event_base != WIFI_EVENT) {
		ESP_LOGE(TAG, "%s: unexpected event_base: %s\n", __func__, event_base);
		return;
	}

	switch (event_id) {
	case WIFI_EVENT_STA_DISCONNECTED:
		esp_mqtt_client_stop(client);
		break;
	default:
		ESP_LOGI(TAG, "Unhandled event: %s:%ld\n", event_base, event_id);
		break;
	}
}

static void system_event_handler(void *arg, esp_event_base_t event_base,
				 int32_t event_id, void *event_data)
{
	if (event_base != SYSTEM_EVENTS) {
		ESP_LOGE(TAG, "%s: unexpected event_base: %s\n", __func__, event_base);
		return;
	}

	switch (event_id) {
	case SYSTEM_HEARTBEAT_EVENT:
		int64_t heartbeat = *((int64_t *) event_data);

		if (mqtt_active) {
			snprintf(heartbeat_message, sizeof(heartbeat_message), "heartbeat: %lld sec", heartbeat);
			esp_mqtt_client_publish(client, "/topic/test", heartbeat_message, 0, 1, 0);
		}
		break;
	case SYSTEM_BUTTON_EVENT:
		if (mqtt_active) {
			uint32_t level = *((uint32_t *) event_data);
			snprintf(button_message, sizeof(button_message), "button: %lu", level);
			esp_mqtt_client_publish(client, "/topic/test", button_message, 0, 1, 0);
		}
		break;
	default:
		ESP_LOGI(TAG, "Unhandled event: %s:%ld\n", event_base, event_id);
		break;
	}
}

void mqtt_task(void *args)
{
	esp_mqtt_client_config_t mqtt_cfg = {
		.broker.address.uri = DEFAULT_MQTT_BROKER_URL,
	};

	client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
			IP_EVENT_STA_GOT_IP,
			&connect_handler,
			NULL,
			NULL));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
			WIFI_EVENT_STA_DISCONNECTED,
			&disconnect_handler,
			NULL,
			NULL));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
			WIFI_EVENT_STA_DISCONNECTED,
			&disconnect_handler,
			NULL,
			NULL));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(SYSTEM_EVENTS,
			ESP_EVENT_ANY_ID,
			&system_event_handler,
			NULL,
			NULL));

	while (1) {
		vTaskDelay(1000);
	}
}
