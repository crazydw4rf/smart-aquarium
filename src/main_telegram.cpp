/**
 * @author Binar Nugroho <binarnugroho775@gmail.com>
 *
 *
 * @note
 * https://github.com/crazydw4rf/smart-aquarium
 *
 * Kode ini dibuat untuk memenuhi tugas kuliah yaitu proyek final pada mata
 * kuliah Sistem Pengendali Mikro yaitu dengan membuat atau merancang alat untuk
 * sistem kendali jarak jauh, monitoring sensor, dan lain-lain dengan
 * menggunakan mikrokontroler berbasis IoT (Internet of Things).
 *
 * Ide proyek yang dipilih adalah Smart Aquarium yang mana cara kerjanya adalah
 * untuk membuat sistem kendali jarak jauh lewat aplikasi pesan instan Telegram
 * dan membaca beberapa parameter sensor seperti sensor suhu dan ketinggian air
 * pada aquarium.
 *
 * Untuk implementasi pada metode komunikasi dan monitoring menggunakan
 * Telegram, kode ini sudah mendukung di kedua jenis chip atau jenis mcu board
 * yaitu ESP32 dan ESP8266.
 */
#include <Arduino.h>
#include <CommandRouter.h>
#include <DallasTemperature.h>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <OneWire.h>
#include <Telek.h>
#include <Utils.h>

#include <string>

#include "pins.h"
#include "secret.h"

#define MESSAGE_UPDATE_INTERVAL 2000
#define SENSOR_UPDATE_INTERVAL 3000

// deklarasi konstanta rentang nilai sensor yang aman
const float WATER_TEMP_SAFE_MIN = 28;  // derajat celcius
const float WATER_TEMP_SAFE_MAX = 34;
const float WATER_LEVEL_SAFE_MAX = 100;  // persentase
const float WATER_LEVEL_SAFE_MIN = 80;

// deklarasi variable state dan nilai sensor
float waterLevel = 0;  // persentase ketinggian air
float waterTemp = 0;

// deklarasi struct/class instance
Telek botClient(BOT_TOKEN);
MessageBody* msgBody = new MessageBody{};
CommandRouter router;
BotCommand botCmd = {0};

// sensor suhu air
OneWire onewireBus_1(ONEWIRE_BUS_PIN_1);
DallasTemperature tempSensor(&onewireBus_1);

// deklarasi pesan dan perintah yang dikirim
namespace Aqua {
const char START_MESSAGE[] = R"MSG(Hai aku adalah Aqua sebuah bot telegram
yang dapat memberi informasi dan mengontrol smart aquarium kamu.
Untuk perintah selengkapnya kamu bisa kirim perintah /help.)MSG";
const char HELP_MESSAGE[] = R"MSG(*Perintah dasar*
		/start  =>  Untuk menampilkan welcome message
		/help  =>  Untuk menampilkan daftar perintah atau help message
	*Control*
		/led\_toggle  =>  Switch lampu led on/off
  /pompa\_toggle => Switch pompa air on/off
	*Monitor*
  /air\_suhu => Mengirim informasi suhu air
  /air\_tinggi => Mengirim informasi level persentase tinggi air
  *Status*
  /status\_control => Mengirim status control saat ini
  /status\_sensor => Mengirim nilai sensor saat ini
)MSG";  // pake format markdown biar cakep
const char COMMAND_START[] = "/start";
const char COMMAND_HELP[] = "/help";
const char COMMAND_LED[] = "/led";
const char COMMAND_PUMP[] = "/pompa";
const char COMMAND_WATER_MONITOR[] = "/air";  // /air_suhu, /air_tinggi
const char COMMAND_STATUS[] = "/status";      // /status_control, /status_sensor
}  // namespace Aqua

// variabel task handle untuk mengatur task seperti delete, suspend/resume dan
// set prioritas menggunakan fungsi seperti vTaskDelete, vTaskSuspend,
// vTaskResume, vTaskPrioritySet dan lainnya
#ifdef ESP32
TaskHandle_t messageUpdaterHandle = NULL;
TaskHandle_t sensorUpdaterHandle = NULL;
void task_sensorUpdater(void*);
void task_messageUpdater(void*);
#endif

// deklarasi command handler untuk telegram
void handle_start(Telek& telek, const BotCommand& cmd);
void handle_help(Telek& telek, const BotCommand& cmd);
void handle_ctrl_led(Telek& telek, const BotCommand& cmd);
void handle_ctrl_pump(Telek& telek, const BotCommand& cmd);
void handle_water_monitor(Telek& telek, const BotCommand& cmd);
void handle_status(Telek& telek, const BotCommand& cmd);

CommandMap commandHandlers{
    {Aqua::COMMAND_START, handle_start},
    {Aqua::COMMAND_HELP, handle_help},
    {Aqua::COMMAND_LED, handle_ctrl_led},
    {Aqua::COMMAND_PUMP, handle_ctrl_pump},
    {Aqua::COMMAND_WATER_MONITOR, handle_water_monitor},
    {Aqua::COMMAND_STATUS, handle_status},
};

void setup() {
  Serial.begin(115200);
  tempSensor.begin();

  router.setRoutes(commandHandlers);

  pinMode(LED_RELAY, OUTPUT);
  pinMode(PUMP_RELAY, OUTPUT);
#ifdef ESP32
  analogSetPinAttenuation(WATER_LEVEL_SIGNAL_PIN, ADC_11db);
#endif

  // kondisi awal relay mati semua
  digitalWrite(LED_RELAY, HIGH);
  digitalWrite(PUMP_RELAY, HIGH);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("\n\nMencoba menyambungkan ke jaringan WiFi...");

  while (!WiFi.isConnected()) {
    Serial.println(".");
    delay(500);
  }

  Serial.printf("Tersambung ke jaringan WiFi dengan SSID: %s\n", WIFI_SSID);
  delay(1000);

  auto botInfo = botClient.getBotInfo();

  Serial.printf("Bot telegram sudah berjalan dengan nama: %s\n",
                botInfo.username);

  botClient.sendMessage(TELEGRAM_USER_ID, "Aqua Ready!!");
  botClient.getMessageUpdate(nullptr);

#ifdef ESP32
  xTaskCreatePinnedToCore(task_sensorUpdater, "sensorUpdater", 2048, NULL, 1,
                          &sensorUpdaterHandle, 1);
  xTaskCreatePinnedToCore(task_messageUpdater, "messageUpdater", 8192, NULL, 1,
                          &messageUpdaterHandle, 1);
#endif
}

void sensorUpdate() {
  constexpr float MAX_SENSOR_VALUE = 260;
  tempSensor.requestTemperatures();
  waterTemp = tempSensor.getTempCByIndex(0);

  pinMode(WATER_LEVEL_POWER_PIN, OUTPUT);
  digitalWrite(WATER_LEVEL_POWER_PIN, HIGH);
#ifdef ESP32
  vTaskDelay(50 / portTICK_PERIOD_MS);
#else
  delay(50);
#endif
  auto sensor_raw = analogRead(WATER_LEVEL_SIGNAL_PIN);
  digitalWrite(WATER_LEVEL_POWER_PIN, LOW);
  float sensor_persen = (sensor_raw / MAX_SENSOR_VALUE) * 100.0f;
  if (sensor_persen > 100) sensor_persen = 100;
  waterLevel = sensor_persen;
  Serial.printf("suhu air: %.2f, tinggi air: %.2f%%\n", waterTemp, waterLevel);
}

void messageUpdate() {
  if (botClient.getMessageUpdate(msgBody)) {
    Serial.printf("pesan masuk: @%s: '%s'\n", msgBody->sender,
                  msgBody->message);
    if (botClient.parseCommand(botCmd, msgBody->message)) {
      if (!router.dispatch(botClient, botCmd)) {
        Serial.println("gagal menjalankan perintah");
      }
    }
  }
}

#ifdef ESP32
void loop() { delay(1000); }

void task_sensorUpdater(void*) {
  while (true) {
    sensorUpdate();
    vTaskDelay(SENSOR_UPDATE_INTERVAL / portTICK_PERIOD_MS);
  }
}

void task_messageUpdater(void*) {
  while (true) {
    messageUpdate();
    vTaskDelay(MESSAGE_UPDATE_INTERVAL / portTICK_PERIOD_MS);
  }
}
#else
void loop() {
  static uint32_t lastMessageUpdate = 0;
  static uint32_t lastSensorUpdate = 0;

  if (millis() - lastMessageUpdate >= MESSAGE_UPDATE_INTERVAL) {
    messageUpdate();
    lastMessageUpdate = millis();
  }

  if (millis() - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
    sensorUpdate();
    lastSensorUpdate = millis();
  }

  delay(20);
}
#endif

void handle_start(Telek& telek, const BotCommand& cmd) {
  telek.sendMessage(Aqua::START_MESSAGE);
}

void handle_help(Telek& telek, const BotCommand& cmd) {
  telek.sendMessage(Aqua::HELP_MESSAGE);
}

void handle_ctrl_led(Telek& telek, const BotCommand& cmd) {
  static bool state = false;
  if (streq(cmd.parameter, "toggle") && !state) {
    digitalWrite(LED_RELAY, LOW);
    telek.sendMessage("Lampu sudah menyala bos!");
  } else if (state) {
    digitalWrite(LED_RELAY, HIGH);
    telek.sendMessage("Siap bos!");
  }

  state = !state;
}

void handle_ctrl_pump(Telek& telek, const BotCommand& cmd) {
  static bool state = false;
  if (streq(cmd.parameter, "toggle") && !state) {
    digitalWrite(PUMP_RELAY, LOW);
    telek.sendMessage("Pompa air sudah menyala bos!");
  } else if (state) {
    digitalWrite(PUMP_RELAY, HIGH);
    telek.sendMessage("Siap bos!");
  }

  state = !state;
}

void handle_water_monitor(Telek& telek, const BotCommand& cmd) {
  if (streq(cmd.parameter, "suhu")) {
    char msg[32];
    sprintf(msg, "Suhu air:  *%.1f°C*", waterTemp);
    telek.sendMessage(msg);
  } else if (streq(cmd.parameter, "tinggi")) {
    char msg[32];
    sprintf(msg, "Tinggi air:  *%.2f%%*", waterLevel);
    telek.sendMessage(msg);
  } else {
    telek.sendMessage("Ngawur ya boss!");
  }
}

void handle_status(Telek& telek, const BotCommand& cmd) {
  if (streq(cmd.parameter, "control")) {
    char msg[128];
    const char* ledStatus = digitalRead(LED_RELAY) == LOW ? "ON" : "OFF";
    const char* pumpStatus = digitalRead(PUMP_RELAY) == LOW ? "ON" : "OFF";
    sprintf(msg, "*Status Kontrol:*\nLED: %s\nPompa: %s", ledStatus,
            pumpStatus);
    telek.sendMessage(msg);
  } else if (streq(cmd.parameter, "sensor")) {
    char msg[128];
    sprintf(msg, "*Status Sensor:*\nSuhu air: %.1f°C\nTinggi air: %.2f%%",
            waterTemp, waterLevel);
    telek.sendMessage(msg);
  } else {
    telek.sendMessage("Gunakan /status\\_control atau /status\\_sensor");
  }
}
