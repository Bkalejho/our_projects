#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_system.h"

#define LINK_LED	GPIO_NUM_16
#define BAT_LED		GPIO_NUM_5
#define	SRC_TEST	GPIO_NUM_4

void app_main(void)
{
	gpio_set_direction(LINK_LED, GPIO_MODE_OUTPUT);
	gpio_set_direction(BAT_LED, GPIO_MODE_OUTPUT);
	gpio_set_direction(SRC_TEST, GPIO_MODE_INPUT);
	gpio_set_pull_mode(SRC_TEST, GPIO_PULLUP_ONLY);
 	bool state=0;
 	int level = 0;

    while (1) {
        state=!state;

        level = gpio_get_level(SRC_TEST);

        printf("El nivel de entrada es: %i \r\n", level);

        gpio_set_level(BAT_LED, level);
        gpio_set_level(LINK_LED, gpio_get_level(SRC_TEST));
        
        vTaskDelay(10/ portTICK_RATE_MS);
    }
       
}


