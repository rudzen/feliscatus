#pragma once

#include <memory>
#include <filesystem>
#include <fmt/format.h>

struct FileResolver final {

  explicit FileResolver(const std::string_view f) : file_(std::filesystem::path(f)) {}

  explicit FileResolver(const std::string_view f, const std::string_view post_name) {
    fmt::memory_buffer b;
    fmt::format_to(b, "{}{}", post_name, f);
    file_ = std::filesystem::path(fmt::to_string(b));
  }

  [[nodiscard]]
  bool exists() const;

  [[nodiscard]]
  bool is_file() const;

  [[nodiscard]]
  std::uintmax_t size() const;

  [[nodiscard]]
  std::filesystem::path file_name() const;

private:
  std::filesystem::path file_;
};

inline bool FileResolver::exists() const {
  return std::filesystem::exists(file_);
}

inline bool FileResolver::is_file() const {
  return std::filesystem::is_regular_file(file_);
}

inline std::uintmax_t FileResolver::size() const {
  if (exists() && is_file())
  {
    const auto filesize = std::filesystem::file_size(file_);
    if (filesize != static_cast<uintmax_t>(-1))
      return filesize;
  }

  return static_cast<uintmax_t>(-1);
}

inline std::filesystem::path FileResolver::file_name() const {
    return std::filesystem::absolute(file_);
}

namespace file_handler {

template<typename T, typename... Args>
std::unique_ptr<T> make_file_resolver(Args&&... args)
{
  static_assert(std::is_same_v<T, FileResolver>);
  return std::make_unique<T>(T(std::forward<Args>(args)...));
}

}
