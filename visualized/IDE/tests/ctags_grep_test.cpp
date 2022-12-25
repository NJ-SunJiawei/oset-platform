#include "config.hpp"
#include "ctags.hpp"
#include "grep.hpp"
#include <glib.h>
#include <gtkmm.h>
#include <gtksourceviewmm.h>

void ctags_grep_test_function() {
}

class Test {
  void ctags_grep_test_function2() {
  }
};

int main() {
  auto app = Gtk::Application::create();
  Gsv::init();

#ifdef JUCI_USE_UCTAGS
  Config::get().project.ctags_command = "uctags";
#else
  Config::get().project.ctags_command = "ctags";
#endif
  Config::get().project.grep_command = "grep";
  Config::get().project.default_build_path = "build";
  Config::get().project.debug_build_path = "build";

  auto tests_path = boost::filesystem::canonical(JUCI_TESTS_PATH);

  // Ctags tests
  {
    Ctags ctags(tests_path);
    g_assert(ctags.project_path == tests_path.parent_path());
    bool found = false;
    std::string line;
    while(std::getline(ctags.output, line)) {
      if(line.find("ctags_grep_test_function") != std::string::npos) {
        {
          auto location = ctags.get_location(line, false);
          g_assert(location.source == "void ctags_grep_test_function() {");
          g_assert(ctags.project_path / location.file_path == tests_path / "ctags_grep_test.cpp");
          g_assert_cmpint(location.line, ==, 7);
          g_assert_cmpint(location.index, ==, 5);
          g_assert(location.symbol == "ctags_grep_test_function");
          g_assert(location.scope.empty());
          g_assert(location.kind.empty());
        }
        {
          auto location = ctags.get_location(line, true);
          g_assert(location.source == "void <b>ctags_grep_test_function</b>() {");
          g_assert(ctags.project_path / location.file_path == tests_path / "ctags_grep_test.cpp");
          g_assert_cmpint(location.line, ==, 7);
          g_assert_cmpint(location.index, ==, 5);
          g_assert(location.symbol == "ctags_grep_test_function");
          g_assert(location.scope.empty());
          g_assert(location.kind.empty());
        }
        found = true;
        break;
      }
    }
    g_assert(found == true);
  }
  {
    Ctags ctags(tests_path, false, true);
    g_assert(ctags.project_path == tests_path.parent_path());
    bool found = false;
    std::string line;
    while(std::getline(ctags.output, line)) {
      if(line.find("ctags_grep_test_function") != std::string::npos) {
        {
          auto location = ctags.get_location(line, false);
          g_assert(location.source == "void ctags_grep_test_function() {");
          g_assert(ctags.project_path / location.file_path == tests_path / "ctags_grep_test.cpp");
          g_assert_cmpint(location.line, ==, 7);
          g_assert_cmpint(location.index, ==, 5);
          g_assert(location.symbol == "ctags_grep_test_function");
          g_assert(location.scope.empty());
          g_assert(location.kind == "function");
        }
        {
          auto location = ctags.get_location(line, true);
          g_assert(location.source == "void <b>ctags_grep_test_function</b>() {");
          g_assert(ctags.project_path / location.file_path == tests_path / "ctags_grep_test.cpp");
          g_assert_cmpint(location.line, ==, 7);
          g_assert_cmpint(location.index, ==, 5);
          g_assert(location.symbol == "ctags_grep_test_function");
          g_assert(location.scope.empty());
          g_assert(location.kind == "function");
        }
        found = true;
        break;
      }
    }
    g_assert(found == true);
  }
  {
    Ctags ctags(tests_path, true);
    g_assert(ctags.project_path == tests_path.parent_path());
    bool found = false;
    std::string line;
    while(std::getline(ctags.output, line)) {
      if(line.find("ctags_grep_test_function2") != std::string::npos) {
        {
          auto location = ctags.get_location(line, false);
          g_assert(location.source == "void ctags_grep_test_function2() {");
          g_assert(ctags.project_path / location.file_path == tests_path / "ctags_grep_test.cpp");
          g_assert_cmpint(location.line, ==, 11);
          g_assert_cmpint(location.index, ==, 7);
          g_assert(location.symbol == "ctags_grep_test_function2");
          g_assert(location.scope == "Test");
          g_assert(location.kind.empty());
        }
        found = true;
        break;
      }
    }
    g_assert(found == true);
  }

  // Grep tests
  {
    Grep grep(tests_path, "ctags_grep_test_function", true, false);
    g_assert(grep.project_path == tests_path.parent_path());
    bool found = false;
    std::string line;
    while(std::getline(grep.output, line)) {
      if(line.find("ctags_grep_test_function") != std::string::npos) {
        {
          auto location = grep.get_location(line, true, true);
          g_assert(location.markup == "tests/ctags_grep_test.cpp:8:void <b>ctags_grep_test_function</b>() {");
          g_assert(grep.project_path / location.file_path == tests_path / "ctags_grep_test.cpp");
          g_assert_cmpint(location.line, ==, 7);
          g_assert_cmpint(location.offset, ==, 5);
          {
            auto location2 = grep.get_location(location.markup, false, true);
            g_assert(location2.markup == "tests/ctags_grep_test.cpp:8:void <b>ctags_grep_test_function</b>() {");
            g_assert(grep.project_path / location2.file_path == tests_path / "ctags_grep_test.cpp");
            g_assert_cmpint(location2.line, ==, 7);
            g_assert_cmpint(location2.offset, ==, 5);
          }
        }
        {
          auto location = grep.get_location(line, true, false);
          g_assert(location.markup == "tests/ctags_grep_test.cpp:8:void <b>ctags_grep_test_function</b>() {");
          g_assert(grep.project_path / location.file_path == tests_path / "ctags_grep_test.cpp");
          g_assert_cmpint(location.line, ==, 7);
          g_assert_cmpint(location.offset, ==, 0);
          {
            auto location2 = grep.get_location(location.markup, false, false);
            g_assert(location2.markup == "tests/ctags_grep_test.cpp:8:void <b>ctags_grep_test_function</b>() {");
            g_assert(grep.project_path / location2.file_path == tests_path / "ctags_grep_test.cpp");
            g_assert_cmpint(location2.line, ==, 7);
            g_assert_cmpint(location2.offset, ==, 0);
          }
        }
        {
          auto location = grep.get_location(line, true, true, (boost::filesystem::path("tests") / "ctags_grep_test.cpp").string());
          g_assert(location.markup == "tests/ctags_grep_test.cpp:8:void <b>ctags_grep_test_function</b>() {");
          g_assert(grep.project_path / location.file_path == tests_path / "ctags_grep_test.cpp");
          g_assert(location);
          g_assert_cmpint(location.line, ==, 7);
          g_assert_cmpint(location.offset, ==, 5);
        }
        {
          auto location = grep.get_location(line, true, true, "CMakeLists.txt");
          g_assert(location.markup == "tests/ctags_grep_test.cpp:8:void <b>ctags_grep_test_function</b>() {");
          g_assert(location.file_path.empty());
          g_assert(!location);
        }
        found = true;
        break;
      }
      else
        g_assert(false);
    }
    g_assert(found == true);
  }
  {
    auto pattern = std::string("C") + "tags_grep_test_function";
    {
      Grep grep(tests_path, pattern, true, false);
      g_assert(grep.project_path == tests_path.parent_path());
      bool found = false;
      std::string line;
      while(std::getline(grep.output, line)) {
        if(line.find("ctags_grep_test_function") != std::string::npos)
          found = true;
      }
      g_assert(found == false);
    }
    {
      Grep grep(tests_path, pattern, false, false);
      g_assert(grep.project_path == tests_path.parent_path());
      bool found = false;
      std::string line;
      while(std::getline(grep.output, line)) {
        if(line.find("ctags_grep_test_function") != std::string::npos)
          found = true;
      }
      g_assert(found == true);
    }
  }
}
