#include "CommandRouter.h"

#include <Utils.h>

#include <stdexcept>
#include <string>

// constructor
CommandRouter::CommandRouter() {}

CommandRouter::CommandRouter(const CommandMap& handlers)
    : m_handlers(handlers) {}

// destructor
CommandRouter::~CommandRouter() {}

bool CommandRouter::dispatch(Telek& telek, const BotCommand& cmd) const {
  try {
    m_handlers.at(cmd.command)(telek, cmd);
    return true;
  } catch (const std::out_of_range& e) {
    return false;
  }
}