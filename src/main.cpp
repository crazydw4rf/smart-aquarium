#include <Arduino.h>
#include <CommandRouter.h>
#include <Telek.h>
#include <Utils.h>
#include <WiFi.h>

#include "handler.h"
#include "secret.h"

// deklarasi variable state waktu
uint32_t lastMessageUpdate = 0;

// deklarasi struct/class instance
MessageBody msgBody = {0};
BotCommand botCmd = {0};
CommandRouter router;

Telek t_client(BOT_TOKEN);

// deklarasi pin
constexpr uint8_t BASE_RELAY_PIN = 20;
constexpr uint8_t PUMP_RELAY = BASE_RELAY_PIN;
constexpr uint8_t LED_RELAY = BASE_RELAY_PIN + 1;

// deklarasi pesan dan perintah yang dikirim
namespace Aqua {
constexpr char START_MESSAGE[] = R"MSG(Hai aku adalah Aqua sebuah bot telegram
yang dapat memberi informasi dan mengontrol smart aquarium kamu.
Untuk perintah selengkapnya kamu bisa kirim perintah /help.)MSG";
constexpr char HELP_MESSAGE[] = R"MSG(*Perintah dasar*
		/start  =>  Untuk menampilkan welcome message
		/help  =>  Untuk menampilkan daftar perintah atau help message
	*Control*
		/led\\_on  =>  Menyalakan lampu led
		/led\\_off  =>  Mematikan lampu led
  /pompa\\_on => Menyalakan pompa air
  /pompa\\_off => Mematikan pompa air
	*Monitor*
  /suhu\\_lapor => Mengirim informasi suhu air
  /air\\_lapor => Mengirim informasi level persentase air
)MSG";  // pake format markdown biar cakep
constexpr char COMMAND_START[] = "/start";
constexpr char COMMAND_HELP[] = "/help";
constexpr char COMMAND_LED[] = "/led";
constexpr char COMMAND_SUHU[] = "/suhu";
constexpr char COMMAND_TINGGI[] = "/tinggi";
const char COMMAND_NOCOMMAND = '\0';
}  // namespace Aqua

void setup() {
  Serial.begin(115200);
  Serial.print("\n\n\n");

  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("mencoba menyambungkan ke jaringan WiFi\nSSID: %s\n",
                WIFI_SSID);

  while (!WiFi.isConnected()) {
    Serial.print(".");
    delay(600);
  }

  Serial.println("\nterhubung ke jaringan wifi");
  delay(1000);

  t_client.setDebugMode();  // hapus baris ini kalau gak mau debug info di
                            // serial monitor

  auto info = t_client.getBotInfo();

  Serial.println("\nbot sudah berjalan");
  Serial.printf("nama bot: %s\n", info.username);

  t_client.setChatId(TELEGRAM_USER_ID);
  t_client.sendMessage("Aku idup lagi woyy!!");

  router.registerCommand(Aqua::COMMAND_START, handle_start);
  router.registerCommand(Aqua::COMMAND_HELP, handle_help);
  router.registerCommand(Aqua::COMMAND_LED, handle_led);
}

// TODO: simpan update_id pada pesan yang terima dan bandingkan dengan yang
// update id dari pesan terakhir jika id sama maka jangan lakukan operasi
// lanjutan (parsing command)
void loop() {
  uint32_t currentTime = millis();

  if (currentTime - lastMessageUpdate >= MESSAGE_UPDATE_INTERVAL) {
    // kalau internet nya jelek, method ini (aqua.getMessageUpdate()) bisa makan
    // waktu lebih dari 3 detik yang mana di cycle loop selanjutnya di bagian
    // kondisi cek waktu interval akan langsung terpenuhi
    if (t_client.getMessageUpdate(msgBody)) {
      Serial.printf("pesan masuk: @%s: '%s'\n", msgBody.sender,
                    msgBody.message);

      if (t_client.parseCommand(botCmd, msgBody.message)) {
        if (!router.execute(t_client, botCmd)) {
          Serial.println("Gagal mengeksekusi perintah");
        }
      }
    }
    lastMessageUpdate = currentTime;
  }

  delay(200);
}

void handle_start(Telek& telek, const BotCommand& cmd) {
  telek.sendMessage(Aqua::START_MESSAGE);
}

void handle_help(Telek& telek, const BotCommand& cmd) {
  telek.sendMessage(Aqua::HELP_MESSAGE);
}

void handle_led(Telek& telek, const BotCommand& cmd) {
  if (streq(cmd.parameter, "on")) {
    digitalWrite(BUILTIN_LED, HIGH);
    telek.sendMessage("Dah nyala bos!");
  } else {
    digitalWrite(BUILTIN_LED, LOW);
    telek.sendMessage("Siap bos!");
  }
}