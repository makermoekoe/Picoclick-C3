#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <PubSubClient.h>

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PWD";
const char* mqtt_server = "YOUR_BROKER_IP";

// Gateway MAC Address: 7C:DF:A1:55:B6:B2

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

int value = 0;
int bat_level = 0;


#define LED_PIN     13

typedef struct struct_message {
    int id;
    int value;
    int battery_level;
    int single_tap_duration;
} struct_message;

struct_message data;
bool new_data_received = false;
bool new_data_to_mqtt = false;
unsigned long t_new_data_received = 0;

#define N_SINGLETAP_SLOTS   10
bool reset_after_single_tap = false;
unsigned long t_reset_single_tap[N_SINGLETAP_SLOTS];
int reset_after_single_tap_id[N_SINGLETAP_SLOTS];
int single_tap_duration[N_SINGLETAP_SLOTS];



void on_data_recv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  digitalWrite(LED_PIN, 1);
  memcpy(&data, incomingData, sizeof(data));
  Serial.print("MAC: ");
  String mac_str = String(mac[0], HEX) + ":" + String(mac[1], HEX) + ":" + String(mac[2], HEX) + ":" + String(mac[3], HEX);
  Serial.println(mac_str);
  Serial.print("ID: ");
  Serial.println(data.id);
  Serial.print("VALUE: ");
  Serial.println(data.value);
  Serial.print("BAT: ");
  Serial.println(data.battery_level);
  Serial.print("SINGLETAP DURATION: ");
  Serial.println(data.single_tap_duration);

  Serial.println();

  if(data.single_tap_duration > 0){
    reset_after_single_tap = true;
    bool slot_found = false;
    for(int i=0; i<N_SINGLETAP_SLOTS; i++){
      if(single_tap_duration[i] == 0 && !slot_found){
        single_tap_duration[i] = data.single_tap_duration;
        reset_after_single_tap_id[i] = data.id;
        t_reset_single_tap[i] = millis();
        slot_found = true;
      }
    }
  }

  new_data_received = true;
  new_data_to_mqtt = true;
  t_new_data_received = millis();
}


void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("MQTT message arrived on topic: ");
  Serial.print(topic);
  Serial.print(": ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    digitalWrite(LED_PIN, 1);
    // Attempt to connect
    if (client.connect("MQTTClient")) {
      Serial.println("connected");
      client.subscribe("home/#");
      digitalWrite(LED_PIN, 0);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}



void setup() {
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(115200);
  Serial.println();

  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  WiFi.mode(WIFI_AP_STA);

  Serial.print("Get WiFi channel... ");
  int32_t channel = getWiFiChannel(ssid);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  Serial.println(channel);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, 1);
    delay(250);
    Serial.print(".");
    digitalWrite(LED_PIN, 0);
    delay(250);
  }

  Serial.println("");
  Serial.println("WiFi connected with IP:");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(on_data_recv);
}

void loop() {
  if(new_data_received){
    if(new_data_to_mqtt){
      new_data_to_mqtt = false;

      String topic_value = "home/node" + String(data.id) + "/value";
      String topic_battery = "home/node" + String(data.id) + "/battery";
      char topic_value_char[topic_value.length() + 1];
      char topic_battery_char[topic_battery.length() + 1];
      topic_value.toCharArray(topic_value_char, topic_value.length() + 1);
      topic_battery.toCharArray(topic_battery_char, topic_battery.length() + 1);

      char value_str[8];
      dtostrf(data.value, 1, 2, value_str);
      client.publish(topic_value_char, value_str);

      char bat_str[8];
      dtostrf(data.battery_level, 1, 2, bat_str);
      client.publish(topic_battery_char, bat_str);
    }

    if(millis() >= t_new_data_received + 500){
      digitalWrite(LED_PIN, 0);
      new_data_received = false;
    }
  }

  if(reset_after_single_tap){
    bool any_reset_open = false;
    for(int i=0; i<N_SINGLETAP_SLOTS; i++){
      if(single_tap_duration[i] > 0){
        any_reset_open = true;
        if(millis() >= t_reset_single_tap[i] + single_tap_duration[i]){
          Serial.println("send zero");
          String topic_value = "home/node" + String(reset_after_single_tap_id[i]) + "/value";
          char topic_value_char[topic_value.length() + 1];
          topic_value.toCharArray(topic_value_char, topic_value.length() + 1);
          char value_str[8];
          dtostrf(0, 1, 2, value_str);
          client.publish(topic_value_char, value_str);

          single_tap_duration[i] = 0;
          reset_after_single_tap_id[i] = 0;
          delay(2);
        }
      }
    }
    if(!any_reset_open){
      reset_after_single_tap = false;
    }
  }

  if (!client.connected()) {
    reconnect();
  }

  client.loop();
}
