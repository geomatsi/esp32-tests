/* FSM Traffic Light Example */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_timer.h"

#include "common.h"

enum {
	RED = 0,
	BLUE,
	GREEN,
} rgb_led_id_t;

struct gpio_led rgb_led[] = {
	[RED] = {
		.pin = CONFIG_GPIO_LED_RED,
		.name = "RED",
		.state = 0,
	},
	[BLUE] = {
		.pin = CONFIG_GPIO_LED_BLUE,
		.name = "BLUE",
		.state = 0,
	},
	[GREEN] = {
		.pin = CONFIG_GPIO_LED_GREEN,
		.name = "GREEN",
		.state = 0,
	},
};

static tl_state_t tl_state = INIT_STATE;
static esp_timer_handle_t tl_timer;
static const char *TAG = "rgb";

static void rgb_set_state(color_t color)
{
	for (int i = 0; i < ARRAY_SIZE(rgb_led); i++) {
		rgb_led[i].state = 0;
		gpio_set_level(rgb_led[i].pin, rgb_led[i].state);
	}

	switch (color) {
		case OFF:
			ESP_LOGI(TAG, "LED: turn off");
			break;
		case RED_COLOR:
			rgb_led[RED].state = 1;
			gpio_set_level(rgb_led[RED].pin, rgb_led[RED].state);
			ESP_LOGI(TAG, "LED: turn on RED");
			break;
		case BLUE_COLOR:
			rgb_led[BLUE].state = 1;
			gpio_set_level(rgb_led[BLUE].pin, rgb_led[BLUE].state);
			ESP_LOGI(TAG, "LED: turn on BLUE");
			break;
		case GREEN_COLOR:
			rgb_led[GREEN].state = 1;
			gpio_set_level(rgb_led[GREEN].pin, rgb_led[GREEN].state);
			ESP_LOGI(TAG, "LED: turn on GREEN");
			break;
		case YELLOW_COLOR:
			rgb_led[GREEN].state = 1;
			rgb_led[RED].state = 1;
			gpio_set_level(rgb_led[GREEN].pin, rgb_led[GREEN].state);
			gpio_set_level(rgb_led[RED].pin, rgb_led[RED].state);
			ESP_LOGI(TAG, "LED: turn on YELLOW");
			break;
		default:
			ESP_LOGE(TAG, "unexpected color %d", color);
			break;
	}
}

static void tl_callback(void* arg)
{
	static int flash_count = 0;

	ESP_LOGI(TAG, "TL timer: state %d", tl_state);

	switch (tl_state) {
		case INIT_STATE:
			tl_state = RED_STATE;
			rgb_set_state(RED_COLOR);
			ESP_ERROR_CHECK(esp_timer_restart(tl_timer, CONFIG_RED_TIME_MS * 1000));
			break;
		case RED_STATE:
			tl_state = GREEN_STATE;
			rgb_set_state(GREEN_COLOR);
			ESP_ERROR_CHECK(esp_timer_restart(tl_timer, CONFIG_GREEN_TIME_MS * 1000));
			break;
		case GREEN_STATE:
			tl_state = GREEN_FLASH_STATE;
			rgb_set_state(OFF);
			ESP_ERROR_CHECK(esp_timer_restart(tl_timer, CONFIG_GREEN_FLASH_TIME_MS * 1000));
			break;
		case GREEN_FLASH_STATE:
			if (++flash_count >= CONFIG_GREEN_FLASH_COUNT) {
				flash_count = 0;
				tl_state = YELLOW_STATE;
				rgb_set_state(YELLOW_COLOR);
				ESP_ERROR_CHECK(esp_timer_restart(tl_timer, CONFIG_YELLOW_TIME_MS * 1000));
				break;
			}

			rgb_set_state(flash_count % 2 ? GREEN_COLOR : OFF);
			ESP_ERROR_CHECK(esp_timer_restart(tl_timer, CONFIG_GREEN_FLASH_TIME_MS * 1000));
			break;
		case YELLOW_STATE:
			tl_state = RED_STATE;
			rgb_set_state(RED_COLOR);
			ESP_ERROR_CHECK(esp_timer_restart(tl_timer, CONFIG_RED_TIME_MS * 1000));
			break;
		default:
			ESP_LOGE(TAG, "TL timer: unexpected state %d", tl_state);
			tl_state = INIT_STATE;
			ESP_ERROR_CHECK(esp_timer_restart(tl_timer, 100));
			break;
	}
}

void app_main(void)
{
	esp_timer_create_args_t timer_args = {
		.callback = &tl_callback,
		.name = "tl_timer"
	};

	for (int i = 0; i < ARRAY_SIZE(rgb_led); i++) {
		gpio_reset_pin(rgb_led[i].pin);
		gpio_set_direction(rgb_led[i].pin, GPIO_MODE_OUTPUT);
		gpio_set_level(rgb_led[i].pin, 0);
	}

	ESP_ERROR_CHECK(esp_timer_create(&timer_args, &tl_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(tl_timer, 100));

	/* main idle cycle */

	while (1) {
		vTaskDelay(1000);
	}
}
