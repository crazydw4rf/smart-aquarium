#include "CommandRouter.h"

#include <Utils.h>

// constructor
CommandRouter::CommandRouter() : m_handlerCount(0) {
  m_handlers = new Command[MAX_COMMAND_HANDLER];

  memset(m_handlers, 0, sizeof(Command) * MAX_COMMAND_HANDLER);
}

// destructor
CommandRouter::~CommandRouter() {
  if (m_handlers != nullptr) {
    delete[] m_handlers;
    m_handlers = nullptr;
  }
}

void CommandRouter::registerCommand(const char* command,
                                    CommandHandler handler) {
  if (m_handlerCount > MAX_COMMAND_HANDLER) return;

  // menambah command handler ke array m_handlers
  m_handlers[m_handlerCount].command = command;
  m_handlers[m_handlerCount].handler = handler;

  m_handlerCount++;
}

bool CommandRouter::execute(Telek& telek, const BotCommand& cmd) const {
  for (int i = 0; i < m_handlerCount; i++) {
    if (streq(cmd.command, m_handlers[i].command)) {
      m_handlers[i].handler(telek, cmd);
      return true;
    }
  }

  return false;
}