#pragma once

#include <string.h>

inline bool streq(const char* str1, const char* str2) {
  return strcmp(str1, str2) == 0;
}
