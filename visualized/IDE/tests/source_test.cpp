#include "config.hpp"
#include "filesystem.hpp"
#include "source.hpp"
#include <glib.h>

std::string hello_world_with_trailing_whitespaces = R"(#include <iostream>  
    
int main() {  
  std::cout << "hello world\n";    
})";

std::string hello_world = R"(#include <iostream>

int main() {
  std::cout << "hello world\n";
}
)";

// Requires display server to work
// However, it is possible to use the Broadway backend if the test is run in a pure terminal environment:
// broadwayd&
// make test

int main() {
  auto app = Gtk::Application::create();
  Gsv::init();

  auto tests_path = boost::filesystem::canonical(JUCI_TESTS_PATH);
  auto source_file = tests_path / "tmp" / "source_file.cpp";

  {
    Source::View view(source_file, Glib::RefPtr<Gsv::Language>());
    view.get_buffer()->set_text(hello_world_with_trailing_whitespaces);
    g_assert(view.save());
  }

  Source::View view(source_file, Glib::RefPtr<Gsv::Language>());
  g_assert(view.get_buffer()->get_text() == hello_world_with_trailing_whitespaces);
  view.remove_trailing_whitespace_characters();
  view.add_missing_newline_at_end_of_file();
  g_assert(view.get_buffer()->get_text() == hello_world);
  view.remove_trailing_whitespace_characters();
  view.add_missing_newline_at_end_of_file();
  g_assert(view.get_buffer()->get_text() == hello_world);

  g_assert(boost::filesystem::remove(source_file));
  g_assert(!boost::filesystem::exists(source_file));

  for(int c = 0; c < 2; ++c) {
    size_t found = 0;
    auto style_scheme_manager = Source::StyleSchemeManager::get_default();
    for(auto &search_path : style_scheme_manager->get_search_path()) {
      if(search_path == Config::get().home_juci_path / "styles") // found added style
        ++found;
    }
    g_assert(found == 1);
  }

  // replace_text tests
  {
    auto buffer = view.get_buffer();
    {
      auto text = "line 1\nline 2\nline3";
      buffer->set_text(text);
      buffer->place_cursor(buffer->begin());
      view.replace_text(text);
      g_assert(buffer->get_text() == text);
      g_assert(buffer->get_insert()->get_iter() == buffer->begin());

      buffer->place_cursor(buffer->end());
      view.replace_text(text);
      g_assert(buffer->get_text() == text);
      g_assert(buffer->get_insert()->get_iter() == buffer->end());

      view.place_cursor_at_line_offset(1, 0);
      view.replace_text(text);
      g_assert(buffer->get_text() == text);
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 0);
    }
    {
      auto old_text = "line 1\nline3";
      auto new_text = "line 1\nline 2\nline3";
      buffer->set_text(old_text);
      view.place_cursor_at_line_offset(1, 0);
      view.replace_text(new_text);
      g_assert(buffer->get_text() == new_text);
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 0);

      view.replace_text(old_text);
      g_assert(buffer->get_text() == old_text);
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 0);

      view.place_cursor_at_line_offset(0, 0);
      view.replace_text(new_text);
      g_assert(buffer->get_text() == new_text);
      g_assert(buffer->get_insert()->get_iter().get_line() == 0);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 0);

      view.replace_text(old_text);
      g_assert(buffer->get_text() == old_text);
      g_assert(buffer->get_insert()->get_iter().get_line() == 0);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 0);

      view.replace_text(new_text);
      g_assert(buffer->get_text() == new_text);

      view.place_cursor_at_line_offset(2, 0);
      view.replace_text(old_text);
      g_assert(buffer->get_text() == old_text);
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 0);
    }
    {
      auto old_text = "line 1\nline 3";
      auto new_text = "";
      buffer->set_text(old_text);
      view.replace_text(new_text);
      g_assert(buffer->get_text() == new_text);

      view.replace_text(old_text);
      g_assert(buffer->get_text() == old_text);
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 6);
    }
    {
      auto old_text = "";
      auto new_text = "";
      buffer->set_text(old_text);
      view.replace_text(new_text);
      g_assert(buffer->get_text() == new_text);
    }
    {
      auto old_text = "line 1\nline 3\n";
      auto new_text = "";
      buffer->set_text(old_text);
      view.replace_text(new_text);
      g_assert(buffer->get_text() == new_text);

      view.replace_text(old_text);
      g_assert(buffer->get_text() == old_text);
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 0);
    }
    {
      auto old_text = "line 1\n\nline 3\nline 4\n\nline 5\n";
      auto new_text = "line 1\n\nline 33\nline 44\n\nline 5\n";
      buffer->set_text(old_text);
      view.place_cursor_at_line_offset(2, 0);
      view.replace_text(new_text);
      g_assert(buffer->get_text() == new_text);
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 0);

      buffer->set_text(old_text);
      view.place_cursor_at_line_offset(3, 0);
      view.replace_text(new_text);
      g_assert(buffer->get_text() == new_text);
      g_assert(buffer->get_insert()->get_iter().get_line() == 3);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 0);
    }
    {
      auto old_text = "line 1\n\nline 3\nline 4\n\nline 5\n";
      auto new_text = "line 1\n\nline 33\nline 44\nline 45\nline 46\n\nline 5\n";
      buffer->set_text(old_text);
      view.place_cursor_at_line_offset(2, 0);
      view.replace_text(new_text);
      g_assert(buffer->get_text() == new_text);
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 0);

      buffer->set_text(old_text);
      view.place_cursor_at_line_offset(3, 0);
      view.replace_text(new_text);
      g_assert(buffer->get_text() == new_text);
      g_assert(buffer->get_insert()->get_iter().get_line() == 4);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 0);
    }
  }

  // extend_selection() tests
  {
    auto buffer = view.get_buffer();
    view.is_bracket_language = true;
    view.spellcheck_all = false;
    std::string source = "test(1, test(10), \"100\");";
    buffer->set_text(source);
    {
      view.place_cursor_at_line_offset(0, 0);
      view.extend_selection();
      g_assert(view.get_selected_text() == "test");

      view.extend_selection();
      g_assert(view.get_selected_text() == "test(1, test(10), \"100\")");

      view.extend_selection();
      g_assert(view.get_selected_text() == source);
    }
    {
      view.place_cursor_at_line_offset(0, 5);
      view.extend_selection();
      g_assert(view.get_selected_text() == "1");

      view.extend_selection();
      g_assert(view.get_selected_text() == "1, test(10), \"100\"");

      view.extend_selection();
      g_assert(view.get_selected_text() == "test(1, test(10), \"100\")");
    }
    {
      view.place_cursor_at_line_offset(0, 7);
      view.extend_selection();
      g_assert(view.get_selected_text() == " test(10)");
    }
    {
      view.place_cursor_at_line_offset(0, 8);
      view.extend_selection();
      g_assert(view.get_selected_text() == "test");

      view.extend_selection();
      g_assert(view.get_selected_text() == "test(10)");

      view.extend_selection();
      g_assert(view.get_selected_text() == " test(10)");

      view.extend_selection();
      g_assert(view.get_selected_text() == "1, test(10), \"100\"");
    }
    {
      view.place_cursor_at_line_offset(0, 18);
      view.extend_selection();
      g_assert(view.get_selected_text() == " \"100\"");

      view.extend_selection();
      g_assert(view.get_selected_text() == "1, test(10), \"100\"");
    }
    {
      view.place_cursor_at_line_offset(0, 26);
      view.extend_selection();
      g_assert(view.get_selected_text() == source);
    }
    {
      view.place_cursor_at_line_offset(0, 27);
      view.extend_selection();
      g_assert(view.get_selected_text() == source);
    }

    source = "int main() {\n  return 1;\n}\n";
    buffer->set_text(source);
    {
      view.place_cursor_at_line_offset(0, 0);
      view.extend_selection();
      g_assert(view.get_selected_text() == "int");

      view.extend_selection();
      g_assert(view.get_selected_text() == source.substr(0, source.size() - 1));

      view.extend_selection();
      g_assert(view.get_selected_text() == source);
    }
    {
      view.place_cursor_at_line_offset(0, 4);
      view.extend_selection();
      g_assert(view.get_selected_text() == "main");

      view.extend_selection();
      g_assert(view.get_selected_text() == source.substr(4, source.size() - 1 - 4));

      view.extend_selection();
      g_assert(view.get_selected_text() == source.substr(0, source.size() - 1));

      view.extend_selection();
      g_assert(view.get_selected_text() == source);
    }
    {
      view.place_cursor_at_line_offset(1, 2);
      view.extend_selection();
      g_assert(view.get_selected_text() == "return");

      view.extend_selection();
      g_assert(view.get_selected_text() == "return 1;");

      view.extend_selection();
      g_assert(view.get_selected_text() == source.substr(12, 13));

      view.extend_selection();
      g_assert(view.get_selected_text() == source.substr(4, source.size() - 1 - 4));

      view.extend_selection();
      g_assert(view.get_selected_text() == source.substr(0, source.size() - 1));

      view.extend_selection();
      g_assert(view.get_selected_text() == source);
    }

    source = "test<int, int>(11, 22);";
    buffer->set_text(source);
    {
      view.place_cursor_at_line_offset(0, 0);
      view.extend_selection();
      g_assert(view.get_selected_text() == "test");

      view.extend_selection();
      g_assert(view.get_selected_text() == source.substr(0, source.size() - 1));

      view.extend_selection();
      g_assert(view.get_selected_text() == source);
    }
    {
      view.place_cursor_at_line_offset(0, 5);
      view.extend_selection();
      g_assert(view.get_selected_text() == "int");

      view.extend_selection();
      g_assert(view.get_selected_text() == source.substr(5, 8));

      view.extend_selection();
      g_assert(view.get_selected_text() == source.substr(0, source.size() - 1));
    }
    {
      view.place_cursor_at_line_offset(0, 15);
      view.extend_selection();
      g_assert(view.get_selected_text() == "11");

      view.extend_selection();
      g_assert(view.get_selected_text() == "11, 22");

      view.extend_selection();
      g_assert(view.get_selected_text() == source.substr(0, source.size() - 1));
    }

    source = "{\n  {\n    test;\n  }\n}\n";
    buffer->set_text(source);
    {
      view.place_cursor_at_line_offset(2, 4);
      view.extend_selection();
      g_assert(view.get_selected_text() == "test");

      view.extend_selection();
      g_assert(view.get_selected_text() == "test;");

      view.extend_selection();
      g_assert(view.get_selected_text() == "\n    test;\n  ");

      view.extend_selection();
      g_assert(view.get_selected_text() == "{\n    test;\n  }");

      view.extend_selection();
      g_assert(view.get_selected_text() == "\n  {\n    test;\n  }\n");

      view.extend_selection();
      g_assert(view.get_selected_text() == "{\n  {\n    test;\n  }\n}");

      view.extend_selection();
      g_assert(view.get_selected_text() == source);

      view.shrink_selection();
      g_assert(view.get_selected_text() == "{\n  {\n    test;\n  }\n}");

      view.shrink_selection();
      g_assert(view.get_selected_text() == "\n  {\n    test;\n  }\n");

      view.shrink_selection();
      g_assert(view.get_selected_text() == "{\n    test;\n  }");
    }

    {
      auto buffer = view.get_buffer();
      view.is_js = true;
      Gtk::TextIter start, end;

      buffer->set_text(R"(render(
  <div></div>
);)");
      view.place_cursor_at_line_offset(1, 7);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 2 && end.get_line() == 1 && end.get_line_offset() == 13);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 0 && start.get_line_offset() == 7 && end.get_line() == 2 && end.get_line_offset() == 0);

      view.place_cursor_at_line_offset(1, 4);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 3 && end.get_line() == 1 && end.get_line_offset() == 6);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 2 && end.get_line() == 1 && end.get_line_offset() == 13);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 0 && start.get_line_offset() == 7 && end.get_line() == 2 && end.get_line_offset() == 0);

      buffer->set_text(R"(render(
  <div id="test">test test</div>
);)");
      view.place_cursor_at_line_offset(1, 18);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 17 && end.get_line() == 1 && end.get_line_offset() == 21);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 17 && end.get_line() == 1 && end.get_line_offset() == 26);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 2 && end.get_line() == 1 && end.get_line_offset() == 32);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 0 && start.get_line_offset() == 7 && end.get_line() == 2 && end.get_line_offset() == 0);

      view.place_cursor_at_line_offset(1, 4);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 3 && end.get_line() == 1 && end.get_line_offset() == 6);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 2 && end.get_line() == 1 && end.get_line_offset() == 32);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 0 && start.get_line_offset() == 7 && end.get_line() == 2 && end.get_line_offset() == 0);

      buffer->set_text(R"(render(
  <div onClick={() => {}}></div>
);)");
      view.place_cursor_at_line_offset(1, 6);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 2 && end.get_line() == 1 && end.get_line_offset() == 32);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 0 && start.get_line_offset() == 7 && end.get_line() == 2 && end.get_line_offset() == 0);

      buffer->set_text(R"(render(
  <div>
    <div></div>
    <div></div>
  </div>
);)");
      view.place_cursor_at_line_offset(2, 8);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 2 && start.get_line_offset() == 4 && end.get_line() == 2 && end.get_line_offset() == 15);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 7 && end.get_line() == 4 && end.get_line_offset() == 2);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 2 && end.get_line() == 4 && end.get_line_offset() == 8);

      view.place_cursor_at_line_offset(3, 8);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 3 && start.get_line_offset() == 4 && end.get_line() == 3 && end.get_line_offset() == 15);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 7 && end.get_line() == 4 && end.get_line_offset() == 2);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 2 && end.get_line() == 4 && end.get_line_offset() == 8);

      buffer->set_text(R"(render(
  <div>
    <div></div>
    <img src="test">
    <Test/>
    <br>
    <div></div>
  </div>
);)");
      view.place_cursor_at_line_offset(2, 8);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 2 && start.get_line_offset() == 4 && end.get_line() == 2 && end.get_line_offset() == 15);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 7 && end.get_line() == 7 && end.get_line_offset() == 2);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 2 && end.get_line() == 7 && end.get_line_offset() == 8);

      view.place_cursor_at_line_offset(6, 8);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 6 && start.get_line_offset() == 4 && end.get_line() == 6 && end.get_line_offset() == 15);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 7 && end.get_line() == 7 && end.get_line_offset() == 2);
      view.extend_selection();
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_line() == 1 && start.get_line_offset() == 2 && end.get_line() == 7 && end.get_line_offset() == 8);

      view.is_js = false;
    }
  }

  // Snippet tests
  {
    auto buffer = view.get_buffer();
    GdkEventKey event;
    event.state = 0;
    {
      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "std::cout << ${1:content} << std::endl;");
      g_assert(buffer->get_text() == "std::cout << content << std::endl;");
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 13);
      g_assert(end.get_offset() == 20);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << t << std::endl;");
      event.keyval = GDK_KEY_e;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << te << std::endl;");
    }
    {
      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "std::cout << ${1:} << std::endl;");
      g_assert(buffer->get_text() == "std::cout <<  << std::endl;");
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 13);
      g_assert(end.get_offset() == 13);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << t << std::endl;");
      event.keyval = GDK_KEY_e;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << te << std::endl;");
    }
    {
      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "std::cout << $1 << std::endl;");
      g_assert(buffer->get_text() == "std::cout <<  << std::endl;");
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 13);
      g_assert(end.get_offset() == 13);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << t << std::endl;");
      event.keyval = GDK_KEY_e;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << te << std::endl;");
    }
    {
      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "std::cout << ${1:content} << ${1:content} << std::endl;");
      g_assert(buffer->get_text() == "std::cout << content << content << std::endl;");
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 13);
      g_assert(end.get_offset() == 20);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << t << t << std::endl;");
      event.keyval = GDK_KEY_e;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << te << te << std::endl;");

      event.keyval = GDK_KEY_Escape;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << te << te << std::endl;");
      event.keyval = GDK_KEY_s;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << tes << te << std::endl;");
    }
    {
      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "std::cout << ${1:content} << ${1:cont} << std::endl;");
      g_assert(buffer->get_text() == "std::cout << content << cont << std::endl;");
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 13);
      g_assert(end.get_offset() == 20);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << t << t << std::endl;");
      event.keyval = GDK_KEY_e;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << te << te << std::endl;");

      event.keyval = GDK_KEY_Escape;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << te << te << std::endl;");
      event.keyval = GDK_KEY_s;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << tes << te << std::endl;");
    }
    {
      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "std::cout << ${1:content} << $1 << std::endl;");
      g_assert(buffer->get_text() == "std::cout << content <<  << std::endl;");
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 13);
      g_assert(end.get_offset() == 20);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << t << t << std::endl;");
      event.keyval = GDK_KEY_e;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << te << te << std::endl;");

      event.keyval = GDK_KEY_Escape;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << te << te << std::endl;");
      event.keyval = GDK_KEY_s;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << tes << te << std::endl;");
    }
    {
      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "std::cout << ${1:content1} << ${2:content2} << std::endl;");
      g_assert(buffer->get_text() == "std::cout << content1 << content2 << std::endl;");
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 13);
      g_assert(end.get_offset() == 21);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << t << content2 << std::endl;");
      event.keyval = GDK_KEY_e;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << te << content2 << std::endl;");

      event.keyval = GDK_KEY_Tab;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << te << content2 << std::endl;");
      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << te << t << std::endl;");
    }
    {
      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "std::cout << ${1:content} << $0 << std::endl;");
      g_assert(buffer->get_text() == "std::cout << content <<  << std::endl;");
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 13);
      g_assert(end.get_offset() == 20);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << t <<  << std::endl;");
      event.keyval = GDK_KEY_e;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << te <<  << std::endl;");

      event.keyval = GDK_KEY_Tab;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << te <<  << std::endl;");
      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << te << t << std::endl;");
    }
    {
      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "std::cout << ${2:content2} << ${1:content1} << std::endl;");
      g_assert(buffer->get_text() == "std::cout << content2 << content1 << std::endl;");
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 25);
      g_assert(end.get_offset() == 33);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << content2 << t << std::endl;");
      event.keyval = GDK_KEY_e;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << content2 << te << std::endl;");

      event.keyval = GDK_KEY_Tab;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << content2 << te << std::endl;");
      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "std::cout << t << te << std::endl;");
    }
    {
      buffer->set_text("test");
      buffer->select_range(buffer->begin(), buffer->end());
      view.insert_snippet(buffer->get_insert()->get_iter(), "<$1>${TM_SELECTED_TEXT}</$1>");
      g_assert(buffer->get_text() == "<>test</>");
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 1);
      g_assert(end.get_offset() == 1);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "<t>test</t>");
      event.keyval = GDK_KEY_e;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "<te>test</te>");
    }
    {
      buffer->set_text("test test");
      Gtk::TextIter start, end;
      start = buffer->begin();
      end = start;
      end.forward_chars(4);
      buffer->select_range(start, end);
      view.insert_snippet(buffer->get_insert()->get_iter(), "<$1>${TM_SELECTED_TEXT:TM_CURRENT_LINE}</$1>");
      g_assert(buffer->get_text() == "<>test</> test");
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 1);
      g_assert(end.get_offset() == 1);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "<t>test</t> test");
      event.keyval = GDK_KEY_e;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "<te>test</te> test");
    }
    {
      buffer->set_text("test test");
      view.insert_snippet(buffer->get_insert()->get_iter(), "<$1>${TM_SELECTED_TEXT:TM_CURRENT_LINE}</$1>");
      g_assert(buffer->get_text() == "<>test test</>");
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 1);
      g_assert(end.get_offset() == 1);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "<t>test test</t>");
      event.keyval = GDK_KEY_e;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "<te>test test</te>");
    }
    {
      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "\\textbf{${TM_SELECTED_TEXT:no text was selected}}");
      g_assert(buffer->get_text() == "\\textbf{no text was selected}");
      g_assert(buffer->get_insert()->get_iter().get_offset() == 29);
    }
    {
      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "<div${1: id=\"${2:some_id}\"}>\n  $0\n</div>");
      auto result = "<div id=\"some_id\">\n  \n</div>";
      g_assert(buffer->get_text() == result);
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 4);
      g_assert(end.get_offset() == 17);

      event.keyval = GDK_KEY_Tab;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == result);
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 9);
      g_assert(end.get_offset() == 16);

      event.keyval = GDK_KEY_Tab;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == result);
      g_assert(buffer->get_insert()->get_iter().get_offset() == 21);
    }
    {
      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "<div${1: id=\"${2:some_id}\"}>\n  $0\n</div>");
      g_assert(buffer->get_text() == "<div id=\"some_id\">\n  \n</div>");
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 4);
      g_assert(end.get_offset() == 17);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "<divt>\n  \n</div>");
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 5);
      g_assert(end.get_offset() == 5);

      event.keyval = GDK_KEY_Tab;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "<divt>\n  \n</div>");
      g_assert(buffer->get_insert()->get_iter().get_offset() == 9);
    }
    {
      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "\\begin{${1:enumerate}}\n  $0\n\\end{$1}");
      g_assert(buffer->get_text() == "\\begin{enumerate}\n  \n\\end{}");
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 7);
      g_assert(end.get_offset() == 16);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "\\begin{t}\n  \n\\end{t}");
      event.keyval = GDK_KEY_e;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "\\begin{te}\n  \n\\end{te}");

      event.keyval = GDK_KEY_Tab;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "\\begin{te}\n  \n\\end{te}");
      g_assert(buffer->get_insert()->get_iter().get_offset() == 13);
    }
    {
      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "test$test");
      g_assert(buffer->get_text() == "test$test");

      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "${TM_SELECTED_TEXT}");
      g_assert(buffer->get_text() == "");

      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "$TM_SELECTED_TEXT");
      g_assert(buffer->get_text() == "");

      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "\\$");
      g_assert(buffer->get_text() == "$");

      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "\\\\${0:test}");
      g_assert(buffer->get_text() == "\\test");

      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "te{s}t");
      g_assert(buffer->get_text() == "te{s}t");

      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "${0:te{s\\}t}");
      g_assert(buffer->get_text() == "te{s}t");

      buffer->set_text("");
      view.insert_snippet(buffer->get_insert()->get_iter(), "\\test\\");
      g_assert(buffer->get_text() == "\\test\\");
    }
    {
      Config::get().source.enable_multiple_cursors = true;
      buffer->set_text("\n");
      buffer->place_cursor(buffer->begin());
      event.keyval = GDK_KEY_Down;
      event.state = GDK_MOD1_MASK;
      view.on_key_press_event(&event);
      event.state = 0;

      view.enable_multiple_cursors = true;
      view.insert_snippet(buffer->get_insert()->get_iter(), "\\begin{${1:enumerate}}\n  $0\n\\end{${1:enum}}");
      g_assert(buffer->get_text() == "\\begin{enumerate}\n  \n\\end{enum}\n\\begin{enumerate}\n  \n\\end{enum}");
      Gtk::TextIter start, end;
      buffer->get_selection_bounds(start, end);
      g_assert(start.get_offset() == 7);
      g_assert(end.get_offset() == 16);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "\\begin{t}\n  \n\\end{t}\n\\begin{t}\n  \n\\end{t}");
      event.keyval = GDK_KEY_e;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "\\begin{te}\n  \n\\end{te}\n\\begin{te}\n  \n\\end{te}");

      event.keyval = GDK_KEY_Tab;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "\\begin{te}\n  \n\\end{te}\n\\begin{te}\n  \n\\end{te}");
      g_assert(buffer->get_insert()->get_iter().get_offset() == 13);

      event.keyval = GDK_KEY_t;
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "\\begin{te}\n  t\n\\end{te}\n\\begin{te}\n  t\n\\end{te}");
    }
  }
}
