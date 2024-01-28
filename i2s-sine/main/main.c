/* I2S Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_flash.h"
#include "esp_log.h"

#include "driver/i2s_std.h"

#include "sdkconfig.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define STACK_SIZE 3584
#define SAMPLING_FREQ 16000

static const char *TAG = "sine";

int freq[] = {261, 293, 329, 349, 392, 440, 493 };

static i2s_chan_handle_t tx_handle = NULL;

int16_t sine[SAMPLING_FREQ]; /* duration 1 sec */

size_t sine_float(int freq, unsigned int samples_per_sec, int amp, int16_t *sine, unsigned int samples);
size_t sine_cordic(int freq, unsigned int samples_per_sec, int amp, int16_t *sine, unsigned int samples);
size_t sine_cordic_v2(int freq, unsigned int samples_per_sec, int amp, int16_t *sine, unsigned int samples);

static esp_err_t i2s_driver_init(void)
{
	i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
	chan_cfg.auto_clear = true;

	ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

	i2s_std_config_t std_cfg = {
		.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLING_FREQ),
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

static void i2s_test(void *args)
{
	esp_err_t ret = ESP_OK;
	size_t bytes_write = 0;
	uint16_t fq;
	size_t sz;

	for (int n = 0;; n++) {
		fq = freq[n % ARRAY_SIZE(freq)];

#if 1
		sz = sine_float(fq, SAMPLING_FREQ, 2000, &sine[0], ARRAY_SIZE(sine));
		ESP_LOGI(TAG, "%s: play float sine: freq %u bytes %u", __func__, fq, sz);

		ret = i2s_channel_write(tx_handle, sine, sz, &bytes_write, portMAX_DELAY);
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
#endif

#if 1
		sz = sine_cordic_v2(fq, SAMPLING_FREQ, 2000, &sine[0], ARRAY_SIZE(sine));
		ESP_LOGI(TAG, "%s: play cordic sine: freq %u bytes %u", __func__, fq, sz);

		ret = i2s_channel_write(tx_handle, sine, sz, &bytes_write, portMAX_DELAY);
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
#endif
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

	xTaskCreate(i2s_test, "i2s_test", STACK_SIZE, NULL, tskIDLE_PRIORITY, &xTest1Handle);
	if (!xTest1Handle) {
		ESP_LOGE(TAG, "Failed to create task i2s_test");
	}

	while (1) {
		ESP_LOGI(TAG, "idle...");
		vTaskDelay(1000 /* ms */ / portTICK_PERIOD_MS);
	}
}
