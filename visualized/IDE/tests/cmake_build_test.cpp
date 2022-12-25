#include "cmake.hpp"
#include "config.hpp"
#include "process.hpp"
#include "project_build.hpp"
#include <boost/filesystem.hpp>
#include <glib.h>

int main() {
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("", variables, [&called](CMake::Function && /*function*/) {
      called = true;
    });
    g_assert(!called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("project(", variables, [&called](CMake::Function && /*function*/) {
      called = true;
    });
    g_assert(!called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("project(test", variables, [&called](CMake::Function && /*function*/) {
      called = true;
    });
    g_assert(!called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("project(test)", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "project");
      g_assert(function.parameters.size() == 1);
      g_assert(function.parameters.front() == "test");
    });
    g_assert(variables.size() == 2);
    auto it = variables.begin();
    g_assert(it->first == "CMAKE_PROJECT_NAME");
    g_assert(it->second == std::list<std::string>{"test"});
    ++it;
    g_assert(it->first == "PROJECT_NAME");
    g_assert(it->second == std::list<std::string>{"test"});
    g_assert(called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("project(\"test\")", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "project");
      g_assert(function.parameters.size() == 1);
      g_assert(function.parameters.front() == "test");
    });
    g_assert(called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("project(\"te\\\"st\")", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "project");
      g_assert(function.parameters.size() == 1);
      g_assert(function.parameters.front() == "te\"st");
    });
    g_assert(called);
  }
  {
    int called = 0;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("set(TEST testing)\nadd_executable(${TEST} test.cpp)", variables, [&called](CMake::Function &&function) {
      called++;
      if(called == 1)
        g_assert(function.name == "set");
      else {
        g_assert(function.name == "add_executable");
        g_assert(function.parameters.size() == 2);
        auto it = function.parameters.begin();
        g_assert(*it == "testing");
        g_assert(*(++it) == "test.cpp");
      }
    });
    g_assert(called == 2);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("test(${})", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "test");
      g_assert(function.parameters.size() == 1);
      g_assert(function.parameters.front() == "");
    });
    g_assert(called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("test(\"${}\")", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "test");
      g_assert(function.parameters.size() == 1);
      g_assert(function.parameters.front() == "");
    });
    g_assert(called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("test($TEST)", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "test");
      g_assert(function.parameters.size() == 1);
      g_assert(function.parameters.front() == "$TEST");
    });
    g_assert(called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("test(${TEST})", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "test");
      g_assert(function.parameters.size() == 1);
      g_assert(function.parameters.front() == "");
    });
    g_assert(called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("test(\"$TEST\")", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "test");
      g_assert(function.parameters.size() == 1);
      g_assert(function.parameters.front() == "$TEST");
    });
    g_assert(called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("test(\"${TEST}\")", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "test");
      g_assert(function.parameters.size() == 1);
      g_assert(function.parameters.front() == "");
    });
    g_assert(called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("test(${TEST} ${TEST})", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "test");
      g_assert(function.parameters.size() == 2);
      auto it = function.parameters.begin();
      g_assert(*it == "");
      g_assert(*(++it) == "");
    });
    g_assert(called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("test(\"${TEST}\" \"${TEST}\")", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "test");
      g_assert(function.parameters.size() == 2);
      auto it = function.parameters.begin();
      g_assert(*it == "");
      g_assert(*(++it) == "");
    });
    g_assert(called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("test(${TEST} \"${TEST}\")", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "test");
      g_assert(function.parameters.size() == 2);
      auto it = function.parameters.begin();
      g_assert(*it == "");
      g_assert(*(++it) == "");
    });
    g_assert(called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("test(\"${TEST}\" ${TEST})", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "test");
      g_assert(function.parameters.size() == 2);
      auto it = function.parameters.begin();
      g_assert(*it == "");
      g_assert(*(++it) == "");
    });
    g_assert(called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("test(\"\" \"\")", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "test");
      g_assert(function.parameters.size() == 2);
      auto it = function.parameters.begin();
      g_assert(*it == "");
      g_assert(*(++it) == "");
    });
    g_assert(called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("test(    \"\"   \"\"    )", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "test");
      g_assert(function.parameters.size() == 2);
      auto it = function.parameters.begin();
      g_assert(*it == "");
      g_assert(*(++it) == "");
    });
    g_assert(called);
  }
  {
    bool called = false;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("test\n(\n\"\"\n\"\"\n)", variables, [&called](CMake::Function &&function) {
      called = true;
      g_assert(function.name == "test");
      g_assert(function.parameters.size() == 2);
      auto it = function.parameters.begin();
      g_assert(*it == "");
      g_assert(*(++it) == "");
    });
    g_assert(called);
  }
  {
    int called = 0;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("set(TEST testing)\nadd_executable(test${TEST}test test.cpp)", variables, [&called](CMake::Function &&function) {
      called++;
      if(called == 1)
        g_assert(function.name == "set");
      else {
        g_assert(function.name == "add_executable");
        g_assert(function.parameters.size() == 2);
        auto it = function.parameters.begin();
        g_assert(*it == "testtestingtest");
        g_assert(*(++it) == "test.cpp");
      }
    });
    g_assert(called == 2);
  }
  {
    int called = 0;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("set(TEST testing)\nadd_executable(\"${TEST}\" test.cpp)", variables, [&called](CMake::Function &&function) {
      called++;
      if(called == 1)
        g_assert(function.name == "set");
      else {
        g_assert(function.name == "add_executable");
        g_assert(function.parameters.size() == 2);
        auto it = function.parameters.begin();
        g_assert(*it == "testing");
        g_assert(*(++it) == "test.cpp");
      }
    });
    g_assert(called == 2);
  }
  {
    int called = 0;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("set(TEST testing)\nadd_executable(\"test${TEST}test\" test.cpp)", variables, [&called](CMake::Function &&function) {
      called++;
      if(called == 1)
        g_assert(function.name == "set");
      else {
        g_assert(function.name == "add_executable");
        g_assert(function.parameters.size() == 2);
        auto it = function.parameters.begin();
        g_assert(*it == "testtestingtest");
        g_assert(*(++it) == "test.cpp");
      }
    });
    g_assert(called == 2);
  }
  {
    int called = 0;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("set(TEST 1 2 3)\nadd_executable(\"${TEST}\" test.cpp)", variables, [&called](CMake::Function &&function) {
      called++;
      if(called == 1)
        g_assert(function.name == "set");
      else {
        g_assert(function.name == "add_executable");
        g_assert(function.parameters.size() == 2);
        auto it = function.parameters.begin();
        g_assert(*it == "1;2;3");
        g_assert(*(++it) == "test.cpp");
      }
    });
    g_assert(called == 2);
  }
  {
    int called = 0;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("set(TEST 1 2 3)\nadd_executable(\"aaa${TEST}bbb\" test.cpp)", variables, [&called](CMake::Function &&function) {
      called++;
      if(called == 1)
        g_assert(function.name == "set");
      else {
        g_assert(function.name == "add_executable");
        g_assert(function.parameters.size() == 2);
        auto it = function.parameters.begin();
        g_assert(*it == "aaa1;2;3bbb");
        g_assert(*(++it) == "test.cpp");
      }
    });
    g_assert(called == 2);
  }
  {
    int called = 0;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("set(TEST 1 2 3)\nadd_executable(${TEST} test.cpp)", variables, [&called](CMake::Function &&function) {
      called++;
      if(called == 1)
        g_assert(function.name == "set");
      else {
        g_assert(function.name == "add_executable");
        g_assert(function.parameters.size() == 4);
        auto it = function.parameters.begin();
        g_assert(*it == "1");
        g_assert(*(++it) == "2");
        g_assert(*(++it) == "3");
        g_assert(*(++it) == "test.cpp");
      }
    });
    g_assert(called == 2);
  }
  {
    int called = 0;
    std::map<std::string, std::list<std::string>> variables;
    CMake::parse_file("set(TEST 1 2 3)\nadd_executable(aaa${TEST}bbb test.cpp)", variables, [&called](CMake::Function &&function) {
      called++;
      if(called == 1)
        g_assert(function.name == "set");
      else {
        g_assert(function.name == "add_executable");
        g_assert(function.parameters.size() == 4);
        auto it = function.parameters.begin();
        g_assert(*it == "aaa1");
        g_assert(*(++it) == "2");
        g_assert(*(++it) == "3bbb");
        g_assert(*(++it) == "test.cpp");
      }
    });
    g_assert(called == 2);
  }

  auto tests_path = boost::filesystem::canonical(JUCI_TESTS_PATH);
  {
    auto project_path = boost::filesystem::canonical(tests_path / "..");

    {
      CMake cmake(project_path);
      TinyProcessLib::Process process("cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..", (project_path / "build").string(), [](const char *bytes, size_t n) {});
      g_assert(process.get_exit_status() == 0);

      g_assert(cmake.get_executable(project_path / "build", project_path) == "");
      g_assert(cmake.get_executable(project_path / "build" / "non_existing_file.cpp", project_path) == "");
    }
    {
      CMake cmake(project_path / "src");
      g_assert(cmake.get_executable(project_path / "build", project_path / "src") == project_path / "build" / "src" / "juci");
      g_assert(cmake.get_executable(project_path / "build", project_path / "src" / "cmake.cpp") == project_path / "build" / "src" / "juci");
      g_assert(cmake.get_executable(project_path / "build", project_path / "src" / "juci.cpp") == project_path / "build" / "src" / "juci");
      g_assert(cmake.get_executable(project_path / "build", project_path / "src" / "non_existing_file.cpp") == project_path / "build" / "src" / "juci");
    }
    {
      CMake cmake(tests_path);

      g_assert(cmake.project_path == project_path);

      g_assert(cmake.get_executable(project_path / "build", tests_path).parent_path() == project_path / "build" / "tests");
      g_assert(cmake.get_executable(project_path / "build", tests_path / "cmake_build_test.cpp") == project_path / "build" / "tests" / "cmake_build_test");
      g_assert(cmake.get_executable(project_path / "build", tests_path / "non_existing_file.cpp").parent_path() == project_path / "build" / "tests");
    }

    auto build = Project::Build::create(tests_path);
    g_assert(dynamic_cast<Project::CMakeBuild *>(build.get()));

    build = Project::Build::create(tests_path / "stubs");
    g_assert(dynamic_cast<Project::CMakeBuild *>(build.get()));
    g_assert(build->project_path == project_path);

    Config::get().project.default_build_path = "./build";
    g_assert(build->get_default_path() == project_path / "build");

    Config::get().project.debug_build_path = "<default_build_path>/debug";
    g_assert(build->get_debug_path() == project_path / "build/debug");

    auto project_path_filename = project_path.filename();
    Config::get().project.debug_build_path = "../debug_<project_directory_name>";
    g_assert(build->get_debug_path() == project_path.parent_path() / ("debug_" + project_path_filename.string()));

    Config::get().project.default_build_path = "../build_<project_directory_name>";
    g_assert(build->get_default_path() == project_path.parent_path() / ("build_" + project_path_filename.string()));
  }
  {
    auto project_path = tests_path / "source_clang_test_files";
    CMake cmake(project_path);

    g_assert(cmake.project_path == project_path);

    g_assert(cmake.get_executable(project_path / "build", project_path / "main.cpp") == boost::filesystem::path(".") / "test");
    g_assert(cmake.get_executable(project_path / "build", project_path / "non_existing_file.cpp") == boost::filesystem::path(".") / "test");
    g_assert(cmake.get_executable(project_path / "build", project_path) == boost::filesystem::path(".") / "test");
  }
}
