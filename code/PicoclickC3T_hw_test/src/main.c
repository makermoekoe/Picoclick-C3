#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#include "led_strip.h"

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
    // pStrip_a->clear(pStrip_a, 100);
}

int app_main(){
    gpio_reset_pin(LATCH_PIN);
    gpio_set_direction(LATCH_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LATCH_PIN, true);

    configure_led();

    vTaskDelay(20 / portTICK_PERIOD_MS);


    pStrip_a->set_pixel(pStrip_a, 0, 50, 0, 0);
    pStrip_a->refresh(pStrip_a, 100);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    pStrip_a->set_pixel(pStrip_a, 0, 0, 50, 0);
    pStrip_a->refresh(pStrip_a, 100);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    pStrip_a->set_pixel(pStrip_a, 0, 0, 0, 50);
    pStrip_a->refresh(pStrip_a, 100);
    vTaskDelay(500 / portTICK_PERIOD_MS);


    gpio_set_level(LATCH_PIN, false);

    while(true){
        if(s_led_state){
            pStrip_a->set_pixel(pStrip_a, 0, 50, 0, 0);
        }
        else{
            pStrip_a->set_pixel(pStrip_a, 0, 0, 0, 0);
        }
        pStrip_a->refresh(pStrip_a, 100);
        s_led_state = !s_led_state;
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}