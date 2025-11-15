#pragma once

#include <WiFiClientSecure.h>

#define BASE_API_URL "https://api.telegram.org/bot"

#define EMPTY_RESPONSE "{}"

// kalau override value dari macro berikut harus sebelum baris
// include file header ini

#ifndef MESSAGE_UPDATE_INTERVAL
#define MESSAGE_UPDATE_INTERVAL 1500
#endif

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 512
#endif

#ifndef PAYLOAD_BUFFER_SIZE
#define PAYLOAD_BUFFER_SIZE 512
#endif

#ifndef BOT_MESSAGE_POLLING_TIMEOUT
#define BOT_MESSAGE_POLLING_TIMEOUT 60
#endif

struct BotInfo {
  char username[20];
};

struct BotCommand {
  char command[16];
  char parameter[16];
};

struct MessageBody {
  char sender[32];
  char message[32];
};

class Telek {
 private:
  WiFiClientSecure* m_wifiClient;

  const char* m_token;
  bool m_isDebugMode;
  char m_baseURL[80];
  char m_fullURL[128];
  char m_chatId[12];
  uint32_t m_lastUpdateId;

 public:
  explicit Telek(const char* token);
  ~Telek();

  bool parseCommand(BotCommand& cmd, const char* str) const;
  BotInfo getBotInfo();
  void sendMessage(const char* msg);
  void sendMessage(const char* chatId, const char* msg);
  bool getMessageUpdate(MessageBody* msgBody);

  void setDebugMode();
  void setChatId(const char* chatId);

 private:
  String HTTPGet();
  String HTTPPost(const char* payload);
  void setMethod(const char* method);
  void TELEK_DEBUG(const char* str);

  template <typename... T>
  void TELEK_DEBUG(const char* format, T... args);
};
