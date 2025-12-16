/*
 * Smart Aquarium
 *
 * Topik MQTT:
 * - aquarium/command  (subscribe) - Menerima perintah kontrol
 * - aquarium/sensor   (publish)   - Mempublikasikan data sensor
 * - aquarium/control  (publish)   - Mempublikasikan status perangkat
 *
 * Payload JSON:
 *
 * Perintah (diterima di aquarium/command):
 *   Heartbeat:
 *     {"type": "heartbeat"}
 *
 *   Kontrol perangkat:
 *     {"type": "control", "device": "led|pump", "state": "on|off"}
 *
 * Data sensor (dipublikasikan di aquarium/sensor):
 *   {"type": "sensor", "temp": 25.5, "level": 85.2, "timestamp": 12345}
 *
 * Status kontrol (dipublikasikan di aquarium/control):
 *   {"type": "control_status", "led": "on|off", "pump": "on|off"}
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncMqttClient.h>
#include <DallasTemperature.h>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266WiFiType.h>
#endif
#include <OneWire.h>
#include <Ticker.h>
#include <Utils.h>

#include "pins.h"
#include "secret.h"

#define SENSOR_UPDATE_INTERVAL 3000

const char* TOPIC_COMMAND = "aquarium/command";
const char* TOPIC_SENSOR = "aquarium/sensor";
const char* TOPIC_CONTROL = "aquarium/control";

float waterLevel = 0;
float waterTemp = 0;
bool shouldPublishSensor = false;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
Ticker wifiReconnectTimer;

// Sensor
OneWire onewireBus_1(ONEWIRE_BUS_PIN_1);
DallasTemperature tempSensor(&onewireBus_1);

void connectToWifi();
void connectToMqtt();
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttMessage(char* topic, char* payload,
                   AsyncMqttClientMessageProperties properties, size_t len,
                   size_t index, size_t total);
void handleCommand(const JsonDocument& doc);
void publishSensorData();
void publishControlStatus();
void sensorUpdate();

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Smart Aquarium MQTT ===");

  pinMode(LED_RELAY, OUTPUT);
  pinMode(PUMP_RELAY, OUTPUT);
#ifdef ESP32
  analogSetPinAttenuation(WATER_LEVEL_SIGNAL_PIN, ADC_11db);
#endif

  // Initial relay state: OFF
  digitalWrite(LED_RELAY, HIGH);
  digitalWrite(PUMP_RELAY, HIGH);

  // MQTT callbacks
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USER, MQTT_PASSWORD);

  // WiFi event handlers
#ifdef ESP32
  WiFi.onEvent(
      [](WiFiEvent_t event, WiFiEventInfo_t info) {
        Serial.println("WiFi koneksi tersambung");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        connectToMqtt();
      },
      WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

  WiFi.onEvent(
      [](WiFiEvent_t event, WiFiEventInfo_t info) {
        Serial.println("WiFi koneksi terputus");
        mqttReconnectTimer.detach();
        wifiReconnectTimer.once(2, connectToWifi);
      },
      WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
#else
  static WiFiEventHandler wifiConnectHandler;
  static WiFiEventHandler wifiDisconnectHandler;

  wifiConnectHandler =
      WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event) {
        Serial.println("WiFi koneksi tersambung");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        connectToMqtt();
      });

  wifiDisconnectHandler = WiFi.onStationModeDisconnected(
      [](const WiFiEventStationModeDisconnected& event) {
        Serial.println("WiFi koneksi terputus");
        mqttReconnectTimer.detach();
        wifiReconnectTimer.once(2, connectToWifi);
      });
#endif

  connectToWifi();
}

void loop() {
  static uint32_t lastSensorUpdate = 0;

  if (millis() - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
    sensorUpdate();

    if (shouldPublishSensor) {
      publishSensorData();
      shouldPublishSensor = false;
    }

    lastSensorUpdate = millis();
  }

  delay(20);
}

void connectToWifi() {
  Serial.println("Mencoba menyambungkan ke jaringan WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() { mqttClient.connect(); }

void onMqttConnect(bool sessionPresent) {
  Serial.println("MQTT koneksi tersambung");

  // Subscribe to command topic
  mqttClient.subscribe(TOPIC_COMMAND, 0);

  // Publish initial status
  publishControlStatus();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("MQTT koneksi terputus");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttMessage(char* topic, char* payload,
                   AsyncMqttClientMessageProperties properties, size_t len,
                   size_t index, size_t total) {
  // Create null-terminated string
  char message[len + 1];
  memcpy(message, payload, len);
  message[len] = '\0';
  Serial.print("< ");
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }

  handleCommand(doc);
}

void handleCommand(const JsonDocument& doc) {
  const char* type = doc["type"];

  if (!type) {
    Serial.println("Missing 'type' field");
    return;
  }

  if (streq(type, "heartbeat")) {
    Serial.println("Heartbeat diterima dari klien");
    shouldPublishSensor = true;

  } else if (streq(type, "control")) {
    const char* device = doc["device"];
    const char* state = doc["state"];

    if (!device || !state) {
      Serial.println("Missing 'device' or 'state' field");
      return;
    }

    bool turnOn = streq(state, "on");

    if (streq(device, "led")) {
      digitalWrite(LED_RELAY, turnOn ? LOW : HIGH);
      Serial.printf("LED: %s\n", turnOn ? "ON" : "OFF");
      publishControlStatus();

    } else if (streq(device, "pump")) {
      digitalWrite(PUMP_RELAY, turnOn ? LOW : HIGH);
      Serial.printf("Pump: %s\n", turnOn ? "ON" : "OFF");
    } else {
      Serial.print("Unknown device: ");
      Serial.println(device);
    }
  } else {
    Serial.print("Unknown command type: ");
    Serial.println(type);
  }
}

void publishSensorData() {
  JsonDocument doc;
  doc["type"] = "sensor";
  doc["temp"] = waterTemp;
  doc["level"] = waterLevel;
  doc["timestamp"] = millis();

  char buffer[128];
  size_t len = serializeJson(doc, buffer);

  mqttClient.publish(TOPIC_SENSOR, 0, false, buffer, len);
  Serial.print("> ");
  Serial.println(buffer);
}

void publishControlStatus() {
  JsonDocument doc;
  doc["type"] = "control_status";
  doc["led"] = (digitalRead(LED_RELAY) == LOW) ? "on" : "off";
  doc["pump"] = (digitalRead(PUMP_RELAY) == LOW) ? "on" : "off";

  char buffer[128];
  size_t len = serializeJson(doc, buffer);

  mqttClient.publish(TOPIC_CONTROL, 0, true, buffer, len);
}

void sensorUpdate() {
  tempSensor.requestTemperatures();
  waterTemp = tempSensor.getTempCByIndex(0);

  pinMode(WATER_LEVEL_POWER_PIN, OUTPUT);
  digitalWrite(WATER_LEVEL_POWER_PIN, HIGH);
  delay(50);

  int raw = analogRead(WATER_LEVEL_SIGNAL_PIN);
  digitalWrite(WATER_LEVEL_POWER_PIN, LOW);

  float percent = (raw / 260.0f) * 100.0f;
  if (percent > 100.0f) percent = 100.0f;
  waterLevel = percent;

  Serial.printf("suhu air: %.2f°C, tinggi air: %.2f%%\n", waterTemp,
                waterLevel);
}