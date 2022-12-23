#pragma once

#include <array>
#include <csignal>
#include <ostream>
#include <string>

#include "defer.h"

namespace Shell {

struct Result {
  int exitCode;
  std::string output;
  bool operator==(const Result &rhs) const {
    return output == rhs.output && exitCode == rhs.exitCode;
  }
  bool operator!=(const Result &rhs) const {
    return !(rhs == *this);
  }
  explicit operator bool() const {
    return exitCode == 0;
  }
  friend std::ostream &operator<<(std::ostream &os, const Result &result) {
    os << "exit code: " << result.exitCode << " output: " << result.output;
    return os;
  }
};

Result exec(const std::string &command) {
  std::array<char, 8192> buffer{};
  std::string result;
#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#define WIFEXITED
#define WEXITSTATUS
#else
  struct sigaction act_tmp, act_old;  // NOLINT
  act_tmp.sa_handler = SIG_DFL;
  sigemptyset(&act_tmp.sa_mask);
  act_tmp.sa_flags = 0;

  sigaction(SIGCHLD, &act_tmp, &act_old);
  defer {
    sigaction(SIGCHLD, &act_old, nullptr);
  };
#endif

  FILE *pipe = popen(command.c_str(), "r");
  if (pipe == nullptr) {
    return {-1, "popen() open failed!"};
  }
  try {
    std::size_t bytesRead;
    while ((bytesRead = std::fread(buffer.data(), sizeof(buffer.at(0)), sizeof(buffer), pipe)) != 0) {
      result += std::string(buffer.data(), bytesRead);
    }
  } catch (...) {
    pclose(pipe);
    return {-1, "popen() exec failed!"};
  }
  int ret = pclose(pipe);
  if (ret == -1) {
    return Result{-1, strerror(errno)};
  } else {
    if (WIFEXITED(ret)) {  // normal exit()
      int exitcode = WEXITSTATUS(ret);
      return Result{exitcode, result};
    } else {  // other error
      return Result{-2, result};
    }
  }
}

}  // namespace Shell
