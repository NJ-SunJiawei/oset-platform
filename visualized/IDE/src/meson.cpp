#include "meson.hpp"
#include "compile_commands.hpp"
#include "config.hpp"
#include "dialog.hpp"
#include "filesystem.hpp"
#include "json.hpp"
#include "terminal.hpp"
#include "utility.hpp"
#include <boost/optional.hpp>
#include <fstream>
#include <future>
#include <regex>

Meson::Meson(const boost::filesystem::path &path) {
  const auto find_project = [](const boost::filesystem::path &file_path) {
    std::ifstream input(file_path.string(), std::ios::binary);
    if(input) {
      std::string line;
      while(std::getline(input, line)) {
        const static std::regex project_regex("^ *project *\\(.*", std::regex::icase | std::regex::optimize);
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
    auto search_file = search_path / "meson.build";
    if(boost::filesystem::exists(search_file, ec)) {
      if(find_project(search_file)) {
        project_path = search_path;
        break;
      }
    }
    if(search_path == search_path.root_directory())
      break;
    search_path = search_path.parent_path();
  }
}

bool Meson::update_default_build(const boost::filesystem::path &default_build_path, bool force) {
  boost::system::error_code ec;
  if(project_path.empty() || !boost::filesystem::exists(project_path / "meson.build", ec) || default_build_path.empty())
    return false;

  if(!boost::filesystem::exists(default_build_path, ec)) {
    boost::system::error_code ec;
    boost::filesystem::create_directories(default_build_path, ec);
    if(ec) {
      Terminal::get().print("\e[31mError\e[m: could not create " + filesystem::get_short_path(default_build_path).string() + ": " + ec.message() + "\n", true);
      return false;
    }
  }

  auto compile_commands_path = default_build_path / "compile_commands.json";
  bool compile_commands_exists = boost::filesystem::exists(compile_commands_path, ec);
  if(!force && compile_commands_exists)
    return true;

  bool canceled = false;
  Dialog::Message message("Creating/updating default build", [&canceled] {
    canceled = true;
  });
  boost::optional<int> exit_status;
  auto process = Terminal::get().async_process(Config::get().project.meson.command + ' ' + (compile_commands_exists ? "--internal regenerate " : "") + "--buildtype plain " + filesystem::escape_argument(project_path.string()),
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
  return exit_status == 0;
}

bool Meson::update_debug_build(const boost::filesystem::path &debug_build_path, bool force) {
  boost::system::error_code ec;
  if(project_path.empty() || !boost::filesystem::exists(project_path / "meson.build", ec) || debug_build_path.empty())
    return false;

  if(!boost::filesystem::exists(debug_build_path, ec)) {
    boost::system::error_code ec;
    boost::filesystem::create_directories(debug_build_path, ec);
    if(ec) {
      Terminal::get().print("\e[31mError\e[m: could not create " + filesystem::get_short_path(debug_build_path).string() + ": " + ec.message() + "\n", true);
      return false;
    }
  }

  bool compile_commands_exists = boost::filesystem::exists(debug_build_path / "compile_commands.json", ec);
  if(!force && compile_commands_exists)
    return true;

  bool canceled = false;
  Dialog::Message message("Creating/updating debug build", [&canceled] {
    canceled = true;
  });
  boost::optional<int> exit_status;
  auto process = Terminal::get().async_process(Config::get().project.meson.command + ' ' + (compile_commands_exists ? "--internal regenerate " : "") + "--buildtype debug " + filesystem::escape_argument(project_path.string()),
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

boost::filesystem::path Meson::get_executable(const boost::filesystem::path &build_path, const boost::filesystem::path &file_path) {
  CompileCommands compile_commands(build_path);

  ssize_t best_match_size = -1;
  boost::filesystem::path best_match_executable;

  for(auto &command : compile_commands.commands) {
    auto source_file = filesystem::get_normal_path(command.file);
    auto values = command.parameter_values("-o");
    if(!values.empty()) {
      size_t pos;
      if((pos = values[0].find('@')) != std::string::npos) {
        if(starts_with(values[0], pos + 1, "exe")) {
          auto executable = build_path / values[0].substr(0, pos);
          if(source_file == file_path)
            return executable;
          auto source_file_directory = source_file.parent_path();
          if(filesystem::file_in_path(file_path, source_file_directory)) {
            auto size = std::distance(source_file_directory.begin(), source_file_directory.end());
            if(size > best_match_size) {
              best_match_size = size;
              best_match_executable = executable;
            }
          }
        }
      }
    }
  }

  if(best_match_executable.empty()) { // Newer Meson outputs intro-targets.json that can be used to find executable
    try {
      JSON targets(build_path / "meson-info" / "intro-targets.json");
      for(auto &target : targets.array()) {
        if(target.string("type") == "executable") {
          auto filenames = target.array("filename");
          if(filenames.empty()) // No executable file found
            break;
          auto executable = filesystem::get_normal_path(filenames.begin()->string());
          for(auto &target_source : target.array("target_sources")) {
            for(auto &source : target_source.array("sources")) {
              auto source_file = filesystem::get_normal_path(source.string());
              if(source_file == file_path)
                return executable;
              auto source_file_directory = source_file.parent_path();
              if(filesystem::file_in_path(file_path, source_file_directory)) {
                auto size = std::distance(source_file_directory.begin(), source_file_directory.end());
                if(size > best_match_size) {
                  best_match_size = size;
                  best_match_executable = executable;
                }
              }
            }
          }
        }
      }
    }
    catch(...) {
    }
  }

  return best_match_executable;
}
