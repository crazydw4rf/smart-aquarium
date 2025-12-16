#pragma once

#include <WiFiClientSecure.h>

#define BASE_API_URL "https://api.telegram.org/bot"

#define EMPTY_RESPONSE "{}"

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
  const char* m_token;
  uint32_t m_lastUpdateId;
  char m_chatId[12];
  WiFiClientSecure* m_WiFiClient;

 public:
  explicit Telek(const char* token);
  ~Telek();

  bool parseCommand(BotCommand& cmd, const char* message) const;
  BotInfo getBotInfo();
  void sendMessage(const String& msg);
  void sendMessage(const char* chatId, const String& msg);
  bool getMessageUpdate(MessageBody* msgBody);

  void setChatId(const char* chatId);

 private:
  String HTTPGet(const char* apiMethod);
  String HTTPPost(const char* apiMethod, const String& payload);
  String buildURL(const char* apiMethod);
};

inline void Telek::setChatId(const char* chatId) {
  strncpy(m_chatId, chatId, sizeof(m_chatId) - 1);
}

inline String Telek::buildURL(const char* apiMethod) {
  return String(BASE_API_URL) + m_token + "/" + apiMethod;
}