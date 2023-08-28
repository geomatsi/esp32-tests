#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(SYSTEM_EVENTS);

enum {
	SYSTEM_HEARTBEAT_EVENT,
};

void heartbeat_task(void *args);
void http_task(void *args);
