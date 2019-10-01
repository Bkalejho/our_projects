#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_system.h"

#define Skip_ROM        0xCC
#define Convert_T       0x44
#define Read_scratchpad 0xBE

#define Tx_18B20    gpio_set_direction(Port_18B20, GPIO_MODE_OUTPUT)
#define Rx_18B20    gpio_set_direction(Port_18B20, GPIO_MODE_INPUT)

#define Port_18B20	GPIO_NUM_2 
#define Alive		GPIO_NUM_5

#define __delay_1us  for(uint32_t __count_for_us=0; __count_for_us<5; __count_for_us++)
#define __delay_10us  for(uint32_t __count_for_us=0; __count_for_us<147; __count_for_us++)
#define __delay_60us  for(uint32_t __count_for_us=0; __count_for_us<900; __count_for_us++)
#define __delay_500us  for(uint32_t __count_for_us=0; __count_for_us<3325; __count_for_us++)

uint8_t reset();
void write(uint8_t WRT);
uint8_t read();

void app_main(void)
{
	gpio_set_direction(Alive, GPIO_MODE_OUTPUT);
	//gpio_set_pull_mode(SRC_TEST, GPIO_PULLUP_ONLY);
 	
 	bool state=0;
    uint8_t tempL=0, tempH=0;
    int16_t temp_int=0;
    float temp=0;

    while (1) {
        
        state=!state;

        if(reset()==0){
            write(Skip_ROM);        
            write(Convert_T);       
            vTaskDelay(750/portTICK_RATE_MS);

            reset();
            write(Skip_ROM);        
            write(Read_scratchpad);

            tempL=read();
            tempH=read();
            temp_int=  (tempH << 8) + tempL;
            temp=temp_int;
            temp*=0.0625;

            printf("La temperatura es %.2f \r\n", temp); 
        }else{
            printf("El sensor no está conectado \r\n");
        }

        gpio_set_level(Alive, state);
                
        vTaskDelay(1000/ portTICK_RATE_MS);
    }
       
}

uint8_t reset()
{
    Tx_18B20;                               // set pin to output
    gpio_set_level(Port_18B20, 0);          // set pin# to low (0)
    __delay_500us;                          // 1 wire require time delay
    Rx_18B20;                               //set pin to input
    __delay_60us;                           // 1 wire require time delay
    
    if (gpio_get_level(Port_18B20) == 0)    // if there is a presence pluse
    { 
        __delay_500us;
        return 0;                           // return 0 ( 1-wire is presence)
    } 
    else 
    {
        __delay_500us;
        return 1;                           // return 1 ( 1-wire is NOT presence)
    }
}

void write(uint8_t WRT)
{
    uint8_t i;
    Rx_18B20;                               // set pin to input 
    
    for(i = 0; i < 8; i++)
    {
        if((WRT & (1<<i))!= 0) 
        {
            // write 1
            Tx_18B20;                           // set pin to output 
            gpio_set_level(Port_18B20,0);       // set pin to low
            __delay_1us;                        // 1 wire require time delay
            Rx_18B20;                           // set pin to input
            __delay_60us;                       // 1 wire require time delay
        } 
        else 
        {
            //write 0
            Tx_18B20;                           // set pin to output
            gpio_set_level(Port_18B20,0);       // set pin to low
            __delay_60us;                       // 1 wire require time delay
            Rx_18B20;                           // set pin# to input (release the bus)
        }
    }
}

uint8_t read()
{
    uint8_t i,result = 0;
    Rx_18B20; // TRIS is input(1)
    
    for(i = 0; i < 8; i++)
    {
        Tx_18B20;                           // set pin to 
        gpio_set_level(Port_18B20,0);       // set pin to low
        __delay_1us;
        __delay_1us;
        Rx_18B20;                           // set pin to input
        if(gpio_get_level(Port_18B20) != 0){
            result |= 1<<i;    
        }
        
        __delay_60us;                       // wait for recovery time
    }
    
    return result;
}
