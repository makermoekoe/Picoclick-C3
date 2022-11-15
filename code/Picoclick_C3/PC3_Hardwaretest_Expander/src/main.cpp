#include <Arduino.h>
#include <WiFi.h>
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_MCP23X08.h>
#include "config.h"

#define NUM_LEDS        2
CRGB leds[NUM_LEDS];

Adafruit_MCP23X08 mcp;

void setup(){
  pinMode(BUTTON_PIN, INPUT);
  pinMode(ADC_ENABLE_PIN, OUTPUT);
  pinMode(ADC_PIN, INPUT);
  analogReadResolution(12);
  digitalWrite(ADC_ENABLE_PIN, HIGH);

  btStop();
  WiFi.mode(WIFI_OFF);

  FastLED.addLeds<APA102, APA102_SDI_PIN, APA102_CLK_PIN, BGR>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(160);
  delay(50);

  leds[0] = CRGB::Blue;
  leds[1] = CRGB::Blue;
  FastLED.show();

  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);

  if(!mcp.begin_I2C()){
    printf("Error.\r\n");
    leds[0] = CRGB::Red;
    leds[1] = CRGB::Red;
    FastLED.show();
    delay(3000);
    esp_deep_sleep_start();
  }

  for(int i=0; i<8; i++) mcp.pinMode(i, OUTPUT);
}

unsigned long t_expander = millis() + 300;

int counter = 0;
bool dir = true;

void loop() {
  if(digitalRead(BUTTON_PIN) == 1){
    leds[0] = CRGB::Red;
    leds[1] = CRGB::Red;
    FastLED.show();
    delay(500);
    esp_deep_sleep_start();
  }

  if(millis() >= t_expander + 1000){
    t_expander = millis();
    if(dir) mcp.digitalWrite(counter, 1);
    else mcp.digitalWrite(counter, 0);

    counter++;
    if(counter == 8){
      counter = 0;
      dir = !dir;
    }
  }
}