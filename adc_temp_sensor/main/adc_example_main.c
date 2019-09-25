/* adc example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_log.h"

static const char *TAG = "LM35 temperature";

static void adc_task()
{
    float temp=0;
    uint16_t adc_data[2];

    while (1) {
        if (ESP_OK == adc_read(&adc_data[0])) {
        	temp=adc_data[0];
        	temp*=0.002846;		//This has the ADC value ADC/(1/1023) multiply for the Voltage divisor factor (320K/100K)
        	temp/=0.01;				//This line compute the temperature value for lm35
            ESP_LOGI(TAG, "the temperature is: %.1f c \r\n", temp);
        }

        vTaskDelay(250 / portTICK_RATE_MS);
    }
}

void app_main()
{
    // 1. init adc
    adc_config_t adc_config;

    // Depend on menuconfig->Component config->PHY->vdd33_const value
    // When measuring system voltage(ADC_READ_VDD_MODE), vdd33_const must be set to 255.
    adc_config.mode = ADC_READ_TOUT_MODE;
    adc_config.clk_div = 8; // ADC sample collection clock = 80MHz/clk_div = 10MHz
    ESP_ERROR_CHECK(adc_init(&adc_config));

    // 2. Create a adc task to read adc value
    xTaskCreate(adc_task, "adc_task", 1024, NULL, 5, NULL);
}
