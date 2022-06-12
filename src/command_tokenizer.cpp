#include "command_tokenizer.h"
#include <iterator>
#include <map>

std::vector<std::string>
command_tokenizer::split_string(const std::string &delimiter) {
  std::vector<std::string> res;
  size_t start = 0;
  size_t end = 0;
  std::string token;
  while (end < input_command.size() &&
         (end = input_command.find(delimiter, start)) != std::string::npos) {
    token = input_command.substr(start, end - start);
    start = end + 1;
    if (token == "")
      continue;
    res.push_back(token);
  }
  if (end == std::string::npos) {
    res.push_back(input_command.substr(start, input_command.size() - start));
  }
  return res;
}

bool command_tokenizer::is_target_token(const std::string &token) {
  // the desired token format: _[0-9]+
  return token.size() > 1 && token[0] == '_' &&
         std::find_if(token.begin() + 1, token.end(), [](unsigned char c) {
           return !std::isdigit(c);
         }) == token.end();
}

int command_tokenizer::convert_token_to_number(const std::string &token) {
  return std::stoi(token.substr(1, token.size() - 1));
}

std::vector<std::string>
command_tokenizer::replace_symbols(std::vector<std::string> &lookup) {
  std::vector<std::string> res;
  std::vector<std::string> target_list = split_string(" ");
  for (auto &token : target_list) {
    auto new_token = token;
    if (is_target_token(token)) {
      int converted_value = convert_token_to_number(token);
      if (converted_value == 0) {
        res.insert(res.end(), lookup.begin(), lookup.end());
        continue;
      }

      int index = converted_value - 1;
      if (index < lookup.size()) {
        new_token = lookup[index];
      }
    }
    res.push_back(new_token);
  }
  return res;
}
