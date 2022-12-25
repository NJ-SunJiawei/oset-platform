#include "utility.hpp"
#include <algorithm>
#include <cstring>
#include <gtkmm.h>
#include <vector>

ScopeGuard::~ScopeGuard() {
  if(on_exit)
    on_exit();
}

size_t utf8_character_count(const std::string &text, size_t pos, size_t length) noexcept {
  size_t characters = 0;
  auto size = length == std::string::npos ? text.size() : std::min(pos + length, text.size());
  for(; pos < size;) {
    if(static_cast<unsigned char>(text[pos]) <= 0b01111111) {
      ++characters;
      ++pos;
    }
    else if(static_cast<unsigned char>(text[pos]) >= 0b11111000) // Invalid UTF-8 byte
      ++pos;
    else if(static_cast<unsigned char>(text[pos]) >= 0b11110000) {
      ++characters;
      pos += 4;
    }
    else if(static_cast<unsigned char>(text[pos]) >= 0b11100000) {
      ++characters;
      pos += 3;
    }
    else if(static_cast<unsigned char>(text[pos]) >= 0b11000000) {
      ++characters;
      pos += 2;
    }
    else // // Invalid start of UTF-8 character
      ++pos;
  }
  return characters;
}

size_t utf16_code_units_byte_count(const std::string &text, size_t code_units, size_t start_pos) {
  if(code_units == 0)
    return 0;

  size_t pos = start_pos;
  size_t current_code_units = 0;
  for(; pos < text.size();) {
    if(static_cast<unsigned char>(text[pos]) <= 0b01111111) {
      ++current_code_units;
      ++pos;
      if(current_code_units >= code_units)
        break;
    }
    else if(static_cast<unsigned char>(text[pos]) >= 0b11111000) // Invalid UTF-8 byte
      ++pos;
    else if(static_cast<unsigned char>(text[pos]) >= 0b11110000) {
      current_code_units += 2;
      pos += 4;
      if(current_code_units >= code_units)
        break;
    }
    else if(static_cast<unsigned char>(text[pos]) >= 0b11100000) {
      ++current_code_units;
      pos += 3;
      if(current_code_units >= code_units)
        break;
    }
    else if(static_cast<unsigned char>(text[pos]) >= 0b11000000) {
      ++current_code_units;
      pos += 2;
      if(current_code_units >= code_units)
        break;
    }
    else // // Invalid start of UTF-8 character
      ++pos;
  }
  return pos - start_pos;
}

size_t utf16_code_unit_count(const std::string &text, size_t pos, size_t length) {
  size_t code_units = 0;
  auto size = length == std::string::npos ? text.size() : std::min(pos + length, text.size());
  for(; pos < size;) {
    if(static_cast<unsigned char>(text[pos]) <= 0b01111111) {
      ++code_units;
      ++pos;
    }
    else if(static_cast<unsigned char>(text[pos]) >= 0b11111000) // Invalid UTF-8 byte
      ++pos;
    else if(static_cast<unsigned char>(text[pos]) >= 0b11110000) {
      code_units += 2;
      pos += 4;
    }
    else if(static_cast<unsigned char>(text[pos]) >= 0b11100000) {
      ++code_units;
      pos += 3;
    }
    else if(static_cast<unsigned char>(text[pos]) >= 0b11000000) {
      ++code_units;
      pos += 2;
    }
    else // // Invalid start of UTF-8 character
      ++pos;
  }
  return code_units;
}

bool starts_with(const char *str, const std::string &test) noexcept {
  for(size_t i = 0; i < test.size(); ++i) {
    if(*str == '\0')
      return false;
    if(*str != test[i])
      return false;
    ++str;
  }
  return true;
}

bool starts_with(const char *str, const char *test) noexcept {
  for(; *test != '\0'; ++test) {
    if(*str == '\0')
      return false;
    if(*str != *test)
      return false;
    ++str;
  }
  return true;
}

bool starts_with(const std::string &str, const std::string &test) noexcept {
  return str.compare(0, test.size(), test) == 0;
}

bool starts_with(const std::string &str, const char *test) noexcept {
  for(size_t i = 0; i < str.size(); ++i) {
    if(*test == '\0')
      return true;
    if(str[i] != *test)
      return false;
    ++test;
  }
  return *test == '\0';
}

bool starts_with(const std::string &str, size_t pos, const std::string &test) noexcept {
  if(pos > str.size())
    return false;
  return str.compare(pos, test.size(), test) == 0;
}

bool starts_with(const std::string &str, size_t pos, const char *test) noexcept {
  if(pos > str.size())
    return false;
  for(size_t i = pos; i < str.size(); ++i) {
    if(*test == '\0')
      return true;
    if(str[i] != *test)
      return false;
    ++test;
  }
  return *test == '\0';
}

bool ends_with(const std::string &str, const std::string &test) noexcept {
  if(test.size() > str.size())
    return false;
  return str.compare(str.size() - test.size(), test.size(), test) == 0;
}

bool ends_with(const std::string &str, const char *test) noexcept {
  auto test_size = strlen(test);
  if(test_size > str.size())
    return false;
  return str.compare(str.size() - test_size, test_size, test) == 0;
}

std::string escape(const std::string &input, const std::set<char> &escape_chars) {
  std::string result;
  result.reserve(input.size());
  for(auto &chr : input) {
    if(escape_chars.find(chr) != escape_chars.end())
      result += '\\';
    result += chr;
  }
  return result;
}

std::string to_hex_string(const std::string &input) {
  std::string result;
  result.reserve(input.size() * 2);
  std::string hex_chars = "0123456789abcdef";
  for(auto &chr : input) {
    result += hex_chars[static_cast<unsigned char>(chr) >> 4];
    result += hex_chars[static_cast<unsigned char>(chr) & 0x0f];
  }
  return result;
}

int version_compare(const std::string &lhs, const std::string &rhs) {
  static auto get_parts = [](const std::string &str) {
    std::vector<int> result;
    std::string tmp;
    for(auto &chr : str) {
      if(chr >= '0' && chr <= '9')
        tmp += chr;
      else if(chr == '.') {
        if(!tmp.empty()) {
          try {
            result.emplace_back(std::stoi(tmp));
          }
          catch(...) {
          }
          tmp.clear();
        }
      }
      else
        tmp += std::to_string(static_cast<unsigned char>(chr)); // Convert for instance letters to numbers
    }
    if(!tmp.empty()) {
      try {
        result.emplace_back(std::stoi(tmp));
      }
      catch(...) {
      }
      tmp.clear();
    }
    return result;
  };
  auto lhs_parts = get_parts(lhs);
  auto rhs_parts = get_parts(rhs);
  if(std::equal(lhs_parts.begin(), lhs_parts.end(), rhs_parts.begin(), rhs_parts.end()))
    return 0;
  if(std::lexicographical_compare(lhs_parts.begin(), lhs_parts.end(), rhs_parts.begin(), rhs_parts.end()))
    return -1;
  return 1;
}

bool Natural::is_digit(char chr) {
  return chr >= '0' && chr <= '9';
}

int Natural::compare_characters(size_t &i1, size_t &i2, const std::string &s1, const std::string &s2) {
  ScopeGuard scope_guard{[&i1, &i2] {
    ++i1;
    ++i2;
  }};
  auto c1 = static_cast<unsigned char>(s1[i1]);
  auto c2 = static_cast<unsigned char>(s2[i2]);
  if(c1 < 0b10000000 && c2 < 0b10000000) { // Both characters are ascii
    auto at = std::tolower(s1[i1]);
    auto bt = std::tolower(s2[i2]);
    if(at < bt)
      return -1;
    else if(at == bt)
      return 0;
    else
      return 1;
  }

  Glib::ustring u1;
  if(c1 >= 0b11110000)
    u1 = s1.substr(i1, 4);
  else if(c1 >= 0b11100000)
    u1 = s1.substr(i1, 3);
  else if(c1 >= 0b11000000)
    u1 = s1.substr(i1, 2);
  else
    u1 = s1[i1];

  Glib::ustring u2;
  if(c2 >= 0b11110000)
    u2 = s2.substr(i2, 4);
  else if(c2 >= 0b11100000)
    u2 = s2.substr(i2, 3);
  else if(c2 >= 0b11000000)
    u2 = s2.substr(i2, 2);
  else
    u2 = s2[i2];

  i1 += u1.bytes() - 1;
  i2 += u2.bytes() - 1;

  u1 = u1.lowercase();
  u2 = u2.lowercase();

  if(u1 < u2)
    return -1;
  else if(u1 == u2)
    return 0;
  else
    return 1;
}

int Natural::compare_numbers(size_t &i1, size_t &i2, const std::string &s1, const std::string &s2) {
  int result = 0;
  while(true) {
    if(i1 >= s1.size() || !is_digit(s1[i1])) {
      if(i2 >= s2.size() || !is_digit(s2[i2])) // a and b has equal number of digits
        return result;
      return -1; // a has fewer digits
    }
    if(i2 >= s2.size() || !is_digit(s2[i2]))
      return 1; // b has fewer digits

    if(result == 0) {
      if(s1[i1] < s2[i2])
        result = -1;
      if(s1[i1] > s2[i2])
        result = 1;
    }
    ++i1;
    ++i2;
  }
}

int Natural::compare(const std::string &s1, const std::string &s2) {
  size_t i1 = 0;
  size_t i2 = 0;
  while(i1 < s1.size() && i2 < s2.size()) {
    if(is_digit(s1[i1]) && !is_digit(s2[i2]))
      return -1;
    if(!is_digit(s1[i1]) && is_digit(s2[i2]))
      return 1;
    if(!is_digit(s1[i1]) && !is_digit(s2[i2])) {
      auto result = compare_characters(i1, i2, s1, s2);
      if(result != 0)
        return result;
    }
    else {
      auto result = compare_numbers(i1, i2, s1, s2);
      if(result != 0)
        return result;
    }
  }

  if(i1 >= s1.size()) {
    if(i2 >= s2.size())
      return 0;
    return -1;
  }
  return 1;
}
