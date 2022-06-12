#pragma once

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace directory {
enum directory_type {
  FOLDER = 0,
  FILE = 1,
  UNKNOWN,
};
}

struct directory_node {
  std::string path;
  std::string name;
  directory::directory_type type;

  directory_node(const std::string &_path, const std::string &_name,
                 directory::directory_type _type)
      : path(_path), name(_name), type(_type) {}
  ~directory_node() = default;

  std::string get_complete_name();
};

class directory_searcher {
private:
  const std::string &root_directory;
  const std::string &pattern;
  void search_helper(std::shared_ptr<std::vector<directory_node>> res,
                     const std::string &cur_pathname);

public:
  directory_searcher(const std::string &_root_directory,
                     const std::string &_pattern)
      : root_directory(_root_directory), pattern(_pattern) {}

  std::shared_ptr<std::vector<directory_node>> query(int depth = -1);

  static void
  print_search_result(std::shared_ptr<std::vector<directory_node>> arr) {
    for (int i = 0; i < arr->size(); ++i) {
      auto node = (*arr)[i];
      printf("[%d] %s\n", i + 1, node.get_complete_name().c_str());
    }
  }
};
