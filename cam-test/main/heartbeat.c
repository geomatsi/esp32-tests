#include "esp_log.h"
#include "esp_event.h"
#include "esp_timer.h"

#include "common.h"

static const char *TAG = "mod:hb";

static esp_timer_handle_t heartbeat_timer;

static void heartbeat_callback(void* arg)
{
	int64_t time_since_boot = esp_timer_get_time() / 1000000;

	ESP_LOGI(TAG, "Heartbeat: time since boot %lld sec", time_since_boot);
	ESP_ERROR_CHECK(esp_event_post(SYSTEM_EVENTS, SYSTEM_HEARTBEAT_EVENT, &time_since_boot,
			sizeof(time_since_boot), 0));
}

void heartbeat_task(void *args)
{
	esp_timer_create_args_t heartbeat_args = {
		.callback = &heartbeat_callback,
		.name = "heartbeat"
	};

	ESP_ERROR_CHECK(esp_timer_create(&heartbeat_args, &heartbeat_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(heartbeat_timer, 10000000));

	while (1) {
		vTaskDelay(1000);
	}
}
