#include "nlohmann/json.hpp"
#include "json.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

std::string JSON::escape_string(std::string string) {
  for(size_t c = 0; c < string.size(); ++c) {
    if(string[c] == '\b') {
      string.replace(c, 1, "\\b");
      ++c;
    }
    else if(string[c] == '\f') {
      string.replace(c, 1, "\\f");
      ++c;
    }
    else if(string[c] == '\n') {
      string.replace(c, 1, "\\n");
      ++c;
    }
    else if(string[c] == '\r') {
      string.replace(c, 1, "\\r");
      ++c;
    }
    else if(string[c] == '\t') {
      string.replace(c, 1, "\\t");
      ++c;
    }
    else if(string[c] == '"') {
      string.replace(c, 1, "\\\"");
      ++c;
    }
    else if(string[c] == '\\') {
      string.replace(c, 1, "\\\\");
      ++c;
    }
  }
  return string;
}

JSON::JSON(StructureType type) noexcept : ptr(type == StructureType::object ? new nlohmann::ordered_json() : new nlohmann::ordered_json(nlohmann::ordered_json::array())), owner(true) {}

JSON::JSON(const std::string &string) : ptr(new nlohmann::ordered_json(nlohmann::ordered_json::parse(string))), owner(true) {}

JSON::JSON(const char *c_str) : ptr(new nlohmann::ordered_json(nlohmann::ordered_json::parse(c_str))), owner(true) {}

JSON::JSON(std::istream &istream) : ptr(new nlohmann::ordered_json(nlohmann::ordered_json::parse(istream))), owner(true) {}

JSON::JSON(const boost::filesystem::path &path) {
  std::ifstream input(path.string(), std::ios::binary);
  if(!input)
    throw std::runtime_error("could not open file " + path.string());
  ptr = new nlohmann::ordered_json(nlohmann::ordered_json::parse(input));
  owner = true;
}

JSON::JSON(JSON &&other) noexcept : ptr(other.ptr), owner(other.owner) {
  other.owner = false;
}

JSON &JSON::operator=(JSON &&other) noexcept {
  if(owner)
    delete ptr;
  ptr = other.ptr;
  owner = other.owner;
  other.owner = false;
  return *this;
}

JSON::~JSON() {
  if(owner)
    delete ptr;
}

JSON JSON::make_owner(JSON &&other) noexcept {
  auto owner = JSON(new nlohmann::ordered_json(std::move(*other.ptr)));
  owner.owner = true;
  other.owner = false;
  return owner;
}

std::ostream &operator<<(std::ostream &os, const JSON &json) {
  return os << *json.ptr;
}

std::string JSON::to_string(int indent) const {
  return ptr->dump(indent);
}

void JSON::to_file(const boost::filesystem::path &path, int indent) const {
  std::ofstream file(path.string(), std::ios::binary);
  if(!file)
    throw std::runtime_error("could not open file " + path.string());
  if(indent != -1)
    file << std::setw(indent);
  file << *ptr << '\n';
}

void JSON::set(const std::string &key, std::string value) noexcept {
  (*ptr)[key] = std::move(value);
}

void JSON::set(const std::string &key, const char *value) noexcept {
  (*ptr)[key] = value;
}

void JSON::set(const std::string &key, long long value) noexcept {
  (*ptr)[key] = value;
}

void JSON::set(const std::string &key, double value) noexcept {
  (*ptr)[key] = value;
}

void JSON::set(const std::string &key, bool value) noexcept {
  (*ptr)[key] = value;
}

void JSON::set(const std::string &key, JSON &&value) noexcept {
  (*ptr)[key] = std::move(*value.ptr);
  value.owner = false;
}

void JSON::set(const std::string &key, const JSON &value) noexcept {
  (*ptr)[key] = *value.ptr;
}

void JSON::remove(const std::string &key) noexcept {
  ptr->erase(key);
}

void JSON::emplace_back(JSON &&value) {
  ptr->emplace_back(std::move(*value.ptr));
  value.owner = false;
}

boost::optional<JSON> JSON::child_optional(const std::string &key) const noexcept {
  try {
    return child(key);
  }
  catch(...) {
    return {};
  }
}

JSON JSON::child(const std::string &key) const {
  return JSON(&ptr->at(key));
}

boost::optional<std::vector<std::pair<std::string, JSON>>> JSON::children_optional(const std::string &key) const noexcept {
  try {
    return children(key);
  }
  catch(...) {
    return {};
  }
}

std::vector<std::pair<std::string, JSON>> JSON::children_or_empty(const std::string &key) const noexcept {
  try {
    return children(key);
  }
  catch(...) {
    return {};
  }
}

std::vector<std::pair<std::string, JSON>> JSON::children(const std::string &key) const {
  return JSON(&ptr->at(key)).children();
}

boost::optional<std::vector<std::pair<std::string, JSON>>> JSON::children_optional() const noexcept {
  try {
    return children();
  }
  catch(...) {
    return {};
  }
}

std::vector<std::pair<std::string, JSON>> JSON::children_or_empty() const noexcept {
  try {
    return children();
  }
  catch(...) {
    return {};
  }
}

std::vector<std::pair<std::string, JSON>> JSON::children() const {
  if(!ptr->is_object())
    throw std::runtime_error("value '" + to_string() + "' is not an object");
  std::vector<std::pair<std::string, JSON>> result;
  for(auto it = ptr->begin(); it != ptr->end(); ++it)
    result.emplace_back(it.key(), JSON(&*it));
  return result;
}

boost::optional<JSON> JSON::object_optional(const std::string &key) const noexcept {
  try {
    return object(key);
  }
  catch(...) {
    return {};
  }
}

JSON JSON::object(const std::string &key) const {
  return JSON(&ptr->at(key)).object();
}


boost::optional<JSON> JSON::object_optional() const noexcept {
  try {
    return object();
  }
  catch(...) {
    return {};
  }
}

JSON JSON::object() const {
  if(!ptr->is_object())
    throw std::runtime_error("value '" + to_string() + "' is not an object");
  return JSON(ptr);
}

boost::optional<std::vector<JSON>> JSON::array_optional(const std::string &key) const noexcept {
  try {
    return array(key);
  }
  catch(...) {
    return {};
  }
}

std::vector<JSON> JSON::array_or_empty(const std::string &key) const noexcept {
  try {
    return array(key);
  }
  catch(...) {
    return {};
  }
}

std::vector<JSON> JSON::array(const std::string &key) const {
  return JSON(&ptr->at(key)).array();
}

boost::optional<std::vector<JSON>> JSON::array_optional() const noexcept {
  try {
    return array();
  }
  catch(...) {
    return {};
  }
}

std::vector<JSON> JSON::array_or_empty() const noexcept {
  try {
    return array();
  }
  catch(...) {
    return {};
  }
}

std::vector<JSON> JSON::array() const {
  if(!ptr->is_array())
    throw std::runtime_error("value '" + to_string() + "' is not an array");
  std::vector<JSON> result;
  result.reserve(ptr->size());
  for(auto &e : *ptr)
    result.emplace_back(&e);
  return result;
}

boost::optional<std::string> JSON::string_optional(const std::string &key) const noexcept {
  try {
    return string(key);
  }
  catch(...) {
    return {};
  }
}

std::string JSON::string_or(const std::string &key, const std::string &default_value) const noexcept {
  try {
    return string(key);
  }
  catch(...) {
    return default_value;
  }
}

std::string JSON::string(const std::string &key) const {
  return ptr->at(key).get<std::string>();
}

boost::optional<std::string> JSON::string_optional() const noexcept {
  try {
    return string();
  }
  catch(...) {
    return {};
  }
}

std::string JSON::string_or(const std::string &default_value) const noexcept {
  try {
    return string();
  }
  catch(...) {
    return default_value;
  }
}

std::string JSON::string() const {
  return ptr->get<std::string>();
}

boost::optional<long long> JSON::integer_optional(const std::string &key, ParseOptions parse_options) const noexcept {
  try {
    return integer(key, parse_options);
  }
  catch(...) {
    return {};
  }
}

long long JSON::integer_or(const std::string &key, long long default_value, ParseOptions parse_options) const noexcept {
  try {
    return integer(key, parse_options);
  }
  catch(...) {
    return default_value;
  }
}

long long JSON::integer(const std::string &key, ParseOptions parse_options) const {
  return JSON(&ptr->at(key)).integer(parse_options);
}

boost::optional<long long> JSON::integer_optional(ParseOptions parse_options) const noexcept {
  try {
    return integer(parse_options);
  }
  catch(...) {
    return {};
  }
}

long long JSON::integer_or(long long default_value, ParseOptions parse_options) const noexcept {
  try {
    return integer(parse_options);
  }
  catch(...) {
    return default_value;
  }
}

long long JSON::integer(ParseOptions parse_options) const {
  if(auto integer = ptr->get<nlohmann::ordered_json::number_integer_t *>())
    return *integer;
  if(auto floating_point = ptr->get<nlohmann::ordered_json::number_float_t *>())
    return *floating_point;
  if(parse_options == ParseOptions::accept_string) {
    if(auto string = ptr->get<nlohmann::ordered_json::string_t *>()) {
      try {
        return std::stoll(*string);
      }
      catch(...) {
      }
    }
  }
  throw std::runtime_error("value '" + to_string() + "' could not be converted to integer");
}

boost::optional<double> JSON::floating_point_optional(const std::string &key, ParseOptions parse_options) const noexcept {
  try {
    return floating_point(key, parse_options);
  }
  catch(...) {
    return {};
  }
}

double JSON::floating_point_or(const std::string &key, double default_value, ParseOptions parse_options) const noexcept {
  try {
    return floating_point(key, parse_options);
  }
  catch(...) {
    return default_value;
  }
}

double JSON::floating_point(const std::string &key, ParseOptions parse_options) const {
  return JSON(&ptr->at(key)).floating_point(parse_options);
}

boost::optional<double> JSON::floating_point_optional(ParseOptions parse_options) const noexcept {
  try {
    return floating_point(parse_options);
  }
  catch(...) {
    return {};
  }
}

double JSON::floating_point_or(double default_value, ParseOptions parse_options) const noexcept {
  try {
    return floating_point(parse_options);
  }
  catch(...) {
    return default_value;
  }
}

double JSON::floating_point(ParseOptions parse_options) const {
  if(auto floating_point = ptr->get<nlohmann::ordered_json::number_float_t *>())
    return *floating_point;
  if(auto integer = ptr->get<nlohmann::ordered_json::number_integer_t *>())
    return *integer;
  if(parse_options == ParseOptions::accept_string) {
    if(auto string = ptr->get<nlohmann::ordered_json::string_t *>()) {
      try {
        return std::stof(*string);
      }
      catch(...) {
      }
    }
  }
  throw std::runtime_error("value '" + to_string() + "' could not be converted to floating point");
}

boost::optional<bool> JSON::boolean_optional(const std::string &key, ParseOptions parse_options) const noexcept {
  try {
    return boolean(key, parse_options);
  }
  catch(...) {
    return {};
  }
}

bool JSON::boolean_or(const std::string &key, bool default_value, ParseOptions parse_options) const noexcept {
  try {
    return boolean(key, parse_options);
  }
  catch(...) {
    return default_value;
  }
}

bool JSON::boolean(const std::string &key, ParseOptions parse_options) const {
  return JSON(&ptr->at(key)).boolean(parse_options);
}

boost::optional<bool> JSON::boolean_optional(ParseOptions parse_options) const noexcept {
  try {
    return boolean(parse_options);
  }
  catch(...) {
    return {};
  }
}

bool JSON::boolean_or(bool default_value, ParseOptions parse_options) const noexcept {

  try {
    return boolean(parse_options);
  }
  catch(...) {
    return default_value;
  }
}

bool JSON::boolean(ParseOptions parse_options) const {
  if(auto boolean = ptr->get<nlohmann::ordered_json::boolean_t *>())
    return *boolean;
  if(auto integer = ptr->get<nlohmann::ordered_json::number_integer_t *>()) {
    if(*integer == 1)
      return true;
    if(*integer == 0)
      return false;
  }
  if(parse_options == ParseOptions::accept_string) {
    if(auto string = ptr->get<nlohmann::ordered_json::string_t *>()) {
      if(*string == "true" || *string == "1")
        return true;
      if(*string == "false" || *string == "0")
        return false;
    }
  }
  throw std::runtime_error("value '" + to_string() + "' could not be converted to bool");
}
