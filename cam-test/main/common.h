#include "esp_event.h"

void heartbeat_task(void *args);
void http_task(void *args);

esp_err_t camera_init(void);
esp_err_t camera_capture(char *filepath);
