#pragma once

#include <Telek.h>

#include <functional>

// TODO: cek kalau max command udah di define dan ada nilainya min 1
#ifndef MAX_COMMAND_HANDLER
#define MAX_COMMAND_HANDLER 10
#endif

typedef std::function<void(Telek& telek, const BotCommand& cmd)> CommandHandler;

struct Command {
  const char* command;
  CommandHandler handler;
};

class CommandRouter {
 private:
  Command* m_handlers;
  uint8_t m_handlerCount;

 public:
  CommandRouter();
  ~CommandRouter();

  void registerCommand(const char* command, CommandHandler handler);
  bool execute(Telek& telek, const BotCommand& cmd) const;
};
