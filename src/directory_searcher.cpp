#include "directory_searcher.h"
#include <cstring>
#include <dirent.h>
#include <regex>

std::string directory_node::get_complete_name() {
  if (path == ".")
    return name;
  if (path.size() > 2 && path.substr(0, 2) == "./")
    return path.substr(2, path.size() - 2) + "/" + name;
  return path + "/" + name;
}

void directory_searcher::search_helper(
    std::shared_ptr<std::vector<directory_node>> res,
    const std::string &cur_pathname) {
  auto dir = opendir(cur_pathname.c_str());
  struct dirent *child;
  while (dir != nullptr && (child = readdir(dir)) != nullptr) {
    auto child_dname = child->d_name;
    if (strcmp(".", child_dname) == 0 || strcmp("..", child_dname) == 0)
      continue;

    directory_node node(cur_pathname, child_dname, directory::UNKNOWN);
    switch (child->d_type) {
    case DT_REG:
      node.type = directory::FILE;
      break;
    case DT_DIR:
      node.type = directory::FOLDER;
      break;
    default:
      node.type = directory::UNKNOWN;
    }

    std::string next_path = cur_pathname + "/" + child_dname;
    if (std::regex_match(std::string(next_path), std::regex(pattern))) {
      res->push_back(node);
    }
    search_helper(res, next_path);
  }
}

std::shared_ptr<std::vector<directory_node>>
directory_searcher::query(int depth) {
  auto res = std::make_shared<std::vector<directory_node>>();
  search_helper(res, root_directory);
  return res;
}