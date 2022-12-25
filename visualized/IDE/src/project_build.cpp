#include "project_build.hpp"
#include "config.hpp"
#include "filesystem.hpp"
#include "json.hpp"
#include "terminal.hpp"
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <regex>

std::unique_ptr<Project::Build> Project::Build::create(const boost::filesystem::path &path) {
  if(path.empty())
    return std::make_unique<Project::Build>();

  boost::system::error_code ec;
  auto search_path = boost::filesystem::is_directory(path, ec) ? path : path.parent_path();

  while(true) {
    if(boost::filesystem::exists(search_path / "CMakeLists.txt", ec)) {
      std::unique_ptr<Project::Build> build(new CMakeBuild(path));
      if(!build->project_path.empty())
        return build;
      else
        return std::make_unique<Project::Build>();
    }

    if(boost::filesystem::exists(search_path / "meson.build"), ec) {
      std::unique_ptr<Project::Build> build(new MesonBuild(path));
      if(!build->project_path.empty())
        return build;
    }

    if(boost::filesystem::exists(search_path / Config::get().project.default_build_path / "compile_commands.json", ec)) {
      std::unique_ptr<Project::Build> build(new CompileCommandsBuild());
      build->project_path = search_path;
      return build;
    }

    if(boost::filesystem::exists(search_path / "Cargo.toml", ec)) {
      std::unique_ptr<Project::Build> build(new CargoBuild());
      build->project_path = search_path;
      return build;
    }

    if(boost::filesystem::exists(search_path / "package.json", ec)) {
      std::unique_ptr<Project::Build> build(new NpmBuild());
      build->project_path = search_path;
      return build;
    }

    if(boost::filesystem::exists(search_path / "__main__.py", ec)) {
      std::unique_ptr<Project::Build> build(new PythonMain());
      build->project_path = search_path;
      return build;
    }

    if(boost::filesystem::exists(search_path / "go.mod", ec)) {
      std::unique_ptr<Project::Build> build(new GoBuild());
      build->project_path = search_path;
      return build;
    }

    if(search_path == search_path.root_directory())
      break;
    search_path = search_path.parent_path();
  }

  return std::make_unique<Project::Build>();
}

boost::filesystem::path Project::Build::get_default_path() {
  if(project_path.empty())
    return boost::filesystem::path();

  boost::filesystem::path default_build_path = Config::get().project.default_build_path;
  auto default_build_path_string = default_build_path.string();

  boost::replace_all(default_build_path_string, "<project_directory_name>", project_path.filename().string());
  default_build_path = default_build_path_string;

  if(default_build_path.is_relative())
    default_build_path = project_path / default_build_path;

  return filesystem::get_normal_path(default_build_path);
}

boost::filesystem::path Project::Build::get_debug_path() {
  if(project_path.empty())
    return boost::filesystem::path();

  boost::filesystem::path debug_build_path = Config::get().project.debug_build_path;
  auto debug_build_path_string = debug_build_path.string();

  boost::replace_all(debug_build_path_string, "<default_build_path>", Config::get().project.default_build_path);

  boost::replace_all(debug_build_path_string, "<project_directory_name>", project_path.filename().string());
  debug_build_path = debug_build_path_string;

  if(debug_build_path.is_relative())
    debug_build_path = project_path / debug_build_path;

  return filesystem::get_normal_path(debug_build_path);
}

std::vector<std::string> Project::Build::get_exclude_folders() {
  auto default_build_path = Config::get().project.default_build_path;
  boost::replace_all(default_build_path, "<project_directory_name>", "");

  auto debug_build_path = Config::get().project.debug_build_path;
  boost::replace_all(debug_build_path, "<default_build_path>", Config::get().project.default_build_path);
  boost::replace_all(debug_build_path, "<project_directory_name>", "");

  return std::vector<std::string>{
      ".git", "build", "debug",                                                                                                       // Common exclude folders
      boost::filesystem::path(default_build_path).filename().string(), boost::filesystem::path(debug_build_path).filename().string(), // C/C++
      "target",                                                                                                                       // Rust
      "node_modules", "dist", "coverage", ".expo",                                                                                    // JavaScript
      ".mypy_cache",
      "__pycache__" // Python
  };
}

Project::CMakeBuild::CMakeBuild(const boost::filesystem::path &path) : Project::Build(), cmake(path) {
  project_path = cmake.project_path;
}

bool Project::CMakeBuild::update_default(bool force) {
  return cmake.update_default_build(get_default_path(), force);
}

bool Project::CMakeBuild::update_debug(bool force) {
  return cmake.update_debug_build(get_debug_path(), force);
}

std::string Project::CMakeBuild::get_compile_command() {
  return Config::get().project.cmake.compile_command;
}

boost::filesystem::path Project::CMakeBuild::get_executable(const boost::filesystem::path &path) {
  auto default_path = get_default_path();
  auto executable = cmake.get_executable(default_path, path);
  if(executable.empty()) {
    auto src_path = project_path / "src";
    boost::system::error_code ec;
    if(boost::filesystem::is_directory(src_path, ec))
      executable = CMake(src_path).get_executable(default_path, src_path);
  }
  return executable;
}

bool Project::CMakeBuild::is_valid(const boost::filesystem::path &build_path) const {
  if(project_path.empty() || build_path.empty())
    return true;
  std::ifstream input((build_path / "CMakeCache.txt").string(), std::ios::binary);
  if(!input)
    return true;
  std::string line;
  bool correct_source_dir = false;
  bool correct_cmake_command = false;
  while(std::getline(input, line)) {
    const static std::regex source_dir_regex("^.*_SOURCE_DIR:STATIC=(.*)\r?$", std::regex::optimize);
    const static std::regex cmake_command_regex("^CMAKE_COMMAND:INTERNAL=(.*)\r?$", std::regex::optimize);
    std::smatch sm;
    if(std::regex_match(line, sm, source_dir_regex))
      correct_source_dir |= boost::filesystem::path(sm[1].str()) == project_path;
    else if(std::regex_match(line, sm, cmake_command_regex))
      correct_cmake_command |= filesystem::is_executable(sm[1].str());
  }
  return correct_source_dir && correct_cmake_command;
}

Project::MesonBuild::MesonBuild(const boost::filesystem::path &path) : Project::Build(), meson(path) {
  project_path = meson.project_path;
}

bool Project::MesonBuild::update_default(bool force) {
  return meson.update_default_build(get_default_path(), force);
}

bool Project::MesonBuild::update_debug(bool force) {
  return meson.update_debug_build(get_debug_path(), force);
}

std::string Project::MesonBuild::get_compile_command() {
  return Config::get().project.meson.compile_command;
}

boost::filesystem::path Project::MesonBuild::get_executable(const boost::filesystem::path &path) {
  auto default_path = get_default_path();
  auto executable = meson.get_executable(default_path, path);
  if(executable.empty()) {
    auto src_path = project_path / "src";
    boost::system::error_code ec;
    if(boost::filesystem::is_directory(src_path, ec))
      executable = meson.get_executable(default_path, src_path);
  }
  return executable;
}

bool Project::MesonBuild::is_valid(const boost::filesystem::path &build_path) const {
  if(project_path.empty() || build_path.empty())
    return true;
  try {
    JSON info(build_path / "meson-info" / "meson-info.json");
    return boost::filesystem::path(info.object("directories").string("source")) == project_path;
  }
  catch(...) {
  }
  return true;
}

bool Project::CargoBuild::update_default(bool force) {
  auto default_build_path = get_default_path();
  if(default_build_path.empty())
    return false;

  boost::system::error_code ec;
  if(!boost::filesystem::exists(default_build_path, ec)) {
    boost::system::error_code ec;
    boost::filesystem::create_directories(default_build_path, ec);
    if(ec) {
      Terminal::get().print("\e[31mError\e[m: could not create " + filesystem::get_short_path(default_build_path).string() + ": " + ec.message() + "\n", true);
      return false;
    }
  }
  return true;
}

bool Project::CargoBuild::update_debug(bool force) {
  return update_default(force);
}

std::string Project::CargoBuild::get_compile_command() {
  return Config::get().project.cargo_command + " build";
}

boost::filesystem::path Project::CargoBuild::get_executable(const boost::filesystem::path &path) {
  auto project_name = project_path.filename().string();
  for(auto &chr : project_name) {
    if(chr == ' ')
      chr = '_';
  }
  return get_debug_path() / project_name;
}
