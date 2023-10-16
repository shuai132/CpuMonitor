#pragma once

#include <cstdint>
#include <fstream>
#include <memory>

namespace file_utils {

inline std::shared_ptr<uint8_t> read_file(const std::string &file, size_t *size, bool *ok = nullptr) {
  std::ifstream ifs(file, std::ios::binary);
  if (!ifs) {
    if (ok != nullptr) {
      *ok = false;
      return {};
    } else {
      throw std::runtime_error("failed to open file: " + file);
    }
  }
  // 获取文件大小
  ifs.seekg(0, std::ios::end);
  *size = ifs.tellg();
  ifs.seekg(0, std::ios::beg);

  // 申请数据缓存内存
  std::shared_ptr<uint8_t> fileData(new uint8_t[*size], [](uint8_t *p) {
    delete[] p;
  });
  char *ptr = reinterpret_cast<char *>(fileData.get());

  // 读取文件
  ifs.read(ptr, *size);  // NOLINT(cppcoreguidelines-narrowing-conversions)
  ifs.close();
  *ok = true;
  return fileData;
}

inline bool write_to_file(const void *data, size_t sizeInByte, const std::string &fileName) {
  std::ofstream outfile(fileName, std::ofstream::binary);
  if (!outfile) {
    return false;
  }
  outfile.write((char *)data, sizeInByte);  // NOLINT(cppcoreguidelines-narrowing-conversions)
  outfile.close();
  return true;
}

}  // namespace file_utils
