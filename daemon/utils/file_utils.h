#pragma once

#include <fstream>

namespace file_utils {

std::string read_text_file(const std::string &file, bool *ok = nullptr) {
  std::ifstream fs(file);
  if (fs) {
    std::string content((std::istreambuf_iterator<char>(fs)), (std::istreambuf_iterator<char>()));
    fs.close();
    *ok = true;
    return content;
  } else {
    *ok = false;
    return "";
  }
}

}  // namespace file_utils
