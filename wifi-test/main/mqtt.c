#include "esp_log.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "mqtt_client.h"

#include "common.h"

#define DEFAULT_MQTT_BROKER_URL   CONFIG_EXAMPLE_BROKER_URL

extern const char *TAG;

static esp_mqtt_client_handle_t client;
static esp_timer_handle_t mqtt_tmr1;
static char tmr1_message[64];

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_mqtt_event_handle_t event = event_data;
	esp_mqtt_client_handle_t client = event->client;
	int msg_id;

	switch ((esp_mqtt_event_id_t)event_id) {
	case MQTT_EVENT_CONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		msg_id = esp_mqtt_client_publish(client, "/topic/test", "connected", 0, 1, 0);
		ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
		break;
	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
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

static void mqtt_tmr1_callback(void* arg)
{
	int64_t time_since_boot = esp_timer_get_time();

	ESP_LOGI(TAG, "Periodic mqtt event: time since boot %lld us", time_since_boot);
	snprintf(tmr1_message, sizeof(tmr1_message), "%lld", time_since_boot);
	esp_mqtt_client_publish(client, "/topic/test", tmr1_message, 0, 1, 0);
}

void mqtt_app_init(void)
{
	esp_mqtt_client_config_t mqtt_cfg = {
		.broker.address.uri = DEFAULT_MQTT_BROKER_URL,
	};

	client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

	esp_timer_create_args_t mqtt_tmr1_args = {
		.callback = &mqtt_tmr1_callback,
		.name = "mqtt-tmr1"
	};

	ESP_ERROR_CHECK(esp_timer_create(&mqtt_tmr1_args, &mqtt_tmr1));
}

void mqtt_app_start(void)
{
	esp_mqtt_client_start(client);
	ESP_ERROR_CHECK(esp_timer_start_periodic(mqtt_tmr1, 5000000));
}

void mqtt_app_stop(void)
{
	ESP_ERROR_CHECK(esp_timer_stop(mqtt_tmr1));
	esp_mqtt_client_stop(client);
}
