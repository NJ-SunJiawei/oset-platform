#pragma once
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <list>
#include <map>
#include <vector>

class CMake {
public:
  CMake(const boost::filesystem::path &path);
  boost::filesystem::path project_path;

  bool update_default_build(const boost::filesystem::path &default_build_path, bool force = false);
  bool update_debug_build(const boost::filesystem::path &debug_build_path, bool force = false);

  boost::filesystem::path get_executable(const boost::filesystem::path &build_path, const boost::filesystem::path &file_path);

private:
  std::vector<boost::filesystem::path> paths;

  struct Function {
    std::string name;
    std::list<std::string> parameters;
  };
  static void parse_file(const std::string &src, std::map<std::string, std::list<std::string>> &variables, std::function<void(Function &&)> &&on_function);

  // Cmake file API functions
  /// Returns true if file api query file was created, false otherwise including if the file already exists
  bool create_file_api_query(const boost::filesystem::path &build_path);
  boost::optional<boost::filesystem::path> get_executable_from_file_api(const boost::filesystem::path &build_path, const boost::filesystem::path &file_path);
};
