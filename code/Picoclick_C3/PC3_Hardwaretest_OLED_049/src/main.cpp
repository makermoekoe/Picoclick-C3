#include <Arduino.h>
#include <WiFi.h>
#include <FastLED.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "config.h"

#define NUM_LEDS        2
CRGB leds[NUM_LEDS];

U8G2_SSD1306_64X32_1F_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

struct fonts {
  const uint8_t* font;
  int width;
  int height;
};

enum box_alignment {
  Center,
  Left,
  Right
};

fonts font_xl = {u8g2_font_profont29_mf, 16, 19};
fonts font_m = {u8g2_font_profont17_mf, 9, 11};
fonts font_s = {u8g2_font_profont12_mf, 6, 8};
fonts font_xs = {u8g2_font_profont10_mf, 5, 6};

int left = 0, width = 64, top = 0, height = 32;

u8g2_uint_t scrolling_offset = 0;
u8g2_uint_t scrolling_width;
const char *scrolling_text = "makermoekoe ";

void print_text(String str, fonts f, box_alignment align, float pos) {
  u8g2.setFont(f.font);
  int x = 0;
  if(align == Center) x = left + 0.5 * (width - (f.width * str.length() - 1));
  else if(align == Left) x = left + 5;
  else if(align == Right) x = left + width - f.width * str.length() - 5;
  u8g2.firstPage();  
  do{
    u8g2.drawStr(x, top + pos * height + f.height / 2, str.c_str());
  } while(u8g2.nextPage());
}

void setup(){
  pinMode(BUTTON_PIN, INPUT);
  pinMode(ADC_ENABLE_PIN, OUTPUT);
  pinMode(ADC_PIN, INPUT);
  analogReadResolution(12);
  digitalWrite(ADC_ENABLE_PIN, HIGH);

  WiFi.mode(WIFI_OFF);

  FastLED.addLeds<APA102, APA102_SDI_PIN, APA102_CLK_PIN, BGR>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(100);
  delay(50);

  leds[0] = CRGB::Blue;
  leds[1] = CRGB::Blue;
  FastLED.show();

  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);
  u8g2.begin();
  delay(100);
  print_text("HEY", font_xl, Center, 0.5);
  delay(1000);

  scrolling_width = u8g2.getUTF8Width(scrolling_text);		// calculate the pixel width of the text
  // u8g2.setFontMode(0);
}

unsigned long t_display = 0;

void loop() {
  if(digitalRead(BUTTON_PIN) == 1){
    leds[0] = CRGB::Red;
    leds[1] = CRGB::Red;
    FastLED.show();
    print_text("BYE", font_xl, Center, 0.5);
    delay(1000);
    esp_deep_sleep_start();
  }

  if(millis() >= t_display + 15){
    t_display = millis();
    u8g2_uint_t x;
  
    u8g2.firstPage();
    do{
      // draw the scrolling text at current offset
      x = scrolling_offset + 64;
      u8g2.setFont(font_m.font);		// set the target font
      do{								// repeated drawing of the scrolling text...
        u8g2.drawUTF8(x, 20, scrolling_text);			// draw the scolling text
        x += scrolling_width;						// add the pixel width of the scrolling text
      } while(x < 64);		// draw again until the complete display is filled
      
    } while(u8g2.nextPage() );
    
    scrolling_offset-=1;							// scroll by one pixel
    if((u8g2_uint_t)scrolling_offset < (u8g2_uint_t)-scrolling_width){
      scrolling_offset = 0;							// start over again
    }
  }
}