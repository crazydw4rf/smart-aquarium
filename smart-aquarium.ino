#include <WiFi.h>

#include "AquaBot.h"
#include "Utils.h"
#include "secret.h"

const char *wifi_ssid = WIFI_SSID;
const char *wifi_password = WIFI_PASSWORD;
const char *telegram_bot_token = BOT_TOKEN;
const char *user_id = TELEGRAM_USER_ID;

uint32_t lastMessageUpdate = 0;
MessageBody msgBody = {0};
BotCommand cmd = {0};

AquaBot aqua(telegram_bot_token);

const int LED_PIN = BUILTIN_LED;

namespace Aqua {
const char WELLCOME_MESSAGE[] = "Hai aku adalah Aqua sebuah bot telegram\
	yang dapat memberi informasi dan mengontrol smart aquarium kamu.\
	Untuk perintah selengkapnya kamu bisa kirim perintah /help.";
const char HELP_MESSAGE[] = "\
	*Perintah dasar*\n\
		/start  =>  Untuk menampilkan welcome message\n\
		/help  =>  Untuk menampilkan daftar perintah atau help message\n\
	*Control*\n\
		/led\\_on  =>  Menyalakan lampu led\n\
		/led\\_off  =>  Mematikan lampu led\n\
	*Monitor*\n\
	"; // pake format markdown biar cakep
const char *COMMAND_START = "/start";
const char *COMMAND_HELP = "/help";
const char *COMMAND_LED = "/led";
const char *COMMAND_SUHU = "/suhu";
const char *COMMAND_TINGGI = "/tinggi";
const char COMMAND_NOCOMMAND = '\0';
} // namespace Aqua

void setup() {
  Serial.begin(115200);
  Serial.print("\n\n\n");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.begin(wifi_ssid, wifi_password);
  Serial.println("mencoba menyambungkan bot ke server");

  while (!WiFi.isConnected()) {
    Serial.print(".");
    delay(600);
  }

  aqua.setDebugMode();

  auto info = aqua.getBotInfo();

  Serial.println("\nbot sudah berjalan");
  Serial.printf("nama bot: %s\n", info.username);

  aqua.setChatId(user_id);
  aqua.sendMessage("Aku idup lagi woyy!!");
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
    if (aqua.getMessageUpdate(msgBody)) {
      Serial.printf("pesan masuk: @%s: '%s'\n", msgBody.sender,
                    msgBody.message);

      if (aqua.parseCommand(cmd, msgBody.message)) {
        if (streq(cmd.command, Aqua::COMMAND_LED)) {
          if (streq(cmd.parameter, "on")) {
            digitalWrite(LED_PIN, HIGH);
            aqua.sendMessage("Dah nyala bos!");
          } else {
            digitalWrite(LED_PIN, LOW);
            aqua.sendMessage("Siap bos!");
          }
        } else if (streq(cmd.command, Aqua::COMMAND_HELP)) {
          aqua.sendMessage(Aqua::HELP_MESSAGE);
        }
      }
    }
    lastMessageUpdate = currentTime;
  }

  delay(200);
}