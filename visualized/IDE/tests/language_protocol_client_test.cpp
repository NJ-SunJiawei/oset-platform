#include "source_language_protocol.hpp"
#include <glib.h>

// Requires display server to work
// However, it is possible to use the Broadway backend if the test is run in a pure terminal environment:
// broadwayd&
// make test

void flush_events() {
  while(Gtk::Main::events_pending())
    Gtk::Main::iteration();
}

int main() {
  auto app = Gtk::Application::create();
  Gsv::init();

  auto tests_path = boost::filesystem::canonical(JUCI_TESTS_PATH);
  auto build_path = boost::filesystem::canonical(JUCI_BUILD_PATH);

  auto view = new Source::LanguageProtocolView(boost::filesystem::canonical(tests_path / "language_protocol_test_files" / "main.rs"),
                                               Source::LanguageManager::get_default()->get_language("rust"),
                                               "rust",
                                               (build_path / "tests" / "language_protocol_server_test").string());

  while(!view->initialized)
    flush_events();

  g_assert(view->capabilities.document_formatting);

  view->get_buffer()->insert_at_cursor(" ");
  g_assert(view->get_buffer()->get_text() == R"( fn main() {
    let a = 2;
    println!("{}", a);
}
)");

  view->format_style(false, false);
  g_assert(view->get_buffer()->get_text() == R"(fn main() {
    let a = 2;
    println!("{}", a);
}
)");

  g_assert(view->get_declaration_locations);
  auto locations = view->get_declaration_locations();
  g_assert(locations.size() == 1);
  g_assert(locations[0]);
  g_assert(locations[0].file_path == "main.rs");
  g_assert_cmpuint(locations[0].line, ==, 0);
  g_assert_cmpuint(locations[0].index, ==, 3);

  g_assert(view->get_type_declaration_locations);
  locations = view->get_type_declaration_locations();
  g_assert(locations.size() == 1);
  g_assert(locations[0]);
  g_assert(locations[0].file_path == "main.rs");
  g_assert_cmpuint(locations[0].line, ==, 0);
  g_assert_cmpuint(locations[0].index, ==, 4);

  g_assert(view->get_implementation_locations);
  locations = view->get_implementation_locations();
  g_assert(locations.size() == 2);
  g_assert(locations[0].file_path == "main.rs");
  g_assert(locations[1].file_path == "main.rs");
  g_assert_cmpuint(locations[0].line, ==, 0);
  g_assert_cmpuint(locations[0].index, ==, 0);
  g_assert_cmpuint(locations[1].line, ==, 1);
  g_assert_cmpuint(locations[1].index, ==, 0);

  g_assert(view->get_usages);
  auto usages = view->get_usages();
  g_assert(usages.size() == 2);
  g_assert(usages[0].first.file_path == view->file_path);
  g_assert(usages[1].first.file_path == view->file_path);
  g_assert_cmpuint(usages[0].first.line, ==, 1);
  g_assert_cmpuint(usages[0].first.index, ==, 8);
  g_assert_cmpuint(usages[1].first.line, ==, 2);
  g_assert_cmpuint(usages[1].first.index, ==, 19);
  g_assert(usages[0].second == "let <b>a</b> = 2;");
  g_assert(usages[1].second == "println!(&quot;{}&quot;, <b>a</b>);");

  g_assert(view->get_methods);
  auto methods = view->get_methods();
  g_assert(methods.size() == 1);
  g_assert_cmpuint(methods[0].first.line, ==, 0);
  g_assert_cmpuint(methods[0].first.index, ==, 0);
  g_assert(methods[0].second == "1: <b>main</b>");

  std::atomic<int> exit_status(-1);
  view->client->on_exit_status = [&exit_status](int exit_status_) {
    exit_status = exit_status_;
  };

  delete view;

  while(exit_status == -1)
    flush_events();

  g_assert_cmpint(exit_status, ==, 0);
}
