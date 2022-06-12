#pragma once

#include <string>
#include <vector>

class command_tokenizer {
private:
  const std::string input_command;

public:
  command_tokenizer(const std::string &_input_command)
      : input_command(_input_command) {}

  std::vector<std::string> replace_symbols(std::vector<std::string> &lookup);
  std::vector<std::string> split_string(const std::string &delimiter);
  bool is_target_token(const std::string &token);
  int convert_token_to_number(const std::string &token);
};
