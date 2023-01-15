#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "esp_adc/adc_oneshot.h"
#else
#include "driver/adc.h"
#endif

#include "ssd1306.h"

/*
 You have to set this config value with menuconfig
 CONFIG_INTERFACE

 for i2c
 CONFIG_MODEL
 CONFIG_SDA_GPIO
 CONFIG_SCL_GPIO
 CONFIG_RESET_GPIO

 for SPI
 CONFIG_CS_GPIO
 CONFIG_DC_GPIO
 CONFIG_RESET_GPIO
*/

#define TAG "SSD1306"

// https://forum.arduino.cc/t/analog-vu-meter-i2c-oled-sh1106-oledmeter-animation/388374
// VU meter background mask image:
static uint8_t VUMeter1[1024] = { 
	// 128x84
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x03, 0x00, 0x60, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x09, 0x04, 0x80, 0x21, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x98, 0x08, 0x06, 0x03, 0x80, 0x21, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xA4, 0x10, 0x09, 0x00, 0x80, 0x21, 0x20, 0x07, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xA4, 0x10, 0x06, 0x03, 0x00, 0x20, 0xC0, 0x00, 0x80, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x71, 0x80, 0xA4, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x0A, 0x40, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3C, 0x00, 0x00,
	0x00, 0x00, 0x3A, 0x40, 0x00, 0x00, 0x02, 0x01, 0x00, 0x40, 0x80, 0x07, 0x00, 0x20, 0x00, 0x00,
	0x00, 0x00, 0x42, 0x40, 0x00, 0x08, 0x02, 0x01, 0x08, 0x40, 0x80, 0x00, 0x00, 0x38, 0x00, 0x00,
	0x00, 0x00, 0x79, 0x80, 0x04, 0x08, 0x02, 0x01, 0x08, 0x81, 0x10, 0x00, 0x00, 0x04, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x04, 0x08, 0x02, 0x01, 0x08, 0x81, 0x11, 0x04, 0x00, 0x38, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x02, 0x04, 0x02, 0x01, 0x08, 0x81, 0x21, 0x04, 0x00, 0x00, 0x08, 0x00,
	0x00, 0x00, 0x00, 0x84, 0x02, 0x04, 0x0F, 0xFF, 0xFF, 0xC3, 0xE2, 0x04, 0x00, 0x00, 0x08, 0x00,
	0x00, 0x00, 0x00, 0xC2, 0x01, 0x07, 0xF0, 0x00, 0x00, 0x3B, 0xFE, 0x08, 0x40, 0x40, 0x08, 0x00,
	0x00, 0xFE, 0x00, 0x62, 0x01, 0xF8, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xE8, 0x40, 0x80, 0x7F, 0x00,
	0x00, 0x00, 0x00, 0x21, 0x1E, 0x00, 0x04, 0x00, 0x80, 0x00, 0x7F, 0xFE, 0x80, 0x80, 0x08, 0x00,
	0x00, 0x00, 0x03, 0x31, 0xE0, 0x00, 0x04, 0x00, 0x80, 0x04, 0x01, 0xFF, 0xC1, 0x00, 0x08, 0x00,
	0x00, 0x00, 0x07, 0x1E, 0x00, 0x40, 0x00, 0x00, 0x00, 0x04, 0x00, 0x1F, 0xFA, 0x00, 0x08, 0x00,
	0x00, 0x00, 0x07, 0xF0, 0x00, 0x40, 0x3B, 0x07, 0x60, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x03, 0x80, 0x00, 0x00, 0x34, 0x81, 0x90, 0xCC, 0xC0, 0x00, 0x3F, 0xC0, 0x00, 0x00,
	0x00, 0x00, 0x0C, 0x00, 0x03, 0x30, 0x0C, 0x82, 0x90, 0x53, 0x20, 0x00, 0x07, 0xF8, 0x00, 0x00,
	0x00, 0x00, 0x70, 0x40, 0x00, 0xC8, 0x3B, 0x02, 0x60, 0x53, 0x20, 0x00, 0x00, 0xFE, 0x00, 0x00,
	0x00, 0x01, 0x80, 0x20, 0x01, 0xC8, 0x00, 0x00, 0x00, 0x4C, 0xC0, 0x00, 0x00, 0x3F, 0x80, 0x00,
	0x00, 0x06, 0x00, 0x00, 0x03, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xE0, 0x00,
	0x00, 0x08, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFC, 0x00,
	0x00, 0x30, 0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00,
	0x00, 0x00, 0x40, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
	0x00, 0x00, 0xA0, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x02, 0x02, 0x30, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x03, 0x06, 0x30, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x8C, 0x30, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0xD8, 0x30, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x70, 0x19, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void app_main(void)
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
	// ADC1 Init
	adc_oneshot_unit_handle_t adc1_handle;
	adc_oneshot_unit_init_cfg_t init_config1 = {
		.unit_id = ADC_UNIT_1,
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

	// ADC1 config
	adc_oneshot_chan_cfg_t config = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		.atten = ADC_ATTEN_DB_11,
	};
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config));
#else
	// ADC1 config
	ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
	ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11));
#endif

	SSD1306_t dev;

#if CONFIG_I2C_INTERFACE
	ESP_LOGI(TAG, "INTERFACE is i2c");
	ESP_LOGI(TAG, "CONFIG_SDA_GPIO=%d",CONFIG_SDA_GPIO);
	ESP_LOGI(TAG, "CONFIG_SCL_GPIO=%d",CONFIG_SCL_GPIO);
	ESP_LOGI(TAG, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
	i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
#endif // CONFIG_I2C_INTERFACE

#if CONFIG_SPI_INTERFACE
	ESP_LOGI(TAG, "INTERFACE is SPI");
	ESP_LOGI(TAG, "CONFIG_MOSI_GPIO=%d",CONFIG_MOSI_GPIO);
	ESP_LOGI(TAG, "CONFIG_SCLK_GPIO=%d",CONFIG_SCLK_GPIO);
	ESP_LOGI(TAG, "CONFIG_CS_GPIO=%d",CONFIG_CS_GPIO);
	ESP_LOGI(TAG, "CONFIG_DC_GPIO=%d",CONFIG_DC_GPIO);
	ESP_LOGI(TAG, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO);
#endif // CONFIG_SPI_INTERFACE

#if CONFIG_FLIP
	dev._flip = true;
	ESP_LOGW(TAG, "Flip upside down");
#endif

#if CONFIG_SSD1306_128x64
	ESP_LOGI(TAG, "Panel is 128x64");
	ssd1306_init(&dev, 128, 64);
#endif // CONFIG_SSD1306_128x64
#if CONFIG_SSD1306_128x32
	ESP_LOGE(TAG, "Panel is 128x32. This demo cannot be run.");
	while(1) { vTaskDelay(1); }
#endif // CONFIG_SSD1306_128x32

	ssd1306_contrast(&dev, 0xff);
	ssd1306_clear_screen(&dev, false);

	// This function internally converts vertical and horizontal.
	// 128x64 --> 64x128
	// And save it to internal buffer.
	// The internal buffer is upside down vertically and horizontally.
	// I use this function as converter.
	ssd1306_bitmaps(&dev, 0, 0, VUMeter1, 128, 64, false);

	// Allocate memory
	uint8_t *buffer = (uint8_t *)malloc(1024);
	if (buffer == NULL) {
		ESP_LOGE(TAG, "malloc failed");
		while(1) { vTaskDelay(1); }
	}

	// Get from internal buffer to local buffer
	ssd1306_get_buffer(&dev, buffer);

	int hMeter = 65; // horizontal center for needle animation
	//int vMeter = 85; // vertical center for needle animation (outside of dislay limits)
	int vMeter = 63; // vertical center for needle animation (outside of dislay limits)
	int rMeter = 50; // length of needle animation or arch of needle travel

	while(1) {
		//ssd1306_clear_screen(&dev, false);
		int adc_raw;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
		ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw));
#else
		adc_raw = adc1_get_raw(ADC1_CHANNEL_0);
#endif

		// Convert to 10 bits(from 0 to 1023). Because the original code is for ATMEGA328.
#ifndef CONFIG_IDF_TARGET_ESP32S2
		adc_raw = adc_raw / 4; // 12 bits -> 10 bits.
#else
		// ESP32S2 only support 13 bit width
		adc_raw = adc_raw / 8; // 13 bits -> 10 bits.
#endif
		ESP_LOGD(TAG, "adc_raw=%d", adc_raw);
		float MeterValue = adc_raw * 120.079 / 1023;
		MeterValue = MeterValue - 60.039;
		int a1 = (hMeter + (sin(MeterValue / 502.64 * 6.283) * rMeter)); 
		int a2 = (vMeter - (cos(MeterValue / 502.64 * 6.283) * rMeter)); 

		// Set background image. There is no needle.
		ssd1306_set_buffer(&dev, buffer);

		// Set needle
		_ssd1306_line(&dev, a1, a2, hMeter, vMeter, false);

		// Display the entire image
		ssd1306_show_buffer(&dev);
		vTaskDelay(1);

		// Erase needle
		//_ssd1306_line(&dev, a1, a2, hMeter, vMeter, true);
	}
}
