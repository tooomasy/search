#include "command_tokenizer.h"
#include <algorithm>
#include <iostream>
#include <searcher.h>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {

  std::vector<std::string> arguments;
  if (argc > 1) {
    arguments.assign(argv + 1, argv + argc);
  }
  auto config = searcher_config_helper(arguments);
  searcher search(config);
  search.run();

  write(STDOUT_FILENO, "\033[2J", 4);
  write(STDOUT_FILENO, "\033[H", 3);

  return 0;
}