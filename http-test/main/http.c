#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_http_server.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "common.h"

#define HTTP_RESP_SIZE 1024

static const char *TAG = "wifi-http";
static httpd_handle_t server = NULL;
static char heartbeat_message[64];
static char resp[HTTP_RESP_SIZE];

static esp_err_t test_get_handler(httpd_req_t *req)
{
	esp_chip_info_t chip_info;
	unsigned int minor_rev;
	unsigned int major_rev;
	uint32_t flash_size;
	size_t len = 0;

	ESP_LOGI(TAG, "%s: sending response for uri '%s'", __func__, req->uri);

	memset(resp, 0, sizeof(resp));
	esp_chip_info(&chip_info);

	len += snprintf(resp + len, HTTP_RESP_SIZE - len, "This is %s chip with %d CPU core(s), WiFi%s%s%s<br>",
		        CONFIG_IDF_TARGET,
		        chip_info.cores,
		        (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
		        (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
		        (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

	major_rev = chip_info.revision / 100;
	minor_rev = chip_info.revision % 100;
	
	len += snprintf(resp + len, HTTP_RESP_SIZE - len, "silicon revision v%d.%d<br>",
			major_rev, minor_rev);

	if(esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
		len += snprintf(resp + len, HTTP_RESP_SIZE - len, "%" PRIu32 "MB %s flash<br>",
				flash_size / (uint32_t)(1024 * 1024),
				(chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
	}

	len += snprintf(resp + len, HTTP_RESP_SIZE - len, "Minimum free heap size: %" PRIu32 " bytes<br>",
			esp_get_minimum_free_heap_size());

	httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

	return ESP_OK;
}

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
	memset(resp, 0, sizeof(resp));
	snprintf(resp, HTTP_RESP_SIZE, "URI %s is not available", req->uri);

	httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, resp);
	return ESP_FAIL;
}

static const httpd_uri_t test = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = test_get_handler,
};

static httpd_handle_t start_http_server(void)
{
	httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
	httpd_handle_t srv = NULL;

	cfg.lru_purge_enable = true;

	ESP_LOGI(TAG, "%s: starting http server on port: '%d'", __func__, cfg.server_port);

	if (httpd_start(&srv, &cfg) == ESP_OK) {
		httpd_register_uri_handler(srv, &test);
        	httpd_register_err_handler(srv, HTTPD_404_NOT_FOUND, http_404_error_handler);
	}

	return srv;
}

static esp_err_t stop_http_server(httpd_handle_t srv)
{
	return httpd_stop(srv);
}

static void connect_handler(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data)
{
	ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;

	ESP_LOGI(TAG, "STA got ipaddr:" IPSTR, IP2STR(&event->ip_info.ip));

	server = start_http_server();
	if (!server)
		ESP_LOGI(TAG, "Error starting server!");
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
		if (stop_http_server(server) == ESP_OK) {
			server = NULL;
		} else {
			ESP_LOGE(TAG, "Failed to stop http server");
		}
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
		snprintf(heartbeat_message, sizeof(heartbeat_message), "heartbeat: %lld sec", heartbeat);
		break;
	default:
		ESP_LOGI(TAG, "Unhandled event: %s:%ld\n", event_base, event_id);
		break;
	}
}

void http_task(void *args)
{
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

	ESP_ERROR_CHECK(esp_event_handler_instance_register(SYSTEM_EVENTS,
			ESP_EVENT_ANY_ID,
			&system_event_handler,
			NULL,
			NULL));

	while (1) {
		vTaskDelay(1000);
	}
}
