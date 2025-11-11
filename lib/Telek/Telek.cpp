#include "Telek.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// root CA untuk domain telegram bot API api.telegram.org
constexpr char Go_Daddy_G2[] = R"CERT(
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

namespace ApiMethods {
constexpr char GETME[] = "getMe";
constexpr char SEND_MESSAGE[] = "sendMessage";
constexpr char GET_UPDATES[] = "getUpdates";
}  // namespace ApiMethods

// destructor
Telek::~Telek() {
  if (m_wifiClient != nullptr) {
    delete m_wifiClient;
    m_wifiClient = nullptr;
  }
}

// constructor
Telek::Telek(const char* token)
    : m_token(token), m_isDebugMode(false), m_lastUpdateId(0), m_chatId("") {
  m_wifiClient = new WiFiClientSecure;
  m_wifiClient->setCACert(Go_Daddy_G2);

  snprintf(m_baseURL, sizeof(m_baseURL), "%s%s/", BASE_API_URL, m_token);
}

void Telek::setMethod(const char* method) {
  snprintf(m_fullURL, sizeof(m_fullURL), "%s%s", m_baseURL, method);
}

String Telek::HTTPGet() {
  HTTPClient client;
  String res;

  client.begin(*m_wifiClient, m_fullURL);

  int code = client.GET();
  if (!(code >= 200 && code < 400))
    res = EMPTY_RESPONSE;
  else
    res = client.getString();

  client.end();

  return res;
}

String Telek::HTTPPost(const char* payload) {
  HTTPClient client;
  String res;

  client.begin(*m_wifiClient, m_fullURL);
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

  setMethod(ApiMethods::GETME);
  String res = HTTPGet();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, res);
  if (err) {
    TELEK_DEBUG("json deserialization error: %s\n", err.c_str());
    doc.clear();
    return me;
  }

  strncpy(me.username, doc["result"]["username"].as<const char*>(),
          sizeof(me.username) - 1);

  doc.clear();

  return me;
}

void Telek::sendMessage(const char* msg) {
  if (m_chatId[0] == '\0') return;

  setMethod(ApiMethods::SEND_MESSAGE);

  // FIXME: buat buffer sendiri untuk message karena jika string message
  // membuat payload melebihi kapasitas buffer, bisa jadi nanti payload atau
  // string nya kepotong dan format json nya jadi nggak valid
  char payload[PAYLOAD_BUFFER_SIZE] = {0};
  snprintf(payload, sizeof(payload) - 1,
           R"({"chat_id":"%s","text":"%s","parse_mode":"markdown"})", m_chatId,
           msg);

  String res = HTTPPost(payload);

  if (res == EMPTY_RESPONSE || res.isEmpty()) {
    TELEK_DEBUG("gagal mengirim pesan");
    return;
  }

  TELEK_DEBUG("pesan berhasil dikirim");
}

void Telek::sendMessage(const char* chatId, const char* msg) {
  setChatId(chatId);
  sendMessage(msg);
}

bool Telek::getMessageUpdate(MessageBody& msgBody) {
  setMethod(ApiMethods::GET_UPDATES);

  char payload[64] = {0};
  strcpy(payload, R"({"limit":1,"offset":-1,"allowed_updates":["message"]})");

  String res = HTTPPost(payload);
  if (res == EMPTY_RESPONSE || res.isEmpty()) {
    TELEK_DEBUG("tidak ada response dari API");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, res);
  if (err) {
    TELEK_DEBUG("json deserialization error: %s\n", err.c_str());
    doc.clear();
    return false;
  }

  if (!(doc["result"].size() >= 1)) {
    doc.clear();
    return false;
  }

  // mencegah agar hanya pesan terakhir dan baru dikirm yang akan diproses
  auto update_id = doc["result"][0]["update_id"].as<uint32_t>();
  if (update_id > m_lastUpdateId) {
    m_lastUpdateId = update_id;
  } else {
    doc.clear();
    return false;
  }

  JsonObject msg = doc["result"][0]["message"];

  strncpy(msgBody.message, msg["text"].as<const char*>(),
          sizeof(msgBody.message) - 1);
  strncpy(msgBody.sender, msg["from"]["username"].as<const char*>(),
          sizeof(msgBody.sender) - 1);

  msg.clear();
  doc.clear();

  return true;
}

bool Telek::parseCommand(BotCommand& cmd, const char* str) const {
  if (str == nullptr || str[0] == '\0') return false;

  // slash command terdiri dari dua string yang dipisah dengan character
  // underscore '_' /led_on, /suhu_lapor

  char buf[32] = {0};
  strncpy(buf, str, sizeof(buf) - 1);

  char* token = strtok(buf, "_");  // slash command seperti /led, /suhu
  if (token != NULL)
    strncpy(cmd.command, token, sizeof(cmd.command) - 1);
  else
    return false;

  token = strtok(NULL, "");  // value atau parameter nya
  if (token != NULL) strncpy(cmd.parameter, token, sizeof(cmd.parameter) - 1);

  return true;
}

void Telek::setChatId(const char* chatId) { strcpy(m_chatId, chatId); }

void Telek::setDebugMode() { m_isDebugMode = true; }

void Telek::TELEK_DEBUG(const char* str) {
  if (m_isDebugMode) {
    Serial.printf("[Telek]: %s\n", str);
  }
}

template <typename... T>
void Telek::TELEK_DEBUG(const char* format, T... args) {
  if (m_isDebugMode) {
    char buf[LOG_BUFFER_SIZE] = {0};
    snprintf(buf, sizeof(buf) - 1, format, args...);
    TELEK_DEBUG(buf);
  }
}