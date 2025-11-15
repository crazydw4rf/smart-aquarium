#include <Telek.h>

// function handler untuk setiap perintah
void handle_start(Telek& telek, const BotCommand& cmd);
void handle_help(Telek& telek, const BotCommand& cmd);
void handle_ctrl_led(Telek& telek, const BotCommand& cmd);
void handle_ctrl_pump(Telek& telek, const BotCommand& cmd);
void handle_water_temp(Telek& telek, const BotCommand& cmd);
void handle_water_level(Telek& telek, const BotCommand& cmd);