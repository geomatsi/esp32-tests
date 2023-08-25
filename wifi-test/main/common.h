#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(SYSTEM_EVENTS);

enum {
	SYSTEM_HEARTBEAT_EVENT,
	SYSTEM_BUTTON_EVENT,
};

void heartbeat_task(void *args);
void button_task(void *args);
void mqtt_task(void *args);
