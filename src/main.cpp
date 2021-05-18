#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "user_settings.h"

const char* ssid = STASSID;
const char* password = STAPSK;
// MQTT server X509 fingerprint, get it with:
// openssl s_client -showcerts -connect mqtt_server:8883 </dev/null 2>/dev/null | openssl x509 -sha1 -noout -fingerprint
const char mqtt_fingerprint[] PROGMEM = MQTT_FINGERPRINT;
const char* mqtt_server = MQTT_SERVER;
const char* mqtt_user = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;
const char* mqtt_client = MQTT_CLIENT;
const char* mqtt_topic = MQTT_TOPIC;
const int mqtt_port = MQTT_PORT;
int sensor_value_min = SENSOR_VALUE_MIN;
int sensor_value_max = SENSOR_VALUE_MAX;
int sensor_range_min = 0;
int sensor_range_max = 100;

WiFiClientSecure secureClient;
PubSubClient client(secureClient);

/*
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Callback");
}
*/

int read_sensor_data() {
  int raw_value = analogRead(A0);

  int range_value = map(raw_value, sensor_value_min, sensor_value_max,
                        sensor_range_min, sensor_range_max);
  range_value = max(min(range_value, 100), 0);

  Serial.print("Raw sensor value: ");
  Serial.print(raw_value);
  Serial.println();
  Serial.print("Adjusted sensor value: ");
  Serial.print(range_value);
  Serial.println();

  return range_value;
}

void connect_wifi() {
  Serial.println("Connecting WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi connection Failed!");
    delay(5000);
  }
}

void connect_mqtt() {
  secureClient.setFingerprint(mqtt_fingerprint);
  //secureClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  //client.setCallback(callback);

  Serial.println("Connecting MQTT");
  while (!client.connected()) {
    if(!client.connect(mqtt_client, mqtt_user, mqtt_password)) {
      Serial.println("MQTT connection Failed!");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void send_sensor_value(int value) {
  StaticJsonDocument<256> doc;
  doc["value"] = value;

  char mqtt_payload[256];
  serializeJson(doc, mqtt_payload);

  Serial.println("Sending MQTT payload");
  Serial.println(mqtt_payload);
  while(!client.publish(mqtt_topic, mqtt_payload)) {
      Serial.println("MQTT publish failed!");
      delay(5000);
  }
}

void setup() {
  Serial.begin(115200);
  while(!Serial) { }

  Serial.println("Started");

  connect_wifi();
  connect_mqtt();

  int value = read_sensor_data();
  send_sensor_value(value);

  if(!client.loop()) {
      Serial.println("MQTT loop failed!");
  }

  Serial.println("Goodnight!");
  ESP.deepSleep(DEEP_SLEEP_TIMEOUT);
}

void loop() {
}