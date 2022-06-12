#pragma once

#include <string>
#include <unistd.h>

namespace Colors {
const std::string ESCAPE_SEQ("\033");
const std::string RESET_SYMBOL(ESCAPE_SEQ + "[0m");

inline std::string formatter(const std::string &text,
                             const std::string color_code) {
  return (isatty(STDOUT_FILENO))
             ? ESCAPE_SEQ + "[0;" + color_code + "m" + text + RESET_SYMBOL
             : text;
}

inline std::string Green(const std::string &text) {
  return formatter(text, "32");
}

inline std::string Yellow(const std::string &text) {
  return formatter(text, "93");
}
}; // namespace Colors