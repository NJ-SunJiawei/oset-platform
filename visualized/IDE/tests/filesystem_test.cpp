#include "filesystem.hpp"
#include <glib.h>

int main() {
  {
    auto path = filesystem::get_home_path();
    g_assert(!path.empty());
    g_assert(boost::filesystem::exists(path));
    g_assert(boost::filesystem::is_directory(path));
  }
  {
    auto path = filesystem::get_current_path();
    g_assert(!path.empty());
    g_assert(boost::filesystem::exists(path));
    g_assert(boost::filesystem::is_directory(path));
  }

  {
    auto paths = filesystem::get_executable_search_paths();
    g_assert(!paths.empty());
    size_t count = 0;
    for(auto &path : paths) {
      g_assert(!path.empty());
      if(boost::filesystem::exists(path)) {
        g_assert(boost::filesystem::is_directory(path));
        ++count;
      }
    }
    g_assert(count > 0);
  }

  {
    auto original = "test () '\"";
    auto escaped = filesystem::escape_argument(original);
    g_assert_cmpstr(escaped.c_str(), ==, "test\\ \\(\\)\\ \\'\\\"");
    auto unescaped = filesystem::unescape_argument(escaped);
    g_assert_cmpstr(unescaped.c_str(), ==, original);
  }
  {
    auto unescaped = filesystem::unescape_argument("'test \\()\"\\''");
    g_assert_cmpstr(unescaped.c_str(), ==, "test \\()\"'");
  }
  {
    auto unescaped = filesystem::unescape_argument("\"test \\'()\\\"\"");
    g_assert_cmpstr(unescaped.c_str(), ==, "test \\'()\"");
  }
  {
    auto unescaped = filesystem::unescape_argument("\\\\");
    g_assert_cmpstr(unescaped.c_str(), ==, "\\");
  }
  {
    auto unescaped = filesystem::unescape_argument("\\\\\\ ");
    g_assert_cmpstr(unescaped.c_str(), ==, "\\ ");
  }
  {
    auto unescaped = filesystem::unescape_argument("\\\\\\ \\ \\ \\\\");
    g_assert_cmpstr(unescaped.c_str(), ==, "\\   \\");
  }
  {
    auto unescaped = filesystem::unescape_argument("c:\\a\\ b\\c");
    g_assert_cmpstr(unescaped.c_str(), ==, "c:\\a b\\c");
  }
  {
    auto unescaped = filesystem::unescape_argument("\"\\\\\\\"\"");
    g_assert_cmpstr(unescaped.c_str(), ==, "\\\"");
  }
  {
    auto unescaped = filesystem::unescape_argument("\"\\\"\"");
    g_assert_cmpstr(unescaped.c_str(), ==, "\"");
  }
  {
    auto unescaped = filesystem::unescape_argument("\"a\\b\"");
    g_assert_cmpstr(unescaped.c_str(), ==, "a\\b");
  }

  auto tests_path = boost::filesystem::canonical(JUCI_TESTS_PATH);
  {
    g_assert(filesystem::file_in_path(tests_path / "filesystem_test.cc", tests_path));
    g_assert(!filesystem::file_in_path(boost::filesystem::canonical(tests_path / ".." / "CMakeLists.txt"), tests_path));
  }

  auto license_file = boost::filesystem::canonical(tests_path / ".." / "LICENSE");
  {
    g_assert(filesystem::find_file_in_path_parents("LICENSE", tests_path) == license_file);
  }

  {
    g_assert(filesystem::get_normal_path(tests_path / ".." / "LICENSE") == license_file);
    g_assert(filesystem::get_normal_path("/foo") == "/foo");
    g_assert(filesystem::get_normal_path("/foo/") == "/foo");
    g_assert(filesystem::get_normal_path("/foo/.") == "/foo");
    g_assert(filesystem::get_normal_path("/foo/./bar/..////") == "/foo");
    g_assert(filesystem::get_normal_path("/foo/.///bar/../") == "/foo");
    g_assert(filesystem::get_normal_path("../foo") == "../foo");
    g_assert(filesystem::get_normal_path("../../foo") == "../../foo");
    g_assert(filesystem::get_normal_path("../././foo") == "../foo");
    g_assert(filesystem::get_normal_path("/../foo") == "/../foo");
  }

  {
    boost::filesystem::path relative_path = "filesystem_test.cc";
    g_assert(filesystem::get_relative_path(tests_path / relative_path, tests_path) == relative_path);
    g_assert(filesystem::get_relative_path(tests_path / "test" / relative_path, tests_path) == boost::filesystem::path("test") / relative_path);

    g_assert(filesystem::get_relative_path("/test/test/test.cc", "/test/base") == boost::filesystem::path("..") / "test" / "test.cc");
    g_assert(filesystem::get_relative_path("/test/test/test/test.cc", "/test/base") == boost::filesystem::path("..") / "test" / "test" / "test.cc");
    g_assert(filesystem::get_relative_path("/test2/test.cc", "/test/base") == boost::filesystem::path("..") / ".." / "test2" / "test.cc");
  }

  {
    g_assert(filesystem::get_absolute_path("./test1/test2", "/home") == boost::filesystem::path("/") / "home" / "." / "test1" / "test2");
    g_assert(filesystem::get_absolute_path("../test1/test2", "/home") == boost::filesystem::path("/") / "home" / ".." / "test1" / "test2");
    g_assert(filesystem::get_absolute_path("test1/test2", "/home") == boost::filesystem::path("/") / "home" / "test1" / "test2");
    g_assert(filesystem::get_absolute_path("/test1/test2", "/home") == boost::filesystem::path("/") / "test1" / "test2");
    g_assert(filesystem::get_absolute_path("~/test1/test2", "/home") == boost::filesystem::path("~") / "test1" / "test2");
  }

  {
    boost::filesystem::path path = "/ro ot/te stæøå.txt";
    auto uri = filesystem::get_uri_from_path(path);
    g_assert(uri == "file:///ro%20ot/te%20st%C3%A6%C3%B8%C3%A5.txt");
    g_assert(path == filesystem::get_path_from_uri(uri));
  }

#ifdef _WIN32
  {
    std::string uri = "file:///c:/Users/test";
    g_assert(filesystem::get_path_from_uri(uri) == "C:/Users/test");

    boost::filesystem::path path = "C:/Users/test";
    g_assert(filesystem::get_uri_from_path(path) == "file:///C:/Users/test");
  }
#endif

  {
    auto ls = filesystem::get_executable("ls");
    g_assert(ls == "ls");

    auto non_existing_executable = filesystem::get_executable("non_existing_executable");
    g_assert(non_existing_executable == "non_existing_executable");
  }

  {
    g_assert(!filesystem::is_executable(filesystem::get_home_path()));
    g_assert(!filesystem::is_executable(filesystem::get_current_path()));
    g_assert(!filesystem::is_executable(tests_path));
#ifdef _WIN32
    g_assert(filesystem::is_executable(tests_path / ".." / "LICENSE"));
#else
    g_assert(!filesystem::is_executable(tests_path / ".." / "LICENSE"));
#endif
    auto ls = filesystem::find_executable("ls");
    g_assert(!ls.empty());
    g_assert(filesystem::is_executable(ls));
  }
}
