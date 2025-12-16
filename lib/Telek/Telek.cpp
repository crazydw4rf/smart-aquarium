#include <ArduinoJson.h>
#ifdef ESP32
#include <HTTPClient.h>
#include <WiFi.h>
#else
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#endif
#include <Utils.h>
#include <WiFiClientSecure.h>

#include "Telek.h"

// Log macros for ESP8266
#ifndef ESP32
#ifdef DEBUG_LOG_ENABLE
#define log_d(fmt, ...)                \
  do {                                 \
    Serial.print("[DEBUG] > ");        \
    Serial.printf(fmt, ##__VA_ARGS__); \
    Serial.println();                  \
  } while (0)
#else
#define log_d(...)
#endif
#define log_e(fmt, ...)                \
  do {                                 \
    Serial.print("[ERROR] > ");        \
    Serial.printf(fmt, ##__VA_ARGS__); \
    Serial.println();                  \
  } while (0)
#define log_i(fmt, ...)                \
  do {                                 \
    Serial.print("[INFO] > ");         \
    Serial.printf(fmt, ##__VA_ARGS__); \
    Serial.println();                  \
  } while (0)
#endif

// root CA untuk domain telegram bot API api.telegram.org
const char Go_Daddy_G2_Cert[] = R"CERT(
-----BEGIN CERTIFICATE-----
MIIDxTCCAq2gAwIBAgIBADANBgkqhkiG9w0BAQsFADCBgzELMAkGA1UEBhMCVVMx
EDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxGjAYBgNVBAoT
EUdvRGFkZHkuY29tLCBJbmMuMTEwLwYDVQQDEyhHbyBEYWRkeSBSb290IENlcnRp
ZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTA5MDkwMTAwMDAwMFoXDTM3MTIzMTIz
NTk1OVowgYMxCzAJBgNVBAYTAlVTMRAwDgYDVQQIEwdBcml6b25hMRMwEQYDVQQH
EwpTY290dHNkYWxlMRowGAYDVQQKExFHb0RhZGR5LmNvbSwgSW5jLjExMC8GA1UE
AxMoR28gRGFkZHkgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkgLSBHMjCCASIw
DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL9xYgjx+lk09xvJGKP3gElY6SKD
E6bFIEMBO4Tx5oVJnyfq9oQbTqC023CYxzIBsQU+B07u9PpPL1kwIuerGVZr4oAH
/PMWdYA5UXvl+TW2dE6pjYIT5LY/qQOD+qK+ihVqf94Lw7YZFAXK6sOoBJQ7Rnwy
DfMAZiLIjWltNowRGLfTshxgtDj6AozO091GB94KPutdfMh8+7ArU6SSYmlRJQVh
GkSBjCypQ5Yj36w6gZoOKcUcqeldHraenjAKOc7xiID7S13MMuyFYkMlNAJWJwGR
tDtwKj9useiciAF9n9T521NtYJ2/LOdYq7hfRvzOxBsDPAnrSTFcaUaz4EcCAwEA
AaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYE
FDqahQcQZyi27/a9BUFuIMGU2g/eMA0GCSqGSIb3DQEBCwUAA4IBAQCZ21151fmX
WWcDYfF+OwYxdS2hII5PZYe096acvNjpL9DbWu7PdIxztDhC2gV7+AJ1uP2lsdeu
9tfeE8tTEH6KRtGX+rcuKxGrkLAngPnon1rpN5+r5N9ss4UXnT3ZJE95kTXWXwTr
gIOrmgIttRD02JDHBHNA7XIloKmf7J6raBKZV8aPEjoJpL1E/QYVN8Gb5DKj7Tjo
2GTzLH4U/ALqn83/B2gX2yKQOC16jdFU8WnjXzPKej17CuPKf1855eJ1usV2GDPO
LPAvTK33sefOT6jEm0pUBsV/fdUID+Ic/n4XuKxe9tQWskMJDE32p2u0mYRlynqI
4uJEvlz36hz1
-----END CERTIFICATE-----
)CERT";

#ifdef ESP8266
X509List cert(Go_Daddy_G2_Cert);
#endif

namespace ApiMethod {
const char GETME[] = "getMe";
const char SEND_MESSAGE[] = "sendMessage";
const char GET_UPDATES[] = "getUpdates";
}  // namespace ApiMethod

// destructor
Telek::~Telek() {
  if (m_WiFiClient != nullptr) {
    delete m_WiFiClient;
    m_WiFiClient = nullptr;
  }
}

// constructor
Telek::Telek(const char* token)
    : m_token(token), m_lastUpdateId(0), m_chatId{0} {
  m_WiFiClient = new WiFiClientSecure;
#ifdef ESP32
  m_WiFiClient->setCACert(Go_Daddy_G2_Cert);
#else
  m_WiFiClient->setInsecure();
  // m_WiFiClient->setTrustAnchors(&cert);
#endif
}

String Telek::HTTPGet(const char* apiMethod) {
  HTTPClient client;
  String res;

  String url = buildURL(apiMethod);

  client.begin(*m_WiFiClient, url);

  int code = client.GET();
  if (!(code >= 200 && code < 400))
    res = EMPTY_RESPONSE;
  else
    res = client.getString();

  client.end();

  return res;
}

String Telek::HTTPPost(const char* apiMethod, const String& payload) {
  HTTPClient client;
  String res;

  String url = buildURL(apiMethod);

  client.begin(*m_WiFiClient, url);
  client.addHeader("Content-Type", "application/json");

  int code = client.POST(payload);

  if (!(code >= 200 && code < 400))
    res = EMPTY_RESPONSE;
  else
    res = client.getString();

  client.end();

  return res;
}

BotInfo Telek::getBotInfo() {
  BotInfo me = {0};

  String res = HTTPGet(ApiMethod::GETME);

  if (res == EMPTY_RESPONSE || res.isEmpty()) return me;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, res);
  if (err) {
    log_e("json deserialization error: %s", err.c_str());
    return me;
  }

  JsonObjectConst result = doc["result"];

  if (result.isNull()) return me;

  strncpy(me.username, result["username"].as<const char*>(),
          sizeof(me.username) - 1);

  return me;
}

void Telek::sendMessage(const String& msg) {
  if (msg.length() < 1) return;

  String message;
  JsonDocument doc;
  doc["chat_id"] = m_chatId;
  doc["text"] = msg;
  doc["parse_mode"] = "markdown";

  serializeJson(doc, message);

  log_d("message payload: %s", message.c_str());

  String res = HTTPPost(ApiMethod::SEND_MESSAGE, message);

  if (res == EMPTY_RESPONSE || res.isEmpty()) {
    log_e("gagal mengirim pesan");
    return;
  }

  log_i("pesan berhasil dikirim");
}

void Telek::sendMessage(const char* chatId, const String& msg) {
  setChatId(chatId);
  sendMessage(msg);
}

bool Telek::getMessageUpdate(MessageBody* msgBody) {
  String res =
      HTTPPost(ApiMethod::GET_UPDATES,
               R"({"limit":1,"offset":-1,"allowed_updates":["message"]})");
  if (res == EMPTY_RESPONSE || res.isEmpty()) {
    log_e("tidak ada response dari API");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, res);
  if (err) {
    log_e("json deserialization error: %s\n", err.c_str());
    return false;
  }

  JsonArrayConst result = doc["result"];
  if (!(result && result.size() > 0)) return false;

  JsonObjectConst first = result[0];

  // mencegah pengguna lain untuk memakai bot
  auto userId = first["from"]["id"].as<const char*>();
  if (userId && !streq(userId, m_chatId)) return false;

  // mencegah agar hanya pesan terakhir dan baru dikirm yang akan diproses
  auto update_id = first["update_id"].as<uint32_t>();
  if (update_id > m_lastUpdateId) {
    m_lastUpdateId = update_id;
  } else {
    return false;
  }

  // jika parameter yang diberikan sama dengan nullptr
  // maka hanya update message id terakhir saja
  if (msgBody == nullptr) return false;

  JsonObjectConst msg = first["message"];
  strncpy(msgBody->message, msg["text"].as<const char*>(),
          sizeof(msgBody->message) - 1);
  strncpy(msgBody->sender, msg["from"]["username"].as<const char*>(),
          sizeof(msgBody->sender) - 1);

  return true;
}

bool Telek::parseCommand(BotCommand& cmd, const char* message) const {
  if (message == nullptr || message[0] == '\0') return false;

  // slash command terdiri dari dua string yang dipisah dengan character
  // underscore '_' /led_on, /suhu_lapor

  char buff[32];
  snprintf(buff, sizeof(buff), "%s", message);

  char* token = strtok(buff, "_");  // slash command seperti /led, /suhu
  if (token != NULL)
    strncpy(cmd.command, token, sizeof(cmd.command) - 1);
  else
    return false;

  token = strtok(NULL, " ");  // value atau parameter nya
  if (token != NULL) strncpy(cmd.parameter, token, sizeof(cmd.parameter) - 1);

  return true;
}
