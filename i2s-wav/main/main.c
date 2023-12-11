/* I2S Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_flash.h"
#include "esp_log.h"

#include "driver/i2s_std.h"

#include "sdkconfig.h"

#define STACK_SIZE 3584

static const char *TAG = "sound";

static i2s_chan_handle_t tx_handle = NULL;

extern const uint8_t test_wav_start[] asm("_binary_test_wav_start");
extern const uint8_t test_wav_end[]   asm("_binary_test_wav_end");

static esp_err_t i2s_driver_init(void)
{
	i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
	chan_cfg.auto_clear = true;

	ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

	i2s_std_config_t std_cfg = {
		.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
		.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
		.gpio_cfg = {
			.mclk	= I2S_GPIO_UNUSED,
			.bclk	= GPIO_NUM_4,
			.ws	= GPIO_NUM_5,
			.dout	= GPIO_NUM_18,
			.din	= I2S_GPIO_UNUSED,
			.invert_flags = {
				.mclk_inv = false,
				.bclk_inv = false,
				.ws_inv = false,
			},
		},
	};

	ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
	ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));

	return ESP_OK;
}

static void i2s_test1(void *args)
{
	esp_err_t ret = ESP_OK;
	size_t bytes_write = 0;

	while (1) {
		ESP_LOGI(TAG, "%s: play audio", __func__);

		ret = i2s_channel_write(tx_handle, (uint8_t *)test_wav_start, test_wav_end - test_wav_start, &bytes_write, portMAX_DELAY);
		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "%s: i2s write failed: reason %d", __func__, ret);
			abort();
		}

		if (bytes_write > 0) {
			ESP_LOGI(TAG, "%s: i2s sound played, %d bytes are written", __func__, bytes_write);
		} else {
			ESP_LOGE(TAG, "%s: i2s sound play failed", __func__);
			abort();
		}

		ESP_LOGI(TAG, "%s: delay...", __func__);
		vTaskDelay(1000 /* ms */ / portTICK_PERIOD_MS);
	}
}

void app_main(void)
{
	TaskHandle_t xTest1Handle = NULL;

	if (i2s_driver_init() != ESP_OK) {
		ESP_LOGE(TAG, "i2s driver init failed");
		abort();
	} else {
		ESP_LOGI(TAG, "i2s driver init success");
	}

	xTaskCreate(i2s_test1, "i2s_test1", STACK_SIZE, NULL, tskIDLE_PRIORITY, &xTest1Handle);
	if (!xTest1Handle) {
		ESP_LOGE(TAG, "Failed to create task i2s_test1");
	}

	while (1) {
		ESP_LOGI(TAG, "idle...");
		vTaskDelay(1000 /* ms */ / portTICK_PERIOD_MS);
	}
}
