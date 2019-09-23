//23092019

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "driver/adc.h"

#include "lwip/apps/sntp.h"
#include <time.h>

#include "driver/gpio.h"
#include "esp_system.h"




typedef char sample_mssg[100];          //Creates a variable wich is the least string size of each sample 

static const char *TAG = "JAL IoT Ubidots and MQTT";

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const static int CONNECTED_BIT = BIT0;

esp_mqtt_client_handle_t client_h;
bool mqtt_con = 0;
uint8_t con_flag = 0;

#define BROKER_PORT 1883
#define BROKER_URL  "mqtt://industrial.api.ubidots.com"
#define TOKEN       "BBFF-6QgNNxNG9tBB0BtpKGagXOH6usY0DK"
#define UBI_TOPIC   "/v1.6/devices/esp_8266/temperature"
#define SSID_WIFI   "jony"
#define PASS_WIFI   "jfcr920210"

#define LINK_LED    GPIO_NUM_16
#define BAT_LED     GPIO_NUM_5
#define SRC_TEST    GPIO_NUM_4



static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            
            mqtt_con = 1;       //bandera de conexión 
            gpio_set_level(LINK_LED, mqtt_con);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            mqtt_con = 0;       //bandera de conexión             
            /*con_flag++;
            if(con_flag>=3){
                con_flag=0;
                vTaskDelay(2000 / portTICK_RATE_MS);
                fflush(stdout);
                esp_restart();
            }*/

            gpio_set_level(LINK_LED,mqtt_con);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;
    
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
            if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
                /*Switch to 802.11 bgn mode */
                esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
            }
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void wifi_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = SSID_WIFI,//CONFIG_WIFI_SSID,
            .password = PASS_WIFI,//CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(TAG, "start the WIFI SSID:[%s]", CONFIG_WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Waiting for wifi");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = BROKER_URL,
        .event_handle = mqtt_event_handler,
        .port = BROKER_PORT,
        .username = TOKEN,
        .password = "",
        // .user_context = (void *)your_context
    };

    client_h = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client_h);
}

//-----------------------battery task----------------------------
static void bat_task()
{
    for(;;)
    {
       gpio_set_level(BAT_LED, gpio_get_level(SRC_TEST)); 
       vTaskDelay(1000 / portTICK_RATE_MS);
    }

}
//-----------------------adc task----------------------------
static void adc_task()
{
    uint16_t adc_data[2];
    bool context = 0;
    char mensaje[100];
    char context_mensaje[50];
    sample_mssg memory[10];

    uint8_t mem_pos=0;
    char fecha[15];
    uint32_t now;
    struct tm timeinfo;
    char strftime_buf[64];

    vTaskDelay(2000 / portTICK_RATE_MS);

    while (1) {

        //gpio_set_level(BAT_LED, gpio_get_level(SRC_TEST));

        time(&now);//ubidots requiere recibir el timestamp en milisegundos aqui esta en segundos
        localtime_r(&now, &timeinfo);
        
        sprintf(fecha,"%u000",now);//se agregan 3 ceros para enviar la trama a ubidots de forma correcta
        printf("\r\n------------------------------------- New Sample --------------------------------------\r\n");
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGI(TAG,"The current date/time in Bogotá is: %s \r\n", strftime_buf);
   
        if (ESP_OK == adc_read(&adc_data[0])) {
            ESP_LOGI(TAG, "adc read: %d\r\n", adc_data[0]);
        }

        if(context){
            sprintf(context_mensaje,"\"context\": {\"name\": \"Alta Temperatura\"}");
        }else{
            sprintf(context_mensaje,"\"context\": {\"name\": \"Baja Temperatura\"}");
        }

        sprintf(mensaje,"{\"value\": %d, %s,\"timestamp\": %s}", adc_data[0], context_mensaje, fecha);

        if(mqtt_con==1){
            if(mem_pos!=0){
                for(uint8_t i=0; i<mem_pos; i++){
                    esp_mqtt_client_publish(client_h, UBI_TOPIC, memory[i], 0, 1, 0);
                    vTaskDelay(10000 /portTICK_RATE_MS);    
                }
                mem_pos=0;
            }
            esp_mqtt_client_publish(client_h, UBI_TOPIC, mensaje, 0, 1, 0);
            ESP_LOGI(TAG,"MQTT conected \r\n"); 
            ESP_LOGI(TAG,"%s \r\n", mensaje);
        }else{
            ESP_LOGI(TAG,"MQTT disconected \r\n");
            strcpy(memory[mem_pos],mensaje);
            ESP_LOGI(TAG,"%s \r\n", memory[mem_pos]);
            mem_pos++;
            if(mem_pos>=10){
                mem_pos=0;
            }
        }
        

        
        context = !context;
        vTaskDelay(180000 / portTICK_RATE_MS); //sample time 3 minutes 
    }
}

//---------------------------------------------------------------

static void obtain_time(void)
{
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY);
    
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    // Set timezone to Standard Time to Colombia
    //https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
    setenv("TZ", "<-05>5", 1);
    tzset();
}

static void ADC_conf (void)
{
    adc_config_t adc_config;

    // Depend on menuconfig->Component config->PHY->vdd33_const value
    // When measuring system voltage(ADC_READ_VDD_MODE), vdd33_const must be set to 255.
    adc_config.mode = ADC_READ_TOUT_MODE;
    adc_config.clk_div = 8; // ADC sample collection clock = 80MHz/clk_div = 10MHz
    ESP_ERROR_CHECK(adc_init(&adc_config));
}

void app_main()
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    /*esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    nvs_flash_init();*/
    
    ADC_conf();
    // 2. Create a adc task to read adc value
    wifi_init();
    mqtt_app_start();

    gpio_set_direction(LINK_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(BAT_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(SRC_TEST, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SRC_TEST, GPIO_PULLUP_ONLY);



    while(mqtt_con==0){
        printf("Waiting for stable internet conection \r\n");
        vTaskDelay(2000 /portTICK_RATE_MS);
    }
    

    printf("Internet conection succesfull \r\n");
    obtain_time();
    xTaskCreate(bat_task, "bat_task", 176, NULL, 5, NULL);
    xTaskCreate(adc_task, "adc_task", 4096, NULL, 5, NULL);
}
