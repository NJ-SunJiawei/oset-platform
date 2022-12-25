#pragma once

#include "nlohmann/json_fwd.hpp"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <ostream>

class JSON {
  nlohmann::ordered_json *ptr;
  bool owner;

public:
  static std::string escape_string(std::string string);

  enum class ParseOptions { none = 0,
                            accept_string };

  enum class StructureType { object = 0,
                             array };

  /// Create an empty structure (null) or array
  JSON(StructureType type = StructureType::object)
  noexcept;

  explicit JSON(nlohmann::ordered_json *json_ptr) noexcept : ptr(json_ptr), owner(false) {}
  explicit JSON(const std::string &string);
  explicit JSON(const char *c_str);
  explicit JSON(std::istream &istream);
  explicit JSON(const boost::filesystem::path &path);

  JSON(JSON &&other)
  noexcept;
  JSON &operator=(JSON &&other) noexcept;

  ~JSON();

  static JSON make_owner(JSON &&other) noexcept;

  /// Use for instance std::setw(2) prior to this call to enable pretty printing.
  friend std::ostream &operator<<(std::ostream &os, const JSON &json);

  /// Set indent to for instance 2 to enable pretty printing.
  std::string to_string(int indent = -1) const;

  /// Set indent to for instance 2 to enable pretty printing.
  void to_file(const boost::filesystem::path &path, int indent = -1) const;

  void set(const std::string &key, std::string value) noexcept;
  void set(const std::string &key, const char *value) noexcept;
  void set(const std::string &key, int value) noexcept {
    set(key, static_cast<long long>(value));
  }
  void set(const std::string &key, long value) noexcept {
    set(key, static_cast<long long>(value));
  }
  void set(const std::string &key, long long value) noexcept;
  void set(const std::string &key, unsigned value) noexcept {
    set(key, static_cast<long long>(value));
  }
  void set(const std::string &key, unsigned long value) noexcept {
    set(key, static_cast<long long>(value));
  }
  void set(const std::string &key, unsigned long long value) noexcept {
    set(key, static_cast<long long>(value));
  }
  void set(const std::string &key, float value) noexcept {
    set(key, static_cast<double>(value));
  }
  void set(const std::string &key, double value) noexcept;
  void set(const std::string &key, bool value) noexcept;
  void set(const std::string &key, JSON &&value) noexcept;
  void set(const std::string &key, const JSON &value) noexcept;

  /// Might invalidate JSON references returned by children(), if one of the these elements are removed.
  void remove(const std::string &key) noexcept;

  void emplace_back(JSON &&value);

  boost::optional<JSON> child_optional(const std::string &key) const noexcept;
  JSON child(const std::string &key) const;

  boost::optional<std::vector<std::pair<std::string, JSON>>> children_optional(const std::string &key) const noexcept;
  std::vector<std::pair<std::string, JSON>> children_or_empty(const std::string &key) const noexcept;
  std::vector<std::pair<std::string, JSON>> children(const std::string &key) const;
  boost::optional<std::vector<std::pair<std::string, JSON>>> children_optional() const noexcept;
  std::vector<std::pair<std::string, JSON>> children_or_empty() const noexcept;
  std::vector<std::pair<std::string, JSON>> children() const;

  boost::optional<JSON> object_optional(const std::string &key) const noexcept;
  JSON object(const std::string &key) const;
  boost::optional<JSON> object_optional() const noexcept;
  JSON object() const;

  boost::optional<std::vector<JSON>> array_optional(const std::string &key) const noexcept;
  std::vector<JSON> array_or_empty(const std::string &key) const noexcept;
  std::vector<JSON> array(const std::string &key) const;
  boost::optional<std::vector<JSON>> array_optional() const noexcept;
  std::vector<JSON> array_or_empty() const noexcept;
  std::vector<JSON> array() const;

  boost::optional<std::string> string_optional(const std::string &key) const noexcept;
  std::string string_or(const std::string &key, const std::string &default_value) const noexcept;
  std::string string(const std::string &key) const;
  boost::optional<std::string> string_optional() const noexcept;
  std::string string_or(const std::string &default_value) const noexcept;
  std::string string() const;

  boost::optional<long long> integer_optional(const std::string &key, ParseOptions parse_options = ParseOptions::none) const noexcept;
  long long integer_or(const std::string &key, long long default_value, ParseOptions parse_options = ParseOptions::none) const noexcept;
  long long integer(const std::string &key, ParseOptions parse_options = ParseOptions::none) const;
  boost::optional<long long> integer_optional(ParseOptions parse_options = ParseOptions::none) const noexcept;
  long long integer_or(long long default_value, ParseOptions parse_options = ParseOptions::none) const noexcept;
  long long integer(ParseOptions parse_options = ParseOptions::none) const;

  boost::optional<double> floating_point_optional(const std::string &key, ParseOptions parse_options = ParseOptions::none) const noexcept;
  double floating_point_or(const std::string &key, double default_value, ParseOptions parse_options = ParseOptions::none) const noexcept;
  double floating_point(const std::string &key, ParseOptions parse_options = ParseOptions::none) const;
  boost::optional<double> floating_point_optional(ParseOptions parse_options = ParseOptions::none) const noexcept;
  double floating_point_or(double default_value, ParseOptions parse_options = ParseOptions::none) const noexcept;
  double floating_point(ParseOptions parse_options = ParseOptions::none) const;

  boost::optional<bool> boolean_optional(const std::string &key, ParseOptions parse_options = ParseOptions::none) const noexcept;
  bool boolean_or(const std::string &key, bool default_value, ParseOptions parse_options = ParseOptions::none) const noexcept;
  bool boolean(const std::string &key, ParseOptions parse_options = ParseOptions::none) const;
  boost::optional<bool> boolean_optional(ParseOptions parse_options = ParseOptions::none) const noexcept;
  bool boolean_or(bool default_value, ParseOptions parse_options = ParseOptions::none) const noexcept;
  bool boolean(ParseOptions parse_options = ParseOptions::none) const;
};
