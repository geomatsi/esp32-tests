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

#include "format_wav.h"
#include "sdkconfig.h"

#define STACK_SIZE 3584

static const char *TAG = "sound";

static i2s_chan_handle_t tx_handle = NULL;

extern const uint8_t test_wav_start[] asm("_binary_test_wav_start");
extern const uint8_t test_wav_end[]   asm("_binary_test_wav_end");

static fmt_chunk_t *get_wav_fmt(const uint8_t *start, const uint8_t *end)
{
	dsc_chunk_t *dsc = (dsc_chunk_t *)start;
	fmt_chunk_t *fmt;

	if (strncmp(dsc->chunk_id, "RIFF", 4)) {
		return NULL;
	}

	if (strncmp(dsc->chunk_format, "WAVE", 4)) {
		return NULL;
	}

	/* assume that fmt is always after descriptor */
	fmt = (fmt_chunk_t *)(start + sizeof(dsc_chunk_t));

	if (strncmp(fmt->subchunk_id, "fmt", 3)) {
		return NULL;
	}

	return fmt;
}

static data_chunk_t *get_wav_audio(const uint8_t *start, const uint8_t *end)
{
	dsc_chunk_t *dsc = (dsc_chunk_t *)start;
	uint8_t *ptr = (uint8_t *)start;
	data_chunk_t *sch;

	if (strncmp(dsc->chunk_id, "RIFF", 4)) {
		return NULL;
	}

	if (strncmp(dsc->chunk_format, "WAVE", 4)) {
		return NULL;
	}

	ptr = ptr + sizeof(dsc_chunk_t);
	sch = (data_chunk_t *)ptr;

	while(ptr < end) {
		if (!strncmp(sch->subchunk_id, "data", 4)) {
			return sch;
		}

		ptr = ptr + sch->subchunk_size + sizeof(data_chunk_t);
		sch = (data_chunk_t *)ptr;
	}

	return NULL;
}

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

	fmt_chunk_t *fmt = get_wav_fmt(test_wav_start, test_wav_end);
	if (!fmt) {
		ESP_LOGE(TAG, "%s: failed to find wav file format", __func__);
		abort();
	}

	ESP_LOGI(TAG, "%s: format: %u", __func__, fmt->audio_format);
	ESP_LOGI(TAG, "%s: sample_rate: %lu", __func__, fmt->sample_rate);
	ESP_LOGI(TAG, "%s: bits_per_sample: %u", __func__, fmt->bits_per_sample);
	ESP_LOGI(TAG, "%s: type: %s", __func__, (fmt->num_of_channels == I2S_SLOT_MODE_MONO) ? "mono" : "stereo");

	if (fmt->audio_format != 1) {
		ESP_LOGW(TAG, "%s: unexpected audio format: %u != 1 (PCM)", __func__, fmt->audio_format);
	}

	std_cfg.clk_cfg.sample_rate_hz = fmt->sample_rate;

	std_cfg.slot_cfg.slot_mask = (fmt->num_of_channels == I2S_SLOT_MODE_MONO) ? I2S_STD_SLOT_LEFT : I2S_STD_SLOT_BOTH;
	std_cfg.slot_cfg.slot_mode = fmt->num_of_channels;

	std_cfg.slot_cfg.msb_right = (fmt->bits_per_sample <= I2S_DATA_BIT_WIDTH_16BIT) ? true : false;
	std_cfg.slot_cfg.data_bit_width = fmt->bits_per_sample;
	std_cfg.slot_cfg.ws_width = fmt->bits_per_sample;

	ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
	ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));

	return ESP_OK;
}

static void i2s_test(void *args)
{
	esp_err_t ret = ESP_OK;
	size_t bytes_write = 0;
	data_chunk_t *data;

	data = get_wav_audio(test_wav_start, test_wav_end);
	if (!data) {
		ESP_LOGE(TAG, "%s: failed to find wav file audio data", __func__);
		abort();
	}

	while (1) {
		ESP_LOGI(TAG, "%s: play audio", __func__);

		ret = i2s_channel_write(tx_handle, data->data, data->subchunk_size, &bytes_write, portMAX_DELAY);
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

	xTaskCreate(i2s_test, "i2s_test", STACK_SIZE, NULL, tskIDLE_PRIORITY, &xTest1Handle);
	if (!xTest1Handle) {
		ESP_LOGE(TAG, "Failed to create task i2s_test");
	}

	while (1) {
		ESP_LOGI(TAG, "idle...");
		vTaskDelay(1000 /* ms */ / portTICK_PERIOD_MS);
	}
}
