#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_event.h"

#include "driver/gpio.h"

#include "common.h"

#define GPIO_INPUT_IO       CONFIG_EXAMPLE_GPIO_INPUT
#define GPIO_INPUT_PIN_SEL  (1ULL << GPIO_INPUT_IO)

static const char *TAG = "wifi-button";
static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
	uint32_t gpio_num = (uint32_t)arg;
	uint32_t level = gpio_get_level(gpio_num);

	xQueueSendFromISR(gpio_evt_queue, &level, NULL);
}

void button_task(void *args)
{
	gpio_config_t io_conf = {};
	uint32_t value;

	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

	io_conf.intr_type = GPIO_INTR_ANYEDGE;
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	io_conf.mode = GPIO_MODE_INPUT;

	gpio_config(&io_conf);
	gpio_install_isr_service(0);
	gpio_isr_handler_add(GPIO_INPUT_IO, gpio_isr_handler, (void *) GPIO_INPUT_IO);

	while (1) {
		if(xQueueReceive(gpio_evt_queue, &value, portMAX_DELAY)) {
			ESP_LOGI(TAG, "Button: value %lu\n", value);
			ESP_ERROR_CHECK(esp_event_post(SYSTEM_EVENTS, SYSTEM_BUTTON_EVENT,
						       &value, sizeof(value), 0));
		}
	}
}
