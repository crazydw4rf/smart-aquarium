#pragma once

#include <Telek.h>

// TODO: cek kalau max command udah di define dan ada nilainya min 1
#ifndef MAX_COMMAND_HANDLER
#define MAX_COMMAND_HANDLER 10
#endif

typedef void (*CommandHandlerFunc)(Telek& telek, const BotCommand& cmd);

struct CommandHandler {
  const char* command;
  CommandHandlerFunc handler;
};

class CommandRouter {
 private:
  CommandHandler* m_handlers;
  int m_handlerCount;

 public:
  CommandRouter();
  ~CommandRouter();

  void registerCommand(const char* command, CommandHandlerFunc handler);
  bool execute(Telek& telek, const BotCommand& cmd) const;
};