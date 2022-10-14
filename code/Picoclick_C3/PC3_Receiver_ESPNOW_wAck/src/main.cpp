#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <FastLED.h>
#include <U8g2lib.h>
#include <Wire.h>

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

// ESPNOW packet structure. Can be modified but should be the same on the receivers side.
typedef struct struct_message {
  int id;
  uint8_t local_mac[6];
  int value;
  int battery_level;
  int single_tap_duration;
} struct_message;

typedef struct struct_message_recv {
    bool answer;
} struct_message_recv;

struct_message data;
struct_message_recv data_answer;

#define ESPNOW_ID 8888 // Random 4 digit number
uint8_t receiver_address[] = {0x10, 0x91, 0xA8, 0x32, 0x7B, 0x70}; // Mac address of the receiver. 10:91:A8:32:7B:70
uint8_t local_address[] = {0x10, 0x91, 0xA8, 0x32, 0x7B, 0x70};

uint8_t temp_address[6];
uint8_t last_recv_address[6];

String mac;

bool need_answer = false;
bool answer_sent = false;
int oled_recv_state = 0;
int click_counter = 0;

#define MAX_DISPS   2
int current_display = 0;


unsigned long t_answer_send_on = 0;

void on_data_recv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&data, incomingData, sizeof(data));
  memcpy(temp_address, mac, 6);
  need_answer = true;
  oled_recv_state = 0;
  click_counter++;
}

esp_now_peer_info_t peerInfo;

void send_answer(){
  memcpy(peerInfo.peer_addr, temp_address, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
      
  if(esp_now_add_peer(&peerInfo) != ESP_OK){
    printf("Failed to add peer\r\n");
    return;
  }

  printf("send data_answer\r\n");

  data_answer.answer = true;

  esp_now_send(temp_address, (uint8_t *) &data_answer, sizeof(data_answer));
  if(esp_now_del_peer(temp_address) != ESP_OK){
    printf("Failed to delete peer\r\n");
    return;
  }
  memcpy(last_recv_address, temp_address, 6);
  memset(temp_address, 0, 6);

  answer_sent = true;
  t_answer_send_on = millis();
  leds[0] = CRGB::Green;
  leds[1] = CRGB::Green;
  FastLED.show();
}

String mac_to_string(uint8_t *addr){
  String mac_str = String(addr[0], HEX) + ":" + String(addr[1], HEX) + ":" + String(addr[2], HEX) + ":"
    + String(addr[3], HEX) + ":" + String(addr[4], HEX) + ":" + String(addr[5], HEX);
  mac_str.toUpperCase();
  return mac_str;
}

// U8X8_SSD1306_128X64_NONAME_HW_I2C u8g2(/* reset=*/ U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

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

int left = 0, width = 128, top = 0, height = 64;


void print_text(bool clear_buf, String str, fonts f, box_alignment align, float pos, bool send_buf) {
  if(clear_buf) u8g2.clearBuffer();
  u8g2.setFont(f.font);
  int x = 0;
  if(align == Center) x = left + 0.5 * (width - (f.width * str.length() - 1));
  else if(align == Left) x = left + 5;
  else if(align == Right) x = left + width - f.width * str.length() - 5;
  u8g2.setCursor(x, top + pos * height + f.height / 2);
  u8g2.print(str);
  if(send_buf) u8g2.sendBuffer();
}

float get_battery_voltage(){
  digitalWrite(ADC_ENABLE_PIN, LOW);
  delayMicroseconds(10);
  int sum = 0;
  for(int i=0; i<100; i++){
    sum = sum + analogRead(ADC_PIN);
  }
  float result = sum/100.0;
  digitalWrite(ADC_ENABLE_PIN, HIGH);
  return float(result) * (1.42) - 50;
}


void setup(){
  pinMode(BUTTON_PIN, INPUT);
  pinMode(ADC_ENABLE_PIN, OUTPUT);
  pinMode(ADC_PIN, INPUT);
  analogReadResolution(12);

  digitalWrite(ADC_ENABLE_PIN, HIGH);

  WiFi.mode(WIFI_STA);

  FastLED.addLeds<APA102, APA102_SDI_PIN, APA102_CLK_PIN, BGR>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(160);
  delay(50);

  if(esp_now_init() != ESP_OK) {
    printf("Error initializing ESP-NOW\r\n");
    return;
  }

  leds[0] = CRGB::Blue;
  leds[1] = CRGB::Blue;
  FastLED.show();

  esp_now_register_recv_cb(on_data_recv);

  delay(50);

  leds[0] = CRGB::Blue;
  leds[1] = CRGB::Blue;
  FastLED.show();

  mac = WiFi.macAddress();
  // printf("MAC %s\r\n", mac.c_str());

  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);

  u8g2.begin();
  delay(100);

  print_text(true, "HEY", font_xl, Center, 0.5, true);

  leds[0] = CRGB::Green;
  leds[1] = CRGB::Green;
  FastLED.show();
  delay(1500);

  print_text(true, "Waiting for click...", font_s, Center, 0.15, true);
}

unsigned long led_timer = millis();
unsigned long t_display = 0;
int hue1 = 0, hue2 = 0;
int brightness = 255;

void loop() {
  if(digitalRead(BUTTON_PIN) == 1){
    leds[0] = CRGB::Red;
    leds[1] = CRGB::Red;
    FastLED.show();
    print_text(true, "BYE", font_xl, Center, 0.5, true);
    delay(1000);
    esp_sleep_enable_timer_wakeup(5000000);
    esp_deep_sleep_start();
  }

  if(need_answer){
    need_answer = false;
    send_answer();
    oled_recv_state = 1;
    led_timer += 1000;
    print_text(true, "Received click!", font_xs, Center, 0.15, false);
    print_text(false, mac_to_string(last_recv_address), font_s, Center, 0.5, false);
    print_text(false, String("Time: " + String(millis()/1000) + "s"), font_s, Center, 0.85, true);
  }

  if(answer_sent){
    if(millis() >= t_answer_send_on + 1000){
      t_answer_send_on = millis();
      if(oled_recv_state == 1){
        
      }
      else if(oled_recv_state == 2){
        
      }
      else{
        answer_sent = false;
        current_display = 0;
      }
      oled_recv_state++;
    }
  }
  else{
    if(millis() >= t_display + 5000){
      t_display = millis();
      if(current_display == 0){
        print_text(true, "Waiting for click...", font_s, Center, 0.15, false);
        print_text(false, String(click_counter), font_xl, Center, 0.5, false);
        print_text(false, "total clicks", font_s, Center, 0.85, true);
      }
      else if(current_display == 1){
        print_text(true, "Waiting for click...", font_s, Center, 0.15, false);
        print_text(false, mac, font_s, Center, 0.5, false);
        print_text(false, "local address", font_s, Center, 0.85, true);
      }
      current_display = (current_display+1) % MAX_DISPS;
    }
  }

  if(millis() >= led_timer + 15){
    led_timer = millis();
    leds[0] = CHSV(hue1, 255, brightness);
    leds[1] = CHSV(hue2, 255, brightness);
    FastLED.show();

    hue1 = (hue1 + 1)%255;
    hue2 = (hue1 + 127)%255;
  }
}