#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
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
struct_message_recv data_recv;

#define ESPNOW_ID 8888 // Random number
uint8_t receiver_address[] = {0x10, 0x91, 0xA8, 0x32, 0x7B, 0x70}; // Mac address of the receiver.
uint8_t local_address[] = {0x10, 0x91, 0xA8, 0x33, 0xD3, 0x9C}; // 10:91:A8:33:D3:9C

bool espnow_answer_received = false;

void on_data_recv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&data_recv, incomingData, sizeof(data_recv));
  espnow_answer_received = true;
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

  btStop();

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
  
  esp_now_peer_info_t peerInfo;

  memcpy(peerInfo.peer_addr, receiver_address, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
      
  if(esp_now_add_peer(&peerInfo) != ESP_OK){
    printf("Failed to add peer\r\n");
    return;
  }

  esp_now_register_recv_cb(on_data_recv);

  // Fill ESPNOW struct with values.
  data.id = ESPNOW_ID;
  memcpy(data.local_mac, local_address, 6);
  data.value = 1;
  data.battery_level = int(get_battery_voltage());
  data.single_tap_duration = 1000;

  esp_now_send(receiver_address, (uint8_t *) &data, sizeof(data));

  // wait on espnow answer
  unsigned long t_wait_answer_start = millis();
  while(!espnow_answer_received && millis() <= t_wait_answer_start + 300){
    delayMicroseconds(1);
  }

  // This will reduce power consumption.
  WiFi.mode(WIFI_OFF);
  setCpuFrequencyMhz(10);

  CRGB col = espnow_answer_received ? CRGB::Green : CRGB::Red;
  leds[0] = col;
  leds[1] = col;
  FastLED.show();
  delay(500);
  
  int counter = 0;
  while(digitalRead(BUTTON_PIN) == 1){
    leds[0] = counter % 2 == 0 ? CRGB::Blue : CRGB::Black;
    leds[1] = (counter+1) % 2 == 0 ? CRGB::Blue : CRGB::Black;
    FastLED.show();
    delay(50);
    counter++;
  }

  leds[0] = CRGB::Blue;
  leds[1] = CRGB::Blue;
  FastLED.show();
  delay(500);

  // Add a loop which will wait as long as the button is pressed before entering deepsleep.
  // Once in deepsleep the USB console is not available anymore.
  esp_deep_sleep_start();
}

void loop() {
  
}