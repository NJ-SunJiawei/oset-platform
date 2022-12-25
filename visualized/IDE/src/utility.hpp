#pragma once
#include <functional>
#include <set>
#include <string>

class ScopeGuard {
public:
  std::function<void()> on_exit;
  ~ScopeGuard();
};

/// Returns number of utf8 characters in text
size_t utf8_character_count(const std::string &text, size_t pos = 0, size_t length = std::string::npos) noexcept;

/// Returns number of bytes in the given utf16 code units in text
size_t utf16_code_units_byte_count(const std::string &text, size_t code_units, size_t start_pos = 0);
/// Returns number of utf16 code units in the text
size_t utf16_code_unit_count(const std::string &text, size_t pos = 0, size_t length = std::string::npos);

bool starts_with(const char *str, const std::string &test) noexcept;
bool starts_with(const char *str, const char *test) noexcept;
bool starts_with(const std::string &str, const std::string &test) noexcept;
bool starts_with(const std::string &str, const char *test) noexcept;
/// Comparison starts from position pos in str
bool starts_with(const std::string &str, size_t pos, const std::string &test) noexcept;
/// Comparison starts from position pos in str
bool starts_with(const std::string &str, size_t pos, const char *test) noexcept;

bool ends_with(const std::string &str, const std::string &test) noexcept;
bool ends_with(const std::string &str, const char *test) noexcept;

std::string escape(const std::string &input, const std::set<char> &escape_chars);

std::string to_hex_string(const std::string &input);

/// Returns -1 if lhs is smaller than rhs, 0 if equal, and 1 if lhs is larger than rhs
int version_compare(const std::string &lhs, const std::string &rhs);

class Natural {
  static bool is_digit(char chr);

  static int compare_characters(size_t &i1, size_t &i2, const std::string &s1, const std::string &s2);

  static int compare_numbers(size_t &i1, size_t &i2, const std::string &s1, const std::string &s2);

public:
  /// Natural comparison supporting UTF-8 and locale
  static int compare(const std::string &s1, const std::string &s2);
};
