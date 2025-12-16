/*
 * Smart Aquarium - ThingSpeak Integration
 *
 * Channel Configuration (1 channel dengan 4 fields):
 *   Field 1: temp (°C) - Suhu air (write)
 *   Field 2: level (%) - Ketinggian air (write)
 *   Field 3: led_state (0=OFF, 1=ON) - (read/write)
 *   Field 4: pump_state (0=OFF, 1=ON) - (read/write)
 *
 * Note: Field 3 & 4 dibaca dari ThingSpeak untuk kontrol relay
 */

#include <Arduino.h>
#include <DallasTemperature.h>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <OneWire.h>
#include <ThingSpeak.h>
#include <Utils.h>

#include "pins.h"
#include "secret.h"

#define PUBLISH_INTERVAL 20000  // 20 detik (rate limit ThingSpeak)
#define READ_INTERVAL 5000      // 5 detik untuk baca kontrol

float waterLevel = 0;
float waterTemp = 0;
bool ledState = false;
bool pumpState = false;

WiFiClient wifiClient;

// Sensor
OneWire onewireBus_1(ONEWIRE_BUS_PIN_1);
DallasTemperature tempSensor(&onewireBus_1);

void connectToWifi();
void sensorUpdate();
void publishAllData();
void readControlState();

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Smart Aquarium ThingSpeak ===");

  pinMode(LED_RELAY, OUTPUT);
  pinMode(PUMP_RELAY, OUTPUT);
#ifdef ESP32
  analogSetPinAttenuation(WATER_LEVEL_SIGNAL_PIN, ADC_11db);
#endif

  // Initial relay state: OFF
  digitalWrite(LED_RELAY, HIGH);
  digitalWrite(PUMP_RELAY, HIGH);

  tempSensor.begin();
  connectToWifi();

  ThingSpeak.begin(wifiClient);

  Serial.println("ThingSpeak client initialized");
}

void loop() {
  static uint32_t lastPublish = 0;
  static uint32_t lastRead = 0;

  // Read control state from ThingSpeak
  if (millis() - lastRead >= READ_INTERVAL) {
    readControlState();
    lastRead = millis();
  }

  // Publish sensor and status data
  if (millis() - lastPublish >= PUBLISH_INTERVAL) {
    sensorUpdate();
    publishAllData();
    lastPublish = millis();
  }

  delay(100);
}

void connectToWifi() {
  Serial.println("Mencoba menyambungkan ke jaringan WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi koneksi tersambung");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
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

inline int writeFields() {
  return ThingSpeak.writeFields(THINGSPEAK_CHANNEL_ID, THINGSPEAK_API_KEY);
}

inline float readField(unsigned int field) {
  return ThingSpeak.readFloatField(THINGSPEAK_CHANNEL_ID, field);
}

inline String readStatus() {
  return ThingSpeak.readStatus(THINGSPEAK_CHANNEL_ID);
}

void readControlState() {
  float ledValue = readField(3);
  if (ledValue >= 0) {  // Valid read
    bool newLedState = (ledValue > 0.5);
    if (newLedState != ledState) {
      digitalWrite(LED_RELAY, newLedState ? LOW : HIGH);
      ledState = newLedState;
      Serial.printf("[ThingSpeak] LED: %s\n", ledState ? "ON" : "OFF");
    }
  }

  delay(500);

  float pumpValue = readField(4);
  if (pumpValue >= 0) {  // Valid read
    bool newPumpState = (pumpValue > 0.5);
    if (newPumpState != pumpState) {
      digitalWrite(PUMP_RELAY, newPumpState ? LOW : HIGH);
      pumpState = newPumpState;
      Serial.printf("[ThingSpeak] Pompa: %s\n", pumpState ? "ON" : "OFF");
    }
  }
}

void publishAllData() {
  ledState = (digitalRead(LED_RELAY) == LOW);
  pumpState = (digitalRead(PUMP_RELAY) == LOW);

  // Publish all data to single channel (4 fields)
  ThingSpeak.setField(1, waterTemp);
  ThingSpeak.setField(2, waterLevel);
  ThingSpeak.setField(3, ledState ? 1 : 0);
  ThingSpeak.setField(4, pumpState ? 1 : 0);

  // Retry logic: 3 attempts with 5 second delay
  int status = 0;
  int attempt = 0;
  const int maxAttempts = 3;
  const int retryDelay = 5000;

  while (attempt < maxAttempts) {
    attempt++;
    status = writeFields();

    if (status == 200) {
      Serial.printf(
          "[ThingSpeak] Suhu=%.2f°C, Level=%.2f%%, LED=%s, Pompa=%s\n",
          waterTemp, waterLevel, ledState ? "ON" : "OFF",
          pumpState ? "ON" : "OFF");
      break;
    } else {
      Serial.printf("[ThingSpeak] Error: %d (attempt %d/%d)\n", status, attempt,
                    maxAttempts);

      if (attempt < maxAttempts) {
        Serial.printf("[ThingSpeak] Retrying in %d seconds...\n",
                      retryDelay / 1000);
        delay(retryDelay);

        // Re-set fields before retry
        ThingSpeak.setField(1, waterTemp);
        ThingSpeak.setField(2, waterLevel);
        ThingSpeak.setField(3, ledState ? 1 : 0);
        ThingSpeak.setField(4, pumpState ? 1 : 0);
      } else {
        Serial.println("[ThingSpeak] Max retries reached, giving up");
      }
    }
  }
}