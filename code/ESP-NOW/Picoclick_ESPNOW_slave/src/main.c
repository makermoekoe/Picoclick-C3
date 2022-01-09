#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/event_groups.h"
#include <driver/gpio.h>

#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "esp_sleep.h"

#include "sdkconfig.h"

#include "espnow_basic_config.h"

#include "led_strip.h"

static const char *TAG = "Basic_Slave";

void my_data_populate(my_data_t *data);

static EventGroupHandle_t s_evt_group;

#define MY_ESPNOW_WIFI_MODE WIFI_MODE_STA
#define MY_ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
// #define MY_ESPNOW_WIFI_MODE WIFI_MODE_AP
// #define MY_ESPNOW_WIFI_IF   ESP_IF_WIFI_AP

static void packet_sent_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    assert(status == ESP_NOW_SEND_SUCCESS || status == ESP_NOW_SEND_FAIL);
    xEventGroupSetBits(s_evt_group, BIT(status));
}

static void init_espnow_slave(void){
    const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    ESP_ERROR_CHECK( esp_netif_init() );
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(MY_ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start() );
#if MY_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(MY_ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(packet_sent_cb) );
    ESP_ERROR_CHECK( esp_now_set_pmk((const uint8_t *)MY_ESPNOW_PMK) );

    // Alter this if you want to specify the gateway mac, enable encyption, etc
    const esp_now_peer_info_t broadcast_destination = {
        .peer_addr = MY_RECEIVER_MAC,
        .channel = MY_ESPNOW_CHANNEL,
        .ifidx = MY_ESPNOW_WIFI_IF
    };
    ESP_ERROR_CHECK( esp_now_add_peer(&broadcast_destination) );
}

static esp_err_t send_espnow_data(void){
    const uint8_t destination_mac[] = MY_RECEIVER_MAC;
    static my_data_t data;

    // Go to the user function to populate the data to send
    my_data_populate(&data);

    // Send it
    ESP_LOGI(TAG, "Sending %u bytes to " MACSTR, sizeof(data), MAC2STR(destination_mac));
    esp_err_t err = esp_now_send(destination_mac, (uint8_t*)&data, sizeof(data));
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Send error (%d)", err);
        return ESP_FAIL;
    }

    // Wait for callback function to set status bit
    EventBits_t bits = xEventGroupWaitBits(s_evt_group, BIT(ESP_NOW_SEND_SUCCESS) | BIT(ESP_NOW_SEND_FAIL), pdTRUE, pdFALSE, 2000 / portTICK_PERIOD_MS);
    if(!(bits & BIT(ESP_NOW_SEND_SUCCESS))){
        if (bits & BIT(ESP_NOW_SEND_FAIL)){
            ESP_LOGE(TAG, "Send error");
            return ESP_FAIL;
        }
        ESP_LOGE(TAG, "Send timed out");
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGI(TAG, "Sent!");
    return ESP_OK;
}




static led_strip_t *pStrip_a;
static uint8_t s_led_state = 0;

#define STAT_MCP_PIN    1
#define LATCH_PIN       3
#define ADC_PIN         4
#define BTN_PIN         5
#define LED_PIN         6

#define NUM_LEDS        1
#define LED_RMT_CHANNEL 0


static void configure_led(void){
    pStrip_a = led_strip_init(LED_RMT_CHANNEL, LED_PIN, NUM_LEDS);
    pStrip_a->clear(pStrip_a, 50);
}

int app_main(){
    gpio_reset_pin(LATCH_PIN);
    gpio_set_direction(LATCH_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LATCH_PIN, true);

    configure_led();

    init_espnow_slave();

    pStrip_a->set_pixel(pStrip_a, 0, 0, 0, 0);
    pStrip_a->refresh(pStrip_a, 100);

    s_evt_group = xEventGroupCreate();
    assert(s_evt_group);

    send_espnow_data();

    esp_wifi_deinit();
    esp_wifi_stop();

    pStrip_a->set_pixel(pStrip_a, 0, 0, 0, 50);
    pStrip_a->refresh(pStrip_a, 100);
    vTaskDelay(600 / portTICK_PERIOD_MS);

    gpio_set_level(LATCH_PIN, false);

    while(true){
        blink_led(30, 0, 0);
        s_led_state = !s_led_state;
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}