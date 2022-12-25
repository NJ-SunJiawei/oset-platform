#include "cmake.hpp"
#include "config.hpp"
#include "process.hpp"
#include "project_build.hpp"
#include <boost/filesystem.hpp>
#include <glib.h>
#include <gtksourceviewmm.h>

int main() {
  auto app = Gtk::Application::create();
  Gsv::init();
  Config::get().project.default_build_path = "./build";
  Config::get().project.cmake.command = "cmake";

  auto tests_path = boost::filesystem::canonical(JUCI_TESTS_PATH);
  auto cmake_test_files_path = boost::filesystem::canonical(tests_path / "cmake_file_api_test_files");

  CMake cmake(cmake_test_files_path);
  g_assert(cmake.project_path == cmake_test_files_path);

  auto build = Project::Build::create(cmake_test_files_path);
  g_assert(dynamic_cast<Project::CMakeBuild *>(build.get()));

  g_assert(build->update_default(true));

  g_assert(build->get_executable(cmake_test_files_path / "main.cpp") == cmake_test_files_path / "build" / "custom_build_folder" / "custom_executable");
}
