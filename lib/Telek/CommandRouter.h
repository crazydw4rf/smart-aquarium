#pragma once

#include <Telek.h>

#include <map>
#include <string>

typedef std::function<void(Telek& telek, const BotCommand& cmd)> HandlerFunc;
typedef std::map<std::string, HandlerFunc> CommandMap;

class CommandRouter {
 private:
  CommandMap m_handlers;

 public:
  CommandRouter();
  CommandRouter(const CommandMap& handlers);
  ~CommandRouter();

  void registerCommand(const char* command, HandlerFunc handler) {
    m_handlers[command] = handler;
  }
  bool dispatch(Telek& telek, const BotCommand& cmd) const;
  void setRoutes(const CommandMap& routes) { m_handlers = routes; }
  const CommandMap& getRoutes() const { return m_handlers; }
};
