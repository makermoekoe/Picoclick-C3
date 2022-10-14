#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <FastLED.h>
#include <SparkFunLIS3DH.h>
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

LIS3DH lis(I2C_MODE, 0x19); //Default constructor is I2C, addr 0x19.

void configIntterupts();


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

#define ESPNOW_ID 8888 // Random 4 digit number
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

  Wire.begin(SDA_PIN, SCL_PIN);
  lis.settings.accelSampleRate = 50;  //Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
  lis.settings.accelRange = 2;      //Max G force readable.  Can be: 2, 4, 8, 16

  lis.settings.adcEnabled = 0;
  lis.settings.tempEnabled = 0;
  lis.settings.xAccelEnabled = 1;
  lis.settings.yAccelEnabled = 1;
  lis.settings.zAccelEnabled = 1;
  
  //Call .begin() to configure the IMU
  lis.begin();

  int dataToWrite = B01001111;
  lis.writeRegister(LIS3DH_CTRL_REG1, dataToWrite);
  
  configIntterupts();

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

void configIntterupts(){
  uint8_t dataToWrite = 0;

  // //LIS3DH_INT1_CFG   
  // //dataToWrite |= 0x80;//AOI, 0 = OR 1 = AND
  // //dataToWrite |= 0x40;//6D, 0 = interrupt source, 1 = 6 direction source
  // //Set these to enable individual axes of generation source (or direction)
  // // -- high and low are used generically
  // //dataToWrite |= 0x20;//Z high
  // //dataToWrite |= 0x10;//Z low
  // dataToWrite |= 0x08;//Y high
  // //dataToWrite |= 0x04;//Y low
  // //dataToWrite |= 0x02;//X high
  // //dataToWrite |= 0x01;//X low
  // lis.writeRegister(LIS3DH_INT1_CFG, dataToWrite);
  
  // //LIS3DH_INT1_THS   
  // dataToWrite = 0;
  // //Provide 7 bit value, 0x7F always equals max range by accelRange setting
  // dataToWrite |= 0x10; // 1/8 range
  // lis.writeRegister(LIS3DH_INT1_THS, dataToWrite);
  
  // //LIS3DH_INT1_DURATION  
  // dataToWrite = 0;
  // //minimum duration of the interrupt
  // //LSB equals 1/(sample rate)
  // dataToWrite |= 0x01; // 1 * 1/50 s = 20ms
  // lis.writeRegister(LIS3DH_INT1_DURATION, dataToWrite);
  
  //LIS3DH_CLICK_CFG   
  dataToWrite = 0;
  //Set these to enable individual axes of generation source (or direction)
  // -- set = 1 to enable
  //dataToWrite |= 0x20;//Z double-click
  dataToWrite |= 0x10;//Z click
  //dataToWrite |= 0x08;//Y double-click 
  dataToWrite |= 0x04;//Y click
  //dataToWrite |= 0x02;//X double-click
  dataToWrite |= 0x01;//X click
  lis.writeRegister(LIS3DH_CLICK_CFG, dataToWrite);
  
  //LIS3DH_CLICK_SRC
  dataToWrite = 0;
  //Set these to enable click behaviors (also read to check status)
  // -- set = 1 to enable
  //dataToWrite |= 0x20;//Enable double clicks
  dataToWrite |= 0x04;//Enable single clicks
  //dataToWrite |= 0x08;//sine (0 is positive, 1 is negative)
  dataToWrite |= 0x04;//Z click detect enabled
  dataToWrite |= 0x02;//Y click detect enabled
  dataToWrite |= 0x01;//X click detect enabled
  lis.writeRegister(LIS3DH_CLICK_SRC, dataToWrite);
  
  //LIS3DH_CLICK_THS   
  dataToWrite = 0;
  //This sets the threshold where the click detection process is activated.
  //Provide 7 bit value, 0x7F always equals max range by accelRange setting
  dataToWrite |= 0x0A; // ~1/16 range
  lis.writeRegister(LIS3DH_CLICK_THS, dataToWrite);
  
  //LIS3DH_TIME_LIMIT  
  dataToWrite = 0;
  //Time acceleration has to fall below threshold for a valid click.
  //LSB equals 1/(sample rate)
  dataToWrite |= 0x08; // 0x08: 8 * 1/50 s = 160ms
  lis.writeRegister(LIS3DH_TIME_LIMIT, dataToWrite);
  
  //LIS3DH_TIME_LATENCY
  dataToWrite = 0;
  //hold-off time before allowing detection after click event
  //LSB equals 1/(sample rate)
  dataToWrite |= 0x0F; // 4 * 1/50 s = 160ms, 
  lis.writeRegister(LIS3DH_TIME_LATENCY, dataToWrite);
  
  //LIS3DH_TIME_WINDOW 
  dataToWrite = 0;
  //hold-off time before allowing detection after click event
  //LSB equals 1/(sample rate)
  dataToWrite |= 0x8F; // 16 * 1/50 s = 320ms
  lis.writeRegister(LIS3DH_TIME_WINDOW, dataToWrite);

  //LIS3DH_CTRL_REG5
  //Int1 latch interrupt and 4D on  int1 (preserve fifo en)
  lis.readRegister(&dataToWrite, LIS3DH_CTRL_REG5);
  dataToWrite &= 0xF3; //Clear bits of interest
  dataToWrite |= 0x08; //Latch interrupt (Cleared by reading int1_src)
  //dataToWrite |= 0x04; //Pipe 4D detection from 6D recognition to int1?
  lis.writeRegister(LIS3DH_CTRL_REG5, dataToWrite);

  //LIS3DH_CTRL_REG3
  //Choose source for pin 1
  dataToWrite = 0;
  dataToWrite |= 0x80; //Click detect on pin 1
  // dataToWrite |= 0x40; //AOI1 event (Generator 1 interrupt on pin 1)
  // dataToWrite |= 0x20; //AOI2 event ()
  //dataToWrite |= 0x10; //Data ready
  //dataToWrite |= 0x04; //FIFO watermark
  //dataToWrite |= 0x02; //FIFO overrun
  lis.writeRegister(LIS3DH_CTRL_REG3, dataToWrite);
 
  // //LIS3DH_CTRL_REG6
  // //Choose source for pin 2 and both pin output inversion state
  // dataToWrite = 0;
  // // dataToWrite |= 0x80; //Click int on pin 2
  // // dataToWrite |= 0x40; //Generator 1 interrupt on pin 2
  // //dataToWrite |= 0x10; //boot status on pin 2
  // //dataToWrite |= 0x02; //invert both outputs
  // lis.writeRegister(LIS3DH_CTRL_REG6, dataToWrite);
}