//prueba computador jony



#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <inttypes.h>



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


static const char *TAG = "MQTT_EXAMPLE";
/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const static int CONNECTED_BIT = BIT0;



esp_mqtt_client_handle_t client_h;
bool mqtt_con = 0;
uint8_t con_flag = 0;

 uint32_t result2;
 

#define BROKER_PORT 1883
#define BROKER_URL  "mqtt://industrial.api.ubidots.com"
#define TOKEN   "BBFF-6QgNNxNG9tBB0BtpKGagXOH6usY0DK"
#define UBI_TOPIC   "/v1.6/devices/esp_8266/temperature"


static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            /*msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "Bienvenidos", 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);*/

            mqtt_con = 1;       //bandera de conexión 

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            con_flag++;
            if(con_flag>=3){
                con_flag=0;
                vTaskDelay(2000 / portTICK_RATE_MS);
                fflush(stdout);
                esp_restart();
            }
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
            .ssid = "jony",//CONFIG_WIFI_SSID,
            .password = "jfcr920210",//CONFIG_WIFI_PASSWORD,
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

//-----------------------adc task----------------------------

static void adc_task()
{
    uint16_t adc_data[2];
    bool context = 0;
    char mensaje[100];
    char context_mensaje[100];

    while(mqtt_con==0){
        printf("Esperando conexión al broker mqtt para iniciar medición de temperatura \r\n");
        vTaskDelay(2000 /portTICK_RATE_MS);
    };

    printf("conexión establecida con el broker MQTT \r\n");

    vTaskDelay(2000 / portTICK_RATE_MS);

    while (1) {
        if (ESP_OK == adc_read(&adc_data[0])) {
            ESP_LOGI(TAG, "adc read: %d\r\n", adc_data[0]);
        }

        if(context){
            sprintf(context_mensaje,"%ccontext%c: {%cname%c: %cAlta Temperatura%c}", '"', '"', '"', '"', '"', '"');
        }else{
            sprintf(context_mensaje,"%ccontext%c: {%cname%c: %cBaja Temperatura%c}", '"', '"', '"', '"', '"', '"');
        }

        sprintf(mensaje,"{%cvalue%c: %d, %s}", '"', '"',adc_data[0], context_mensaje);

        printf("%s \r\n", mensaje);

        esp_mqtt_client_publish(client_h, UBI_TOPIC, mensaje, 0, 1, 0);

        context = !context;

        vTaskDelay(120000 / portTICK_RATE_MS);
    }
}

//---------------------------------------------------------------

//------------------------------------sntp----------------------
static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}


static void obtain_time(void)
{
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY);
    initialize_sntp();

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
}

static void sntp_example_task(void *arg)
{
    time_t now;
    struct tm timeinfo;
   // char strftime_buf[64];

    time(&now);
    localtime_r(&now, &timeinfo);

    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
    }

    // Set timezone to Eastern Standard Time and print local time
    // setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    // tzset();

    // Set timezone to Standard Time
    //https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
    setenv("TZ", "<-05>5", 1);
    tzset();

    while (1) {
        // update 'now' variable with current time
        //time(&now);
        //localtime_r(&now, &timeinfo);

        //time_t result;
        //uint32_t result3;

    	//result = time(&now);
    	char fecha[100];
    	result2 = time(&now);//ubidots requiere recibir el timestamp en milisegundos aqui esta en segundos
    	sprintf(fecha,"%u",resul2);

    	printf("%s",fecha);

    	//result2*=1000;
    	//printf("%s%u secs since the Epoch\n",
        ///asctime(localtime(&result)),
          // result2);

    	//printf("%s%u segundos desde 1970\n",
       // asctime(localtime(&result2)),
          // result2);

    	//printf("%llu segundos desde 1970\r\n",result2);

    	printf("%u \r\n", result2);

        //if (timeinfo.tm_year < (2016 - 1900)) {
           // ESP_LOGE(TAG, "The current date/time error");
        //} else {

        	
            //strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
            
           // ESP_LOGI(TAG, "The current date/time in Bogotá is: %s", strftime_buf);
        //}

        ESP_LOGI(TAG, "Free heap size: %d\n", esp_get_free_heap_size());
        vTaskDelay(10000 / portTICK_RATE_MS);
    }
}



//----------------------------------------------------------












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

    adc_config_t adc_config;

    // Depend on menuconfig->Component config->PHY->vdd33_const value
    // When measuring system voltage(ADC_READ_VDD_MODE), vdd33_const must be set to 255.
    adc_config.mode = ADC_READ_TOUT_MODE;
    adc_config.clk_div = 8; // ADC sample collection clock = 80MHz/clk_div = 10MHz
    ESP_ERROR_CHECK(adc_init(&adc_config));

    // 2. Create a adc task to read adc value
    xTaskCreate(adc_task, "adc_task", 2048, NULL, 5, NULL);




    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);




    //nvs_flash_init();
    wifi_init();

     

    xTaskCreate(sntp_example_task, "sntp_example_task", 2048, NULL, 10, NULL);






    mqtt_app_start();
}
