#include "esp_camera.h"
#include "esp_log.h"

#include "driver/gpio.h"

#include "common.h"

#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK    0 
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27

#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

static const char *TAG = "mod:cam";

static camera_config_t camera_config = {
	.pin_pwdn  = CAM_PIN_PWDN,
	.pin_reset = CAM_PIN_RESET,
	.pin_xclk = CAM_PIN_XCLK,
	.pin_sccb_sda = CAM_PIN_SIOD,
	.pin_sccb_scl = CAM_PIN_SIOC,

	.pin_d7 = CAM_PIN_D7,
	.pin_d6 = CAM_PIN_D6,
	.pin_d5 = CAM_PIN_D5,
	.pin_d4 = CAM_PIN_D4,
	.pin_d3 = CAM_PIN_D3,
	.pin_d2 = CAM_PIN_D2,
	.pin_d1 = CAM_PIN_D1,
	.pin_d0 = CAM_PIN_D0,
	.pin_vsync = CAM_PIN_VSYNC,
	.pin_href = CAM_PIN_HREF,
	.pin_pclk = CAM_PIN_PCLK,

	/* EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode */
	.xclk_freq_hz = 20000000,
	.ledc_timer = LEDC_TIMER_0,
	.ledc_channel = LEDC_CHANNEL_0,

	/* YUV422 / GRAYSCALE / RGB565 / JPEG */
	.pixel_format = PIXFORMAT_JPEG,

	/* QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG.
	 * The performance of the ESP32-S series has improved a lot,
	 * but JPEG mode always gives better frame rates.
	 */
	.frame_size = FRAMESIZE_VGA,

	/* 0-63, for OV series camera sensors, lower number means higher quality */
	.jpeg_quality = 10,

	/* When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode. */
	.fb_count = 1,

	/* CAMERA_GRAB_LATEST. Sets when buffers should be filled */
	.grab_mode = CAMERA_GRAB_WHEN_EMPTY
};

esp_err_t camera_init(void)
{
	if (CAM_PIN_PWDN != -1) {
		gpio_reset_pin(CAM_PIN_PWDN);
		gpio_set_direction(CAM_PIN_PWDN, GPIO_MODE_OUTPUT);
		gpio_set_level(CAM_PIN_PWDN, 0);
	}

	esp_err_t err = esp_camera_init(&camera_config);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Camera Init Failed");
		return err;
	}

	return ESP_OK;
}

esp_err_t camera_capture(void)
{
	camera_fb_t *fb = esp_camera_fb_get();
	if (!fb) {
		ESP_LOGE(TAG, "Camera Capture Failed");
		return ESP_FAIL;
	}

	/* custom image processing
	 * process_image(fb->width, fb->height, fb->format, fb->buf, fb->len);
	 */

	esp_camera_fb_return(fb);
	return ESP_OK;
}
