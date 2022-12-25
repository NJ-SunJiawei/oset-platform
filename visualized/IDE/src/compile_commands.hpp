#pragma once
#include <boost/filesystem.hpp>
#include <string>
#include <vector>

class CompileCommands {
public:
  class FindSystemIncludePaths {
    int exit_status;

  public:
    std::vector<std::string> include_paths;
    std::vector<std::string> framework_paths;
    operator bool() const { return exit_status == 0; }

    FindSystemIncludePaths();
  };

  class Command {
  public:
    boost::filesystem::path directory;
    std::vector<std::string> parameters;
    boost::filesystem::path file;

    std::vector<std::string> parameter_values(const std::string &parameter_name) const;
  };

  CompileCommands(const boost::filesystem::path &build_path);
  std::vector<Command> commands;

  /// Return arguments for the given file using libclangmm
  static std::vector<std::string> get_arguments(const boost::filesystem::path &build_path, const boost::filesystem::path &file_path);

  static bool is_header(const boost::filesystem::path &path);
  static bool is_source(const boost::filesystem::path &path);
};
