#pragma once

#include "command_tokenizer.h"
#include "directory_searcher.h"
#include <asm-generic/ioctls.h>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

struct searcher_config {
  std::string pathname = ".";
  std::string pattern = ".*";
  int max_depth = -1;
  bool list_only = false;
};

searcher_config searcher_config_helper(std::vector<std::string> &arguments);

class searcher {
private:
  directory_searcher dir_searcher;
  struct termios raw_mode;

  int display_offset;
  std::vector<std::string> display_data_source;
  std::string command_hints = " Press 'q' to exit the program ";

  std::shared_ptr<std::vector<directory_node>> query_result;

public:
  static bool setting_modified;
  static struct termios original_terminal;
  static bool command_executed;

  searcher(const searcher_config &config = searcher_config{})
      : dir_searcher(config.pathname, config.pattern), display_offset(0) {
    std::atexit(searcher::reset_to_default);
  }
  ~searcher() { reset_to_default(); }

  void run();

  std::vector<std::string>
  replace_symbols_in_user_command(std::string cmd_with_replace_symbols);

  std::vector<std::string>
  styling_query_result(std::vector<directory_node> &query_result);
  void execute_user_command(std::string &user_command);
  void set_data_source(std::vector<std::string> &_data_source);

  void enable_raw_mode();
  void backup_current_settings();
  void reset_mode();

  std::string read_key();
  int process_keypress();
  void handle_escape_sequence();
  void handle_user_command();

  void move_line(int direction);
  void move_line_up();
  void move_line_down();
  void move_cursor_to(int row, int col);

  void clear_entire_screen();
  void clear_current_line();
  void draw_default_layout();
  void draw_command_bar();
  void draw_rows();
  void print(const std::string &text...);

  std::pair<unsigned short, unsigned short> get_terminal_window_size();

  static void reset_to_default() {
    if (searcher::setting_modified) {
      tcsetattr(STDIN_FILENO, TCSAFLUSH, &searcher::original_terminal);
      if (!searcher::command_executed) {
        write(STDOUT_FILENO, "\033[2J", 4);
        write(STDOUT_FILENO, "\033[H", 3);
        std::string position("\033[1;1H");
        write(STDOUT_FILENO, position.c_str(), position.size());
      }
    }
  }
};
