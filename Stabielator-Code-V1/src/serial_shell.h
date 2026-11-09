/**
 * @file serial_shell.h
 * @brief Interface en invoerbuffer voor de niet-blokkerende USB-opdrachtverwerker.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#pragma once
#include "config.h"
class SerialShell {
 public:
  void begin();
  void update();
 private:
  void handleCommand(const char* command);
  char _buffer[SERIAL_COMMAND_BUFFER_SIZE] = {};
  size_t _length = 0;
  bool _overflow = false;
};
extern SerialShell serialShell;
