#include <Arduino.h>
#include <CommandRouter.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Telek.h>
#include <Utils.h>
#include <WiFi.h>

#include <cstring>

#include "handler.h"
#include "secret.h"

#define MESSAGE_UPDATE_INTERVAL 2000
#define SENSOR_UPDATE_INTERVAL 3000

// deklarasi variable state dan nilai sensor
static uint8_t waterLevel = 0;
static float waterTemp = 0.0f;

// deklarasi struct/class instance
Telek botClient(BOT_TOKEN);
MessageBody* msgBody = new MessageBody;
BotCommand botCmd = {0};
CommandRouter router;

// deklarasi pin
const uint8_t WATER_LEVEL_SIGNAL_PIN = 34;  // pin ADC1_6
const uint8_t ONEWIRE_BUS_PIN_1 = 15;
const uint8_t LED_RELAY = 22;
const uint8_t PUMP_RELAY = LED_RELAY + 1;

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
		/led\\_toggle  =>  Switch lampu led on/off
  /pompa\\_toggle => Switch pompa air on/off
	*Monitor*
  /air\\_suhu => Mengirim informasi suhu air
  /air\\_tinggi => Mengirim informasi level persentase tinggi air
  *Status*
  /control\\_status => Mengirim status control saat ini
  /sensor\\_status => Mengirim nilai sensor saat ini
)MSG";  // pake format markdown biar cakep
const char COMMAND_START[] = "/start";
const char COMMAND_HELP[] = "/help";
const char COMMAND_LED[] = "/led";
const char COMMAND_PUMP[] = "/pompa";
const char COMMAND_WATER_MONITOR[] = "/air";  // /air_suhu, /air_tinggi
}  // namespace Aqua

// variabel task handle untuk mengatur task seperti delete, suspend/resume dan
// set prioritas menggunakan fungsi seperti vTaskDelete, vTaskSuspend,
// vTaskResume, vTaskPrioritySet dan lainnya
TaskHandle_t messageUpdaterHandle = NULL;
TaskHandle_t sensorUpdaterHandle = NULL;

void task_sensorUpdater(void*) {
  while (true) {
    // untuk delay di dalam fungsi yang dijalankan dengan task tersendiri
    // harus memakai fungsi vTaskDelay
    vTaskDelay(SENSOR_UPDATE_INTERVAL / portTICK_PERIOD_MS);

    tempSensor.requestTemperatures();
    waterTemp = tempSensor.getTempCByIndex(0);

    // FIXME: kalibrasi nilai sensor dan ubah menjadi persentase
    auto sensorVal = analogRead(WATER_LEVEL_SIGNAL_PIN);
    waterLevel = sensorVal;

    Serial.printf("water temp: %.2f, water level: %hhu\n", waterTemp,
                  waterLevel);
  }
}

void task_messageUpdater(void*) {
  while (true) {
    vTaskDelay(MESSAGE_UPDATE_INTERVAL / portTICK_PERIOD_MS);

    if (botClient.getMessageUpdate(msgBody)) {
      Serial.printf("pesan masuk: @%s: '%s'\n", msgBody->sender,
                    msgBody->message);

      if (botClient.parseCommand(botCmd, msgBody->message)) {
        if (!router.execute(botClient, botCmd)) {
          Serial.println("gagal mengeksekusi perintah");
        }
      }
    }
  }
}

void setup() {
  memset(msgBody, 0, sizeof(MessageBody));

  Serial.begin(115200);
  tempSensor.begin();
  Serial.print("\n\n\n");

  pinMode(LED_RELAY, OUTPUT);
  pinMode(PUMP_RELAY, OUTPUT);
  analogSetPinAttenuation(WATER_LEVEL_SIGNAL_PIN, ADC_11db);

  // kondisi awal relay mati semua
  digitalWrite(LED_RELAY, HIGH);
  digitalWrite(PUMP_RELAY, HIGH);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("mencoba menyambungkan ke jaringan WiFi\nSSID: %s\n",
                WIFI_SSID);

  while (!WiFi.isConnected()) {
    Serial.print(".");
    delay(600);
  }

  Serial.println("\nterhubung ke jaringan wifi");
  delay(1000);

  botClient.setDebugMode();  // hapus baris ini kalau gak mau debug info di
                             // serial monitor

  auto info = botClient.getBotInfo();

  Serial.println("\nbot sudah berjalan");
  Serial.printf("nama bot: %s\n", info.username);

  botClient.sendMessage(TELEGRAM_USER_ID, "Aqua Ready!!");

  router.registerCommand(Aqua::COMMAND_START, handle_start);
  router.registerCommand(Aqua::COMMAND_HELP, handle_help);
  router.registerCommand(Aqua::COMMAND_LED, handle_ctrl_led);
  router.registerCommand(Aqua::COMMAND_PUMP, handle_ctrl_pump);
  router.registerCommand(Aqua::COMMAND_WATER_MONITOR, handle_water_monitor);

  // update message id terakhir agar tidak usah merespon
  // perintah terakhir yang dikirim oleh pengguna
  botClient.getMessageUpdate(nullptr);

  // stack size: 1816 - 1820
  xTaskCreatePinnedToCore(task_sensorUpdater, "sensorUpdater", 2500, NULL, 1,
                          &sensorUpdaterHandle, 1);

  // stack size: 5708 - 5788
  xTaskCreatePinnedToCore(task_messageUpdater, "messageUpdater", 8000, NULL, 2,
                          &messageUpdaterHandle, 1);
}

void loop() { delay(1000); }

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
    sprintf(msg, "Tinggi air:  *%hhu%%*", waterLevel);
    telek.sendMessage(msg);
  } else {
    telek.sendMessage("Ngawur ya boss!");
  }
}
