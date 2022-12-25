#include "cmake.hpp"
#include "compile_commands.hpp"
#include "config.hpp"
#include "dialog.hpp"
#include "filesystem.hpp"
#include "json.hpp"
#include "terminal.hpp"
#include "utility.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <fstream>
#include <future>
#include <regex>

CMake::CMake(const boost::filesystem::path &path) {
  const auto find_cmake_project = [](const boost::filesystem::path &file_path) {
    std::ifstream input(file_path.string(), std::ios::binary);
    if(input) {
      std::string line;
      while(std::getline(input, line)) {
        const static std::regex project_regex("^ *project *\\(.*$", std::regex::icase | std::regex::optimize);
        std::smatch sm;
        if(std::regex_match(line, sm, project_regex))
          return true;
      }
    }
    return false;
  };

  boost::system::error_code ec;
  auto search_path = boost::filesystem::is_directory(path, ec) ? path : path.parent_path();
  while(true) {
    auto search_cmake_path = search_path / "CMakeLists.txt";
    if(boost::filesystem::exists(search_cmake_path, ec)) {
      paths.emplace(paths.begin(), search_cmake_path);
      if(find_cmake_project(search_cmake_path)) {
        project_path = search_path;
        break;
      }
    }
    if(search_path == search_path.root_directory())
      break;
    search_path = search_path.parent_path();
  }
}

bool CMake::update_default_build(const boost::filesystem::path &default_build_path, bool force) {
  boost::system::error_code ec;
  if(project_path.empty() || !boost::filesystem::exists(project_path / "CMakeLists.txt", ec) || default_build_path.empty())
    return false;

  if(!boost::filesystem::exists(default_build_path, ec)) {
    boost::system::error_code ec;
    boost::filesystem::create_directories(default_build_path, ec);
    if(ec) {
      Terminal::get().print("\e[31mError\e[m: could not create " + filesystem::get_short_path(default_build_path).string() + ": " + ec.message() + "\n", true);
      return false;
    }
  }

  if(!create_file_api_query(default_build_path) && !force && boost::filesystem::exists(default_build_path / "compile_commands.json", ec))
    return true;

  auto compile_commands_path = default_build_path / "compile_commands.json";
  bool canceled = false;
  Dialog::Message message("Creating/updating default build", [&canceled] {
    canceled = true;
  });
  boost::optional<int> exit_status;
  auto process = Terminal::get().async_process(Config::get().project.cmake.command + ' ' + filesystem::escape_argument(project_path.string()) + " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
                                               default_build_path,
                                               [&exit_status](int exit_status_) {
                                                 exit_status = exit_status_;
                                               });
  bool killed = false;
  while(!exit_status) {
    if(canceled && !killed) {
      process->kill();
      killed = true;
    }
    while(Gtk::Main::events_pending())
      Gtk::Main::iteration();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  message.hide();
  if(exit_status == 0) {
#ifdef _WIN32 // Temporary fix to MSYS2's libclang
    auto compile_commands_file = filesystem::read(compile_commands_path);
    auto replace_drive = [&compile_commands_file](const std::string &param) {
      size_t pos = 0;
      auto param_size = param.length();
      while((pos = compile_commands_file.find(param + "/", pos)) != std::string::npos) {
        if(pos + param_size + 1 < compile_commands_file.size())
          compile_commands_file.replace(pos, param_size + 2, param + compile_commands_file[pos + param_size + 1] + ":");
        else
          break;
      }
    };
    replace_drive("-I");
    replace_drive("-isystem ");
    filesystem::write(compile_commands_path, compile_commands_file);
#endif
    return true;
  }
  return false;
}

bool CMake::update_debug_build(const boost::filesystem::path &debug_build_path, bool force) {
  boost::system::error_code ec;
  if(project_path.empty() || !boost::filesystem::exists(project_path / "CMakeLists.txt", ec) || debug_build_path.empty())
    return false;

  if(!boost::filesystem::exists(debug_build_path, ec)) {
    boost::system::error_code ec;
    boost::filesystem::create_directories(debug_build_path, ec);
    if(ec) {
      Terminal::get().print("\e[31mError\e[m: could not create " + filesystem::get_short_path(debug_build_path).string() + ": " + ec.message() + "\n", true);
      return false;
    }
  }

  if(!force && boost::filesystem::exists(debug_build_path / "CMakeCache.txt", ec))
    return true;

  bool canceled = false;
  Dialog::Message message("Creating/updating debug build", [&canceled] {
    canceled = true;
  });
  boost::optional<int> exit_status;
  auto process = Terminal::get().async_process(Config::get().project.cmake.command + ' ' + filesystem::escape_argument(project_path.string()) + " -DCMAKE_BUILD_TYPE=Debug",
                                               debug_build_path,
                                               [&exit_status](int exit_status_) {
                                                 exit_status = exit_status_;
                                               });
  bool killed = false;
  while(!exit_status) {
    if(canceled && !killed) {
      process->kill();
      killed = true;
    }
    while(Gtk::Main::events_pending())
      Gtk::Main::iteration();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  message.hide();
  return exit_status == 0;
}

boost::filesystem::path CMake::get_executable(const boost::filesystem::path &build_path, const boost::filesystem::path &file_path) {

  // Prefer the CMake file API (if available) which gives exact information about the targets a file belongs to
  if(auto executable = get_executable_from_file_api(build_path, file_path))
    return *executable;

  // CMake does not store in compile_commands.json if an object is part of an executable or not.
  // Therefore, executables are first attempted found in the cmake files. These executables
  // are then used to identify if a file in compile_commands.json is part of an executable or not

  CompileCommands compile_commands(build_path);
  std::vector<std::pair<boost::filesystem::path, boost::filesystem::path>> source_files_and_maybe_executables;
  for(auto &command : compile_commands.commands) {
    auto source_file = filesystem::get_normal_path(command.file);
    auto values = command.parameter_values("-o");
    if(!values.empty()) {
      size_t pos;
      if((pos = values[0].find("CMakeFiles/")) != std::string::npos)
        values[0].erase(pos, 11);
      if((pos = values[0].find(".dir")) != std::string::npos) {
        auto executable = command.directory / values[0].substr(0, pos);
        source_files_and_maybe_executables.emplace_back(source_file, executable);
      }
    }
  }

  std::vector<boost::filesystem::path> cmake_executables;

  // Parse cmake files
  std::map<std::string, std::list<std::string>> variables;
  for(auto &path : paths) {
    parse_file(filesystem::read(path), variables, [this, &build_path, &cmake_executables, &path](Function function) {
      if(function.name == "add_executable") {
        if(!function.parameters.empty() && !function.parameters.front().empty()) {
          auto executable = (path.parent_path() / function.parameters.front()).string();
          auto project_path_str = project_path.string();
          size_t pos = executable.find(project_path_str);
          if(pos != std::string::npos)
            executable.replace(pos, project_path_str.size(), build_path.string());
          cmake_executables.emplace_back(executable);
        }
      }
    });
  }

  ssize_t best_match_size = -1;
  boost::filesystem::path best_match_executable;

  for(auto &cmake_executable : cmake_executables) {
    for(auto &source_file_and_maybe_executable : source_files_and_maybe_executables) {
      auto &source_file = source_file_and_maybe_executable.first;
      auto &maybe_executable = source_file_and_maybe_executable.second;
      if(cmake_executable == maybe_executable) {
        if(source_file == file_path)
          return maybe_executable;
        auto source_file_directory = source_file.parent_path();
        if(filesystem::file_in_path(file_path, source_file_directory)) {
          auto size = std::distance(source_file_directory.begin(), source_file_directory.end());
          if(size > best_match_size) {
            best_match_size = size;
            best_match_executable = maybe_executable;
          }
        }
      }
    }
  }
  if(!best_match_executable.empty())
    return best_match_executable;

  for(auto &source_file_and_maybe_executable : source_files_and_maybe_executables) {
    auto &source_file = source_file_and_maybe_executable.first;
    auto &maybe_executable = source_file_and_maybe_executable.second;
    if(source_file == file_path)
      return maybe_executable;
    auto source_file_directory = source_file.parent_path();
    if(filesystem::file_in_path(file_path, source_file_directory)) {
      auto size = std::distance(source_file_directory.begin(), source_file_directory.end());
      if(size > best_match_size) {
        best_match_size = size;
        best_match_executable = maybe_executable;
      }
    }
  }
  return best_match_executable;
}

void CMake::parse_file(const std::string &src, std::map<std::string, std::list<std::string>> &variables, std::function<void(Function &&)> &&on_function) {
  size_t i = 0;

  auto parse_comment = [&] {
    if(src[i] == '#') {
      while(i < src.size() && src[i] != '\n')
        ++i;
      return true;
    }
    return false;
  };

  auto is_whitespace = [&] {
    return src[i] == ' ' || src[i] == '\t' || src[i] == '\r' || src[i] == '\n';
  };

  auto forward_passed_whitespace = [&] {
    while(i < src.size() && is_whitespace())
      ++i;
    return i < src.size();
  };

  auto parse_variable_name = [&]() -> boost::optional<std::string> {
    if(src[i] == '$' && i + 1 < src.size() && src[i + 1] == '{') {
      auto start = i + 2;
      auto end = src.find('}', start);
      if(end != std::string::npos) {
        i = end;
        auto variable_name = src.substr(start, end - start);
        boost::algorithm::to_upper(variable_name);
        return variable_name;
      }
    }
    return {};
  };

  auto parse_function = [&]() -> boost::optional<Function> {
    Function function;
    if((src[i] >= 'A' && src[i] <= 'Z') || (src[i] >= 'a' && src[i] <= 'z') || src[i] == '_') {
      function.name += src[i++];
      while(i < src.size() && ((src[i] >= 'A' && src[i] <= 'Z') || (src[i] >= 'a' && src[i] <= 'z') || (src[i] >= '0' && src[i] <= '9') || src[i] == '_'))
        function.name += src[i++];
      if(forward_passed_whitespace() && src[i] == '(') {
        ++i;
        // Parse parameters
        for(; forward_passed_whitespace(); ++i) {
          if(src[i] == ')')
            return function;
          else if(src[i] == '"') { // Parse parameter within ""
            std::string parameter;
            ++i;
            for(; i < src.size(); ++i) {
              if(src[i] == '\\' && i + 1 < src.size())
                parameter += src[++i];
              else if(src[i] == '"')
                break;
              else if(auto variable_name = parse_variable_name()) {
                auto it = variables.find(*variable_name);
                if(it != variables.end()) {
                  bool first = true;
                  for(auto &value : it->second) {
                    parameter += (first ? "" : ";") + value;
                    first = false;
                  }
                }
              }
              else
                parameter += src[i];
            }
            function.parameters.emplace_back(std::move(parameter));
          }
          else { // Parse parameter not within ""
            auto parameter_it = function.parameters.end();
            for(; i < src.size() && !is_whitespace() && src[i] != ')'; ++i) {
              if(parameter_it == function.parameters.end())
                parameter_it = function.parameters.emplace(parameter_it);
              if(src[i] == '\\' && i + 1 < src.size())
                *parameter_it += src[++i];
              else if(auto variable_name = parse_variable_name()) {
                auto variable_it = variables.find(*variable_name);
                if(variable_it != variables.end()) {
                  if(variable_it->second.size() == 1)
                    *parameter_it += variable_it->second.front();
                  else if(variable_it->second.size() > 1) {
                    *parameter_it += variable_it->second.front();
                    function.parameters.insert(function.parameters.end(), std::next(variable_it->second.begin()), variable_it->second.end());
                    parameter_it = std::prev(function.parameters.end());
                  }
                }
              }
              else
                *parameter_it += src[i];
            }
            if(src[i] == ')')
              return function;
          }
        }
      }
    }
    return {};
  };

  for(; forward_passed_whitespace(); ++i) {
    if(parse_comment())
      continue;
    if(auto function = parse_function()) {
      boost::algorithm::to_lower(function->name);
      if(function->name == "set" && !function->parameters.empty() && !function->parameters.front().empty()) {
        auto variable_name = std::move(function->parameters.front());
        boost::algorithm::to_upper(variable_name);
        function->parameters.erase(function->parameters.begin());
        variables.emplace(std::move(variable_name), std::move(function->parameters));
      }
      else if(function->name == "project") {
        if(!function->parameters.empty()) {
          variables.emplace("CMAKE_PROJECT_NAME", function->parameters);
          variables.emplace("PROJECT_NAME", function->parameters);
        }
      }
      if(on_function)
        on_function(std::move(*function));
    }
  }
}

bool CMake::create_file_api_query(const boost::filesystem::path &build_path) {
  auto query_directory = build_path / ".cmake" / "api" / "v1" / "query" / "client-jucipp";
  auto query_file = query_directory / "query.json";

  boost::system::error_code ec;
  if(boost::filesystem::exists(query_file, ec))
    return false;

  boost::filesystem::create_directories(query_directory, ec);
  if(ec) {
    Terminal::get().print("\e[31mError\e[m: could not create cmake file api query directory " + filesystem::get_short_path(query_directory).string() + ": " + ec.message() + "\n", true);
    return false;
  }

  if(!filesystem::write(query_file, R"({
  "requests": [
    { "kind": "codemodel" , "version": 2 }
  ]
})")) {
    Terminal::get().print("\e[31mError\e[m: could not create cmake file api query file " + filesystem::get_short_path(query_file).string() + ": " + ec.message() + "\n", true);
    return false;
  }
  return true;
}

boost::optional<boost::filesystem::path> CMake::get_executable_from_file_api(const boost::filesystem::path &build_path, const boost::filesystem::path &file_path) {
  auto reply_directory = build_path / ".cmake" / "api" / "v1" / "reply";

  boost::system::error_code ec;
  if(!boost::filesystem::is_directory(reply_directory, ec)) {
    // If the reply directory does not exist, either CMake was not run or the file API is not yet supported
    return {};
  }

  // Check all target-*.json files and filter for the given file path
  std::vector<boost::filesystem::path> target_files;
  for(const auto &reply_file : boost::filesystem::directory_iterator(reply_directory, ec)) {
    auto reply_file_path = reply_file.path();
    if(starts_with(reply_file_path.stem().string(), "target-"))
      target_files.emplace_back(reply_file_path);
  }
  std::sort(target_files.begin(), target_files.end(), [](const boost::filesystem::path &path1, const boost::filesystem::path &path2) {
    return Natural::compare(path1.string(), path2.string()) < 0;
  });

  boost::optional<boost::filesystem::path> executable;
  ssize_t best_match_size = -1;
  for(const auto &target_file : target_files) {
    try {
      JSON json(target_file);

      if(json.string("type") != "EXECUTABLE")
        continue;

      auto artifacts = json.array("artifacts");
      if(artifacts.empty())
        continue;

      auto relative_path = artifacts.front().string("path");

      for(auto &source : json.array("sources")) {
        auto source_file = project_path / source.string("path");
        if(source_file == file_path)
          return {build_path / relative_path};

        auto source_directory = source_file.parent_path();
        if(filesystem::file_in_path(file_path, source_directory)) {
          auto size = std::distance(source_directory.begin(), source_directory.end());
          if(size > best_match_size) {
            best_match_size = size;
            executable = build_path / relative_path;
          }
        }
      }
    }
    catch(...) {
    }
  }

  return executable;
}
