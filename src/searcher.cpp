#include "searcher.h"
#include "colors.h"
#include "directory_searcher.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <type_traits>
#include <unistd.h>

namespace SpecialKey {
const int ENTER_KEY = 10;
const int ESCAPE_KEY = '\033';
const int BACKSPACE = 127;
} // namespace SpecialKey

namespace EscapeCode {
const std::string ESCAPE_CODE = "\033";

const std::string ARROW_UP = "\033[A";
const std::string ARROW_DOWN = "\033[B";
const std::string ARROW_RIGHT = "\033[C";
const std::string ARROW_LEFT = "\033[D";

const std::string HOME_KEY = "\033[H";
const std::string END_KEY = "\033[F";

const std::string DELETE_KEY = "\033[3~";
} // namespace EscapeCode

searcher_config searcher_config_helper(std::vector<std::string> &arguments) {
  int n = arguments.size();
  searcher_config config = {};
  // pattern: [OPTIONS]... [PATTERN]
  for (int i = 0; i < n; ++i) {
    std::string &str = arguments[i];
    if (str[0] == '-') {
      if (i + 1 >= n)
        throw std::runtime_error("invalid arguments");
      std::string option_value = arguments[i + 1];
      std::string option_name = str.substr(1, str.size() - 1);
      if (option_name == "s" || option_name == "source") {
        config.pathname = option_value;
      } else if (option_name == "d" || option_name == "depth") {
        config.max_depth = std::stoi(option_value);
        if (config.max_depth <= 0) {
          throw std::runtime_error(
              "option \"-d\" or \"-depth\" must be greater than 0");
        }
      }
      ++i;
    } else {
      config.pattern = ".*" + str + ".*";
      break;
    }
  }
  return config;
}

bool searcher::setting_modified = false;
struct termios searcher::original_terminal = {};
bool searcher::command_executed = false;

std::vector<std::string> searcher::replace_symbols_in_user_command(
    std::string cmd_with_replace_symbols) {

  command_tokenizer tokenizer(cmd_with_replace_symbols);

  std::vector<std::string> plain_text_query_result;
  std::transform(
      query_result->begin(), query_result->end(),
      std::back_inserter(plain_text_query_result),
      [](directory_node &node) { return node.path + "/" + node.name; });
  auto replaced_cmd = tokenizer.replace_symbols(plain_text_query_result);
  return replaced_cmd;
}

void searcher::set_data_source(std::vector<std::string> &_data_source) {
  display_data_source = _data_source;
  display_offset = 0;
}

std::vector<std::string>
searcher::styling_query_result(std::vector<directory_node> &query_result) {
  std::vector<std::string> result;
  for (size_t i = 0; i < query_result.size(); ++i) {
    auto query = query_result[i];
    std::string styled_text;

    auto q_path = query.path;
    if (q_path.size() == 1 && q_path == ".") {
      q_path = "";
    } else if (q_path.size() >= 2 && q_path.substr(0, 2) == "./") {
      q_path = q_path.substr(2, q_path.size() - 2).append("/");
    } else if (q_path.size() >= 2 && q_path.substr(0, 2) == "//") {
      q_path = q_path.substr(1, q_path.size() - 1).append("/");
    } else if (q_path != "/") {
      q_path.append("/");
    }

    styled_text.append("[" + std::to_string(i + 1) + "] ")
        .append(Colors::Green(q_path));
    if (query.type == directory::FILE) {
      styled_text.append(Colors::Yellow(query.name));
    } else {
      styled_text.append(Colors::Green(query.name));
    }
    result.push_back(styled_text);
  }
  return result;
}

void searcher::execute_user_command(std::string &user_command) {
  auto replaced_cmd = replace_symbols_in_user_command(user_command);
  char *c_style_command[1024];
  int idx = 0;
  for (auto iter = replaced_cmd.begin(); iter != replaced_cmd.end(); ++iter) {
    c_style_command[idx++] = const_cast<char *>((*iter).c_str());
  }
  c_style_command[idx] = nullptr;

  pid_t pid;
  if ((pid = fork()) < 0) {
    std::cerr << "fork error\n";
    exit(1);
  } else if (pid == 0) {
    clear_entire_screen();
    execvp(c_style_command[0], c_style_command);
    exit(0);
  }

  searcher::command_executed = true;
  wait(nullptr);
  exit(0);
}

void searcher::run() {
  query_result = dir_searcher.query();
  auto styled_result = styling_query_result(*query_result);
  enable_raw_mode();

  clear_entire_screen();
  draw_default_layout();

  set_data_source(styled_result);
  draw_rows();
  while (1) {
    int err = process_keypress();
    if (err == -1)
      break;
  }
}

void searcher::enable_raw_mode() {
  backup_current_settings();
  tcgetattr(STDIN_FILENO, &raw_mode);
  raw_mode.c_iflag &= ~(BRKINT | INPCK | ISTRIP | IXON);
  raw_mode.c_cflag |= (CS8);
  raw_mode.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw_mode.c_cc[VMIN] = 0;
  raw_mode.c_cc[VTIME] = 1;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_mode);

  searcher::setting_modified = true;
}

void searcher::backup_current_settings() {
  tcgetattr(STDIN_FILENO, &original_terminal);
}

void searcher::reset_mode() {
  if (searcher::setting_modified) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal);
  }
}

std::string searcher::read_key() {
  char c;
  int n_read;
  while ((n_read = read(STDIN_FILENO, &c, 1)) != 1) {
    if (n_read == -1 && errno != EAGAIN)
      throw "read error";
  }

  std::string key(1, c);
  if (key == EscapeCode::ESCAPE_CODE) {
    char next_keys[2] = {};
    if (read(STDIN_FILENO, &next_keys[0], 1) != 1)
      return EscapeCode::ESCAPE_CODE;
    if (read(STDIN_FILENO, &next_keys[1], 1) != 1)
      return EscapeCode::ESCAPE_CODE;

    if (next_keys[0] == '[') {
      switch (next_keys[1]) {
      case 'A':
        return EscapeCode::ARROW_UP;
      case 'B':
        return EscapeCode::ARROW_DOWN;
      case 'C':
        return EscapeCode::ARROW_RIGHT;
      case 'D':
        return EscapeCode::ARROW_LEFT;
      case 'F':
        return EscapeCode::END_KEY;
      case 'H':
        return EscapeCode::HOME_KEY;
      default:
        return key;
      }
    } else {
      return key;
    }
  }
  return key;
}

int searcher::process_keypress() {
  std::string keys = read_key();
  if (keys == "q") {
    return -1;
  }

  if (keys == "k" || keys == EscapeCode::ARROW_UP) {
    move_line_up();
  } else if (keys == "j" || keys == EscapeCode::ARROW_DOWN) {
    move_line_down();
  } else if (keys == ":") {
    handle_user_command();
  }
  return 0;
}

void searcher::handle_escape_sequence() {
  // char second_key = read_key();
  // char third_key = read_key();

  // if (second_key != '[')
  //   return;
  // switch (third_key) {
  // case 'A':
  //   move_line_up();
  //   break;
  // case 'B':
  //   move_line_down();
  //   break;
  // }
}

void searcher::handle_user_command() {
  const char command_symbol(':');
  write(STDERR_FILENO, &command_symbol, 1);

  // constexpr unsigned int keys_buffer_size = 1024;
  // char keys[keys_buffer_size] = {};

  int current_cursor_position = 0;
  std::string user_command;
  std::string s_keys;
  while (1) {
    s_keys = read_key();
    int n = s_keys.size();

    if (s_keys[0] == SpecialKey::ENTER_KEY ||
        s_keys == EscapeCode::ESCAPE_CODE) {
      break;
    } else if (s_keys[0] == SpecialKey::BACKSPACE) {
      if (current_cursor_position > 0) {
        user_command.erase(current_cursor_position - 1, 1);
        write(STDOUT_FILENO, "\b \b", 3);
        --current_cursor_position;
      }
    } else if (s_keys == EscapeCode::ARROW_UP ||
               s_keys == EscapeCode::ARROW_DOWN) {
      continue;
    } else if (s_keys == EscapeCode::ARROW_LEFT) {
      if (current_cursor_position > 0) {
        write(STDOUT_FILENO, s_keys.c_str(), n);
        --current_cursor_position;
      }
    } else if (s_keys == EscapeCode::ARROW_RIGHT) {
      if (current_cursor_position < user_command.size()) {
        write(STDOUT_FILENO, s_keys.c_str(), n);
        ++current_cursor_position;
      }
    } else if (s_keys == EscapeCode::HOME_KEY) {
      std::string new_pos =
          "\033[" + std::to_string(current_cursor_position) + "D";
      write(STDOUT_FILENO, new_pos.c_str(), new_pos.size());
      current_cursor_position = 0;
    } else if (s_keys == EscapeCode::END_KEY) {
      std::string new_pos =
          "\033[" +
          std::to_string(user_command.size() - current_cursor_position) + "C";
      write(STDOUT_FILENO, new_pos.c_str(), new_pos.size());
      current_cursor_position = user_command.size();
    } else {
      user_command.insert(current_cursor_position, s_keys);
      write(STDOUT_FILENO, "\033[s", 3);
      draw_command_bar();
      write(STDOUT_FILENO, ":", 1);

      write(STDOUT_FILENO, user_command.c_str(), user_command.size());
      write(STDOUT_FILENO, "\033[u", 3);
      write(STDOUT_FILENO, "\033[C", 3);
      ++current_cursor_position;
    }
  }

  if (s_keys[0] == SpecialKey::ENTER_KEY) {
    execute_user_command(user_command);
    exit(0);
  }

  draw_command_bar();
}

void searcher::move_line(int direction) {
  auto window_size = get_terminal_window_size();
  const unsigned short ROW = window_size.first;
  if (direction + display_offset < 0 ||
      direction + (display_offset + ROW - 1) > display_data_source.size())
    return;
  display_offset += direction;
  draw_rows();
}

void searcher::move_line_up() { move_line(-1); }
void searcher::move_line_down() { move_line(1); }

void searcher::move_cursor_to(int row, int col) {
  std::string position("\033[" + std::to_string(row) + ";" +
                       std::to_string(col) + "H");
  write(STDOUT_FILENO, position.c_str(), position.size());
}

void searcher::clear_entire_screen() {
  write(STDOUT_FILENO, "\033[2J", 4);
  write(STDOUT_FILENO, "\033[H", 3);
}

void searcher::clear_current_line() {
  std::string clear_current_line = "\033[2K";
  write(STDOUT_FILENO, clear_current_line.c_str(), clear_current_line.size());
}

void searcher::draw_default_layout() {
  auto window_size = searcher::get_terminal_window_size();
  const unsigned short ROW = window_size.first;
  for (int i = 0; i < ROW - 1; ++i) {
    clear_current_line();
    std::cout << std::endl;
  }
  draw_command_bar();
}

void searcher::draw_command_bar() {
  auto window_size = get_terminal_window_size();
  const auto ROW = window_size.first;
  move_cursor_to(ROW, 1);
  clear_current_line();

  std::string formatted_command_hints =
      "\033[107;30m" + command_hints + "\033[0m";
  write(STDOUT_FILENO, formatted_command_hints.c_str(),
        formatted_command_hints.size());
}

void searcher::draw_rows() {
  searcher::move_cursor_to(1, 1);
  auto window_size = searcher::get_terminal_window_size();
  const auto ROW = window_size.first;
  const auto COL = window_size.second;
  for (int i = 0;
       i + display_offset < display_data_source.size() && i < ROW - 1; ++i) {
    clear_current_line();
    std::string display_text = display_data_source[display_offset + i];
    if (display_text.size() >= COL) {
      display_text = display_text.substr(0, COL)
                         .append(Colors::ESCAPE_SEQ + Colors::RESET_SYMBOL)
                         .append("...)");
    }
    std::cout << display_text << std::endl;
  }
  searcher::move_cursor_to(ROW, command_hints.size() + 1);
}

void searcher::print(const std::string &text...) {
  std::cout << text << std::endl;
}

std::pair<unsigned short, unsigned short> searcher::get_terminal_window_size() {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
    throw "failed to get the windows size";
  return {ws.ws_row, ws.ws_col};
}