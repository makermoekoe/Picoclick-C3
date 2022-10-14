#include <Arduino.h>
#include <WiFi.h>
#include <FastLED.h>

// Picoclick C3 hardware definitions
#define ADC_ENABLE_PIN  3
#define APA102_SDI_PIN  7
#define APA102_CLK_PIN  6
#define ADC_PIN         4
#define BUTTON_PIN      5

#define FPC_IO_1        2
#define FPC_IO_2        8
#define FPC_IO_BTN      BUTTON_PIN

#define SDA_PIN         FPC_IO_1
#define SCL_PIN         FPC_IO_2

#define NUM_LEDS        2
CRGB leds[NUM_LEDS];

int color = 0;

void setup(){
  pinMode(BUTTON_PIN, INPUT);
  pinMode(ADC_ENABLE_PIN, OUTPUT);
  pinMode(ADC_PIN, INPUT);
  analogReadResolution(12);

  digitalWrite(ADC_ENABLE_PIN, HIGH);

  btStop();

  WiFi.mode(WIFI_OFF);

  FastLED.addLeds<APA102, APA102_SDI_PIN, APA102_CLK_PIN, BGR>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(180);
  delay(50);

  leds[0] = CRGB::Blue;
  leds[1] = CRGB::Blue;
  FastLED.show();
  
  setCpuFrequencyMhz(10);

  delay(500);
}

unsigned long t_last_press = 0;

void loop() {
  // Change color if the button is pressed. After the eight click, generate random color. Press & hold for 1 second to turn off the device.
  // Device will turn off after 10 seconds not pressing the button.
  if(digitalRead(BUTTON_PIN) == 1){
    color++;

    if(color == 1){
      leds[0] = CRGB::Green;
      leds[1] = CRGB::Green;
    }
    else if(color == 2){
      leds[0] = CRGB::Red;
      leds[1] = CRGB::Red;
    }
    else if(color == 3){
      leds[0] = CRGB::Yellow;
      leds[1] = CRGB::Yellow;
    }
    else if(color == 4){
      leds[0] = CRGB::White;
      leds[1] = CRGB::White;
    }
    else if(color == 5){
      leds[0] = CRGB::Orange;
      leds[1] = CRGB::Orange;
    }
    else if(color == 6){
      leds[0] = CRGB::Aqua;
      leds[1] = CRGB::Aqua;
    }
    else if(color == 7){
      leds[0] = CRGB::HotPink;
      leds[1] = CRGB::HotPink;
    }
    else if(color == 8){
      leds[0] = CRGB::LimeGreen;
      leds[1] = CRGB::LimeGreen;
    }
    else{
      int r = random(255), g = random(255), b = random(255);
      leds[0] = CRGB(r, g, b);
      leds[1] = CRGB(r, g, b);
    }
    FastLED.show();
    unsigned long t_pressed = millis();
    while(digitalRead(BUTTON_PIN) == 1){
      delay(10);
      if(millis() > t_pressed + 1000){
        leds[0] = CRGB::Red;
        leds[1] = CRGB::Red;
        FastLED.show();
        delay(1000);
        esp_deep_sleep_start();
        delay(500);
      }
    }
    t_last_press = millis();

  }

  if(millis() - t_last_press > 10000){
    leds[0] = CRGB::Red;
    leds[1] = CRGB::Red;
    FastLED.show();
    delay(1000);
    esp_sleep_enable_timer_wakeup(5000000);
    esp_deep_sleep_start();
    delay(500);
  }
}