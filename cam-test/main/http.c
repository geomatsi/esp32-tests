#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_http_server.h"
#include "esp_chip_info.h"
#include "esp_spiffs.h"
#include "esp_camera.h"
#include "esp_flash.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_vfs.h"

#include "common.h"

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)
#define HTTP_RESP_SIZE 65536

static const char* base_path = "/storage";
static const char *TAG = "mod:http";

static httpd_handle_t server = NULL;
static char resp[HTTP_RESP_SIZE];

static esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
	memset(resp, 0, sizeof(resp));
	snprintf(resp, HTTP_RESP_SIZE, "URI %s is not available", req->uri);

	httpd_resp_send_err(req, err, resp);
	return ESP_FAIL;
}

static esp_err_t main_redirect_handler(httpd_req_t *req)
{
	httpd_resp_set_status(req, "307 Temporary Redirect");
	httpd_resp_set_hdr(req, "Location", "/index.html");
	httpd_resp_send(req, NULL, 0);
	return ESP_OK;
}

static esp_err_t shot_post_handler(httpd_req_t *req)
{
	char filepath[FILE_PATH_MAX];
	esp_err_t ret;

	ESP_LOGI(TAG, "%s: requested uri '%s'", __func__, req->uri);

	strcpy(filepath, base_path);
	strlcat(filepath, "/shot.jpeg", sizeof(filepath));

	ret = camera_capture(filepath);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to capture picture to file");
		goto done;
	}

done:
	/* redirect onto root to see the new picture */
	httpd_resp_set_status(req, "303 See Other");
	httpd_resp_set_hdr(req, "Location", "/index.html");
	httpd_resp_sendstr(req, "Picture taken successfully");
	return ESP_OK;
}
 
static esp_err_t main_get_handler(httpd_req_t *req)
{
	char filepath[FILE_PATH_MAX];
	struct stat file_stat;
	FILE *fd = NULL;
	size_t size;

	strcpy(filepath, base_path);
	strlcat(filepath, req->uri, sizeof(filepath));

	ESP_LOGI(TAG, "%s: constructed filepath '%s'", __func__, filepath);
	ESP_LOGI(TAG, "%s: requested uri '%s'", __func__, req->uri);

	if (stat(filepath, &file_stat) == -1) {
		if (strcmp(req->uri, "/") == 0) {
			return main_redirect_handler(req);
		}

		ESP_LOGE(TAG, "Failed to stat file : %s", filepath);
		return http_404_error_handler(req, HTTPD_404_NOT_FOUND);
	}

	fd = fopen(filepath, "r");
	if (!fd) {
		ESP_LOGE(TAG, "Failed to read file: %s", filepath);
		return http_404_error_handler(req, HTTPD_500_INTERNAL_SERVER_ERROR);
	}

	memset(resp, 0, sizeof(resp));
	size = fread(resp, 1, sizeof(resp), fd);
	fclose(fd);

	if (strcmp(req->uri, "/shot.jpeg") == 0) {
		httpd_resp_set_type(req, "image/jpeg");
	} else {
		httpd_resp_set_type(req, "text/html");
	}

	httpd_resp_send(req, resp, size);

	return ESP_OK;
}

static const httpd_uri_t main = {
	.uri       = "/*",
	.method    = HTTP_GET,
	.handler   = main_get_handler,
};

static const httpd_uri_t shot = {
	.uri       = "/shot",
	.method    = HTTP_POST,
	.handler   = shot_post_handler,
};

static httpd_handle_t start_http_server(void)
{
	httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
	httpd_handle_t srv = NULL;

	cfg.uri_match_fn = httpd_uri_match_wildcard;
	cfg.lru_purge_enable = true;

	ESP_LOGI(TAG, "%s: starting http server on port: '%d'", __func__, cfg.server_port);

	if (httpd_start(&srv, &cfg) == ESP_OK) {
		httpd_register_uri_handler(srv, &main);
		httpd_register_uri_handler(srv, &shot);
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
		ESP_LOGE(TAG, "Error starting server!");
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
		if (server) {
			if (stop_http_server(server) == ESP_OK) {
				server = NULL;
			} else {
				ESP_LOGE(TAG, "Failed to stop http server");
			}
		}
		break;
	default:
		ESP_LOGW(TAG, "Unhandled event: %s:%ld\n", event_base, event_id);
		break;
	}
}

esp_err_t mount_spiffs_storage(const char* base_path)
{
	size_t total = 0;
	size_t used = 0;
	esp_err_t ret;

	ESP_LOGI(TAG, "Initializing SPIFFS");

	esp_vfs_spiffs_conf_t conf = {
		.base_path = base_path,
		.partition_label = NULL,
		.max_files = 5,
		.format_if_mount_failed = false,
	};

	ret = esp_vfs_spiffs_register(&conf);
	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
		return ret;
	}

	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
		return ret;
	}

	ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	return ESP_OK;
}

void http_task(void *args)
{

	ESP_ERROR_CHECK(mount_spiffs_storage(base_path));

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

	while (1) {
		vTaskDelay(1000);
	}
}
