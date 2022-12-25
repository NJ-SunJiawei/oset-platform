#pragma once
#include <string>
#include <vector>

namespace Documentation {
  class CppReference {
  public:
    static std::vector<std::string> get_headers(const std::string &symbol) noexcept;
    static std::string get_url(const std::string &symbol) noexcept;
  };
} // namespace Documentation
