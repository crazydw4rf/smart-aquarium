#include <Arduino.h>

const uint8_t SENSOR_SIGNAL_PIN = 34;
const uint8_t SENSOR_POWER_PIN = 13;

static uint32_t waterLevel = 0;

void task_sensorUpdater(void*) {
  for (;;) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    digitalWrite(SENSOR_POWER_PIN, HIGH);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    auto raw_value = analogRead(SENSOR_SIGNAL_PIN);
    waterLevel = raw_value;
    digitalWrite(SENSOR_POWER_PIN, LOW);
  }
}

TaskHandle_t sensorHandle = NULL;

void setup() {
  Serial.begin(115200);
  log_i("Hello world");

  analogSetPinAttenuation(SENSOR_SIGNAL_PIN, ADC_11db);

  pinMode(SENSOR_POWER_PIN, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, LOW);

  xTaskCreatePinnedToCore(task_sensorUpdater, "sensorUpdater", 1024, NULL, 1,
                          &sensorHandle, 1);
}

void loop() {
  log_i("water level: %lu", waterLevel);
  delay(500);
}