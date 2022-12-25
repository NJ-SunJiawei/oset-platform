#include "config.hpp"
#include "source.hpp"
#include <glib.h>

// Requires display server to work
// However, it is possible to use the Broadway backend if the test is run in a pure terminal environment:
// broadwayd&
// make test

int main() {
  auto app = Gtk::Application::create();
  Gsv::init();

  auto tests_path = boost::filesystem::canonical(JUCI_TESTS_PATH);
  auto source_file = tests_path / "tmp" / "source_file.cpp";

  auto language_manager = Source::LanguageManager::get_default();
  GdkEventKey event;
  event.state = 0;

  {
    auto language = language_manager->get_language("js");
    Source::View view(source_file, language);
    view.get_source_buffer()->set_highlight_syntax(true);
    view.set_tab_char_and_size(' ', 2);
    auto buffer = view.get_buffer();
    {
      buffer->set_text("  ''\n");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_chars(3);
      assert(view.is_spellcheck_iter(iter));
      assert(view.is_code_iter(iter));
    }
    {
      buffer->set_text("  \"\"\n");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_chars(3);
      assert(view.is_spellcheck_iter(iter));
      assert(view.is_code_iter(iter));
    }
    {
      buffer->set_text("  'test\n");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      assert(view.is_spellcheck_iter(iter));
      assert(!view.is_code_iter(iter));
    }
    {
      buffer->set_text("  \"test'\n");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      iter.backward_char();
      assert(view.is_spellcheck_iter(iter));
      assert(!view.is_code_iter(iter));
      iter.forward_char();
      assert(view.is_spellcheck_iter(iter));
      assert(!view.is_code_iter(iter));
    }
    {
      buffer->set_text("  'test'\n");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_chars(2);
      assert(!view.is_spellcheck_iter(iter));
      assert(view.is_code_iter(iter));
      iter.forward_char();
      assert(view.is_spellcheck_iter(iter));
      assert(!view.is_code_iter(iter));
      iter.forward_chars(4);
      assert(view.is_spellcheck_iter(iter));
      assert(view.is_code_iter(iter));
      iter.forward_char();
      assert(!view.is_spellcheck_iter(iter));
      assert(view.is_code_iter(iter));
    }
    {
      buffer->set_text("  \"test\"\n");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_chars(2);
      assert(!view.is_spellcheck_iter(iter));
      assert(view.is_code_iter(iter));
      iter.forward_char();
      assert(view.is_spellcheck_iter(iter));
      assert(!view.is_code_iter(iter));
      iter.forward_chars(4);
      assert(view.is_spellcheck_iter(iter));
      assert(view.is_code_iter(iter));
      iter.forward_char();
      assert(!view.is_spellcheck_iter(iter));
      assert(view.is_code_iter(iter));
    }
    {
      buffer->set_text("  '\\''\n");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_chars(2);
      assert(!view.is_spellcheck_iter(iter));
      assert(view.is_code_iter(iter));
      iter.forward_char();
      assert(view.is_spellcheck_iter(iter));
      assert(!view.is_code_iter(iter));
      iter.forward_char();
      assert(view.is_spellcheck_iter(iter));
      assert(!view.is_code_iter(iter));
      iter.forward_char();
      assert(view.is_spellcheck_iter(iter));
      assert(view.is_code_iter(iter));
      iter.forward_char();
      assert(!view.is_spellcheck_iter(iter));
      assert(view.is_code_iter(iter));
    }
    {
      buffer->set_text("  /**/\n");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_chars(2);
      assert(!view.is_spellcheck_iter(iter));
      assert(!view.is_code_iter(iter));
      iter.forward_char();
      assert(view.is_spellcheck_iter(iter));
      assert(!view.is_code_iter(iter));
      iter.forward_char();
      assert(view.is_spellcheck_iter(iter));
      assert(!view.is_code_iter(iter));
      iter.forward_char();
      assert(view.is_spellcheck_iter(iter));
      assert(!view.is_code_iter(iter));
      iter.forward_char();
      assert(!view.is_spellcheck_iter(iter));
      assert(view.is_code_iter(iter));
    }
    {
      buffer->set_text("  //t\n");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_chars(2);
      assert(!view.is_spellcheck_iter(iter));
      assert(!view.is_code_iter(iter));
      iter.forward_char();
      assert(view.is_spellcheck_iter(iter));
      assert(!view.is_code_iter(iter));
      iter.forward_char();
      assert(view.is_spellcheck_iter(iter));
      assert(!view.is_code_iter(iter));
      iter.forward_char();
      assert(view.is_spellcheck_iter(iter));
      assert(!view.is_code_iter(iter));
    }
  }

  {
    auto language = language_manager->get_language("cpp");
    Source::View view(source_file, language);
    view.get_source_buffer()->set_highlight_syntax(true);
    view.set_tab_char_and_size(' ', 2);
    auto buffer = view.get_buffer();
    event.keyval = GDK_KEY_Return;

    {
      buffer->set_text("{");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "{\n"
                                     "  \n"
                                     "}");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 2);
    }
    {
      buffer->set_text("{\n"
                       "  \n"
                       "}");
      auto iter = buffer->begin();
      iter.forward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "{\n"
                                     "  \n"
                                     "  \n"
                                     "}");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 2);
    }
    {
      buffer->set_text("{\n"
                       "  \n"
                       "}");
      auto iter = buffer->get_iter_at_line(2);
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "{\n"
                                     "  \n"
                                     "  \n"
                                     "}");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 2);
    }
    {
      buffer->set_text("{\n"
                       "  {\n"
                       "}");
      auto iter = buffer->get_iter_at_line(2);
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "{\n"
                                     "  {\n"
                                     "    \n"
                                     "  }\n"
                                     "}");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("{\n"
                       "  {\n"
                       "    \n"
                       "  }\n"
                       "}");
      auto iter = buffer->get_iter_at_line(2);
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "{\n"
                                     "  {\n"
                                     "    \n"
                                     "    \n"
                                     "  }\n"
                                     "}");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("{\n"
                       "{\n"
                       "}");
      auto iter = buffer->get_iter_at_line(1);
      iter.forward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "{\n"
                                     "{\n"
                                     "  \n"
                                     "}");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 2);
    }
    {
      buffer->set_text("{\n"
                       "{\n"
                       "}\n"
                       "}");
      auto iter = buffer->get_iter_at_line(1);
      iter.forward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "{\n"
                                     "{\n"
                                     "  \n"
                                     "}\n"
                                     "}");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 2);
    }
    {
      buffer->set_text("namespace test {\n"
                       "{\n"
                       "}");
      auto iter = buffer->get_iter_at_line(1);
      iter.forward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "namespace test {\n"
                                     "{\n"
                                     "  \n"
                                     "}\n"
                                     "}");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 2);
    }
    {
      buffer->set_text("namespace test {\n"
                       "{\n"
                       "}\n");
      auto iter = buffer->get_iter_at_line(1);
      iter.forward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "namespace test {\n"
                                     "{\n"
                                     "  \n"
                                     "}\n"
                                     "}\n");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 2);
    }
    {
      buffer->set_text("namespace test {\n"
                       "{\n"
                       "}\n"
                       "}\n");
      auto iter = buffer->get_iter_at_line(1);
      iter.forward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "namespace test {\n"
                                     "{\n"
                                     "  \n"
                                     "}\n"
                                     "}\n");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 2);
    }
    {
      buffer->set_text("{\n"
                       "{\n"
                       "  \n"
                       "}\n"
                       "}");
      auto iter = buffer->get_iter_at_line(1);
      iter.forward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "{\n"
                                     "{\n"
                                     "  \n"
                                     "  \n"
                                     "}\n"
                                     "}");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 2);
    }
    {
      buffer->set_text("{\n"
                       "{\n"
                       "  \n"
                       "}\n"
                       "}");
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "{\n"
                                     "  \n"
                                     "{\n"
                                     "  \n"
                                     "}\n"
                                     "}");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 2);
    }
    {
      buffer->set_text("namespace test {\n"
                       "{\n"
                       "  \n"
                       "}\n"
                       "}");
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "namespace test {\n"
                                     "  \n"
                                     "{\n"
                                     "  \n"
                                     "}\n"
                                     "}");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 2);
    }
    {
      buffer->set_text("namespace test {\n"
                       "class C {\n"
                       "  \n"
                       "}\n"
                       "}");
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "namespace test {\n"
                                     "  \n"
                                     "class C {\n"
                                     "  \n"
                                     "}\n"
                                     "}");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 2);
    }
    {
      buffer->set_text("namespace test {\n"
                       "{\n"
                       "  \n"
                       "}\n");
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "namespace test {\n"
                                     "  \n"
                                     "}\n"
                                     "{\n"
                                     "  \n"
                                     "}\n");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 2);
    }
    {
      buffer->set_text("namespace test {\n"
                       "class C {\n"
                       "  \n"
                       "}\n");
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "namespace test {\n"
                                     "  \n"
                                     "}\n"
                                     "class C {\n"
                                     "  \n"
                                     "}\n");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 2);
    }

    {
      buffer->set_text("  int main() {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main() {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  int main() {return 0;");
      auto iter = buffer->begin();
      iter.forward_chars(14);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main() {\n"
                                     "    return 0;\n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  int main() { return 0;");
      auto iter = buffer->begin();
      iter.forward_chars(14);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main() {\n"
                                     "    return 0;\n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  int main() {//comment");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main() {//comment\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  int main() {  ");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main() {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  int main() {\n"
                       "  }");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_chars(4);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main() {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  int main() {//comment\n"
                       "  }");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_chars(4);
      buffer->place_cursor(iter);
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main() {//comment\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  int main() {}");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_chars(1);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main() {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  int main()\n"
                       "  {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main()\n"
                                     "  {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  int main()\n"
                       "  {\n"
                       "  }");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_chars(4);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main()\n"
                                     "  {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  int main()\n"
                       "  {}");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_chars(1);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main()\n"
                                     "  {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  int main()\n"
                       "  {/*comment*/}");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_chars(1);
      buffer->place_cursor(iter);
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main()\n"
                                     "  {/*comment*/\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }

    {
      buffer->set_text("  enum class A { a,\n"
                       "                 b };");
      auto iter = buffer->begin();
      iter.forward_chars(16);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  enum class A {\n"
                                     "                 a,\n"
                                     "                 b };");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 17);
    }
    {
      buffer->set_text("  test(2,\n"
                       "       []() {\n"
                       "         func();\n"
                       "       })\n");
      auto iter = buffer->get_iter_at_line_offset(1, 13);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(2,\n"
                                     "       []() {\n"
                                     "         \n"
                                     "         func();\n"
                                     "       })\n");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 9);
    }

    {
      buffer->set_text("  else");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  else\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  else // comment");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  else // comment\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  else;");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  else;\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  else;//comment");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  else;//comment\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  else {}");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  else {}\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  else {}//comment");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  else {}//comment\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }

    {
      buffer->set_text("  } else if(true)");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  } else if(true)\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  else if constexpr(true)");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  else if constexpr(true)\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  } else if(true)//comment");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  } else if(true)//comment\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  } else if(true)\n"
                       "    ;");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  } else if(true)\n"
                                     "    ;\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  } else if(true)//comment\n"
                       "    ;//comment");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  } else if(true)//comment\n"
                                     "    ;//comment\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }

    {
      buffer->set_text("  if(true) {\n"
                       "    ;");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true) {\n"
                                     "    ;\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  if(true) { /*comment*/\n"
                       "    ;");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true) { /*comment*/\n"
                                     "    ;\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }

    {
      buffer->set_text("  if(true &&\n"
                       "     false");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true &&\n"
                                     "     false\n"
                                     "     ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  if(true &&\n"
                       "     false)");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true &&\n"
                                     "     false)\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  if constexpr(true &&\n"
                       "     false)");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if constexpr(true &&\n"
                                     "     false)\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  if(true &&\n"
                       "     false) {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true &&\n"
                                     "     false) {\n"
                                     "    \n"
                                     "  }");
      auto iter = buffer->end();
      iter.backward_chars(4);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  if(true && // comment\n"
                       "     false) { // comment");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true && // comment\n"
                                     "     false) { // comment\n"
                                     "    \n"
                                     "  }");
      auto iter = buffer->end();
      iter.backward_chars(4);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  if(true &&\n"
                       "     false)\n"
                       "    ;");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true &&\n"
                                     "     false)\n"
                                     "    ;\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  if(true &&\n"
                       "     false)//comment\n"
                       "    ;//comment");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true &&\n"
                                     "     false)//comment\n"
                                     "    ;//comment\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  if(true &&\n"
                       "     false)\n"
                       "    cout << endl <<\n"
                       "         << endl <<");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true &&\n"
                                     "     false)\n"
                                     "    cout << endl <<\n"
                                     "         << endl <<\n"
                                     "         ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  if(true &&\n"
                       "     false)\n"
                       "    cout << endl <<\n"
                       "         << endl;");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true &&\n"
                                     "     false)\n"
                                     "    cout << endl <<\n"
                                     "         << endl;\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }

    {
      buffer->set_text("  int a = 2 +\n"
                       "    2;");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int a = 2 +\n"
                                     "    2;\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  int a = 2 /\n"
                       "    2;");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int a = 2 /\n"
                                     "    2;\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  int a = 2 + // test\n"
                       "    2;");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int a = 2 + // test\n"
                                     "    2;\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  int a = 2;\n"
                       "    2;");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int a = 2;\n"
                                     "    2;\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  int a = 2 #\n"
                       "    2;");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int a = 2 #\n"
                                     "    2;\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }

    {
      buffer->set_text("  func([");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([\n"
                                     "        ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  func([] {},");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([] {},\n"
                                     "       ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  func([]() -> std::vector<std::vector<int>> {},");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([]() -> std::vector<std::vector<int>> {},\n"
                                     "       ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  func([] {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([] {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  func([]() -> std::vector<std::vector<int>> {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([]() -> std::vector<std::vector<int>> {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  func([] {)");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([] {\n"
                                     "    \n"
                                     "  })");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  func([] {})");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_chars(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([] {\n"
                                     "    \n"
                                     "  })");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  func([]->std::vector<std::vector<int>>{)");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([]->std::vector<std::vector<int>>{\n"
                                     "    \n"
                                     "  })");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  func([] {\n"
                       "  })");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_chars(5);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([] {\n"
                                     "    \n"
                                     "  })");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  func([]()->std::vector<std::vector<int>> {\n"
                       "  })");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_chars(5);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([]()->std::vector<std::vector<int>> {\n"
                                     "    \n"
                                     "  })");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  func([]->bool{return true;\n"
                       "  });");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_chars(18);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([]->bool{\n"
                                     "    return true;\n"
                                     "  });");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  func([] {}, [] {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([] {}, [] {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  func([]()->bool {}, []()->bool {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([]()->bool {}, []()->bool {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  func([] {}, [] {},");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([] {}, [] {},\n"
                                     "       ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  func([]()->bool {}, []()->bool {},");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([]()->bool {}, []()->bool {},\n"
                                     "       ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  func([] {}, [] {}");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([] {}, [] {}\n"
                                     "       ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  func([]->bool {}, []->bool {}");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([]->bool {}, []->bool {}\n"
                                     "       ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  func([] {}, [] {}) {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([] {}, [] {}) {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  func([]->bool {}, []->bool {}) {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([]->bool {}, []->bool {}) {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  func([] {\n"
                       "    \n"
                       "  }, [] {\n"
                       "    \n"
                       "  }, {);");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_chars(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([] {\n"
                                     "    \n"
                                     "  }, [] {\n"
                                     "    \n"
                                     "  }, {\n"
                                     "    \n"
                                     "  });");
      g_assert(buffer->get_insert()->get_iter().get_line() == 5);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  func([]() -> std::vector<std::vector<int>> {\n"
                       "    return std::vector<std::vector<int>>();\n"
                       "  }, []() -> std::vector<std::vector<int>> {\n"
                       "    return std::vector<std::vector<int>>();\n"
                       "  }, {);");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_chars(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  func([]() -> std::vector<std::vector<int>> {\n"
                                     "    return std::vector<std::vector<int>>();\n"
                                     "  }, []() -> std::vector<std::vector<int>> {\n"
                                     "    return std::vector<std::vector<int>>();\n"
                                     "  }, {\n"
                                     "    \n"
                                     "  });");
      g_assert(buffer->get_insert()->get_iter().get_line() == 5);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }

    {
      buffer->set_text("  auto func=[] {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  auto func=[] {\n"
                                     "    \n"
                                     "  };");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  auto func=[] {//comment");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  auto func=[] {//comment\n"
                                     "    \n"
                                     "  };");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  test(\n"
                       "      [] {});");
      auto iter = buffer->end();
      iter.backward_chars(3);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "      [] {\n"
                                     "        \n"
                                     "      });");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 8);
    }

    {
      buffer->set_text("  void Class::Class()\n"
                       "      : var(1) {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  void Class::Class()\n"
                                     "      : var(1) {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  class Class {\n"
                       "    void Class():");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  class Class {\n"
                                     "    void Class():\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  void Class::Class() :");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  void Class::Class() :\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 2);
    }
    {
      buffer->set_text("  void Class::Class() :\n"
                       "      var(1) {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  void Class::Class() :\n"
                                     "      var(1) {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  class Test {\n"
                       "  public:\n"
                       "    ;");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  class Test {\n"
                                     "  public:\n"
                                     "    ;\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter().get_line() == 3);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  class Test {\n"
                       "  public:\n"
                       "    void test() {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  class Test {\n"
                                     "  public:\n"
                                     "    void test() {\n"
                                     "      \n"
                                     "    }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 3);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 6);
    }
    {
      buffer->set_text("  void Class::Class(int a,\n"
                       "                    int b) {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  void Class::Class(int a,\n"
                                     "                    int b) {\n"
                                     "    \n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 2);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }

    {
      buffer->set_text("  class Class : BaseClass {\n"
                       "  public:");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  class Class : BaseClass {\n"
                                     "  public:\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  class Class : BaseClass {\n"
                       "    public:");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  class Class : BaseClass {\n"
                                     "  public:\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  class Class : BaseClass {\n"
                       "    public://comment");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  class Class : BaseClass {\n"
                                     "  public://comment\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  class Class : BaseClass {\n"
                       "    int a;\n"
                       "  public:");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  class Class : BaseClass {\n"
                                     "    int a;\n"
                                     "  public:\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  class Class : BaseClass {\n"
                       "    int a;\n"
                       "    public:");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  class Class : BaseClass {\n"
                                     "    int a;\n"
                                     "  public:\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  class Class {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  class Class {\n"
                                     "    \n"
                                     "  };");
      auto iter = buffer->end();
      iter.backward_chars(5);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  class Class {}");
      auto iter = buffer->end();
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  class Class {\n"
                                     "    \n"
                                     "  };");
      iter = buffer->end();
      iter.backward_chars(5);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  struct Struct {};");
      auto iter = buffer->end();
      iter.backward_chars(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  struct Struct {\n"
                                     "    \n"
                                     "  };");
      iter = buffer->end();
      iter.backward_chars(5);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  auto f = []() {");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  auto f = []() {\n"
                                     "    \n"
                                     "  };");
      auto iter = buffer->end();
      iter.backward_chars(5);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  auto f = []() {}");
      auto iter = buffer->end();
      iter.backward_chars(1);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  auto f = []() {\n"
                                     "    \n"
                                     "  };");
      iter = buffer->end();
      iter.backward_chars(5);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }

    {
      buffer->set_text("  /*");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  /*\n"
                                     "   * ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  /*\n"
                       "   *");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  /*\n"
                                     "   *\n"
                                     "   * ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  /*\n"
                       "   */");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  /*\n"
                                     "   */\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("   //comment");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "   //comment\n"
                                     "   ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("   //comment\n"
                       "   //comment");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "   //comment\n"
                                     "   //comment\n"
                                     "   ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("#test\n"
                       "  test();");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "#test\n"
                                     "  \n"
                                     "  test();");
      iter = buffer->get_iter_at_line(1);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  #test\n"
                       "    test();");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  #test\n"
                                     "    \n"
                                     "    test();");
      iter = buffer->get_iter_at_line(1);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }

    {
      buffer->set_text("if('a'=='a')");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_char();
      buffer->place_cursor(iter);
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "if('a'=='a'\n"
                                     "   )");
      iter = buffer->end();
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }


    event.keyval = GDK_KEY_braceleft;
    {
      buffer->set_text("  int main()\n"
                       "  ");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main()\n"
                                     "  {");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  else if(true)\n"
                       "    ");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  else if(true)\n"
                                     "  {");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  else if(true)//comment\n"
                       "    ");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  else if(true)//comment\n"
                                     "  {");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  if(true)\n"
                       "  ");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true)\n"
                                     "  {");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }

    Config::get().source.smart_inserts = true;
    Config::get().source.smart_brackets = true;
    {
      buffer->set_text("  ");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  {}");
      auto iter = buffer->end();
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  {}");
      auto iter = buffer->end();
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  {{}}");
      iter = buffer->end();
      iter.backward_chars(2);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      {
        buffer->set_text("  {{}");
        auto iter = buffer->begin();
        iter.forward_chars(4);
        buffer->place_cursor(iter);
        view.on_key_press_event(&event);
        g_assert(buffer->get_text() == "  {{{}}");
        iter = buffer->begin();
        iter.forward_chars(5);
        g_assert(buffer->get_insert()->get_iter() == iter);
      }
      {
        event.keyval = GDK_KEY_braceright;
        buffer->set_text("  {}}");
        auto iter = buffer->begin();
        iter.forward_chars(3);
        buffer->place_cursor(iter);
        view.on_key_press_event(&event);
        g_assert(buffer->get_text() == "  {}}");
        iter = buffer->begin();
        iter.forward_chars(4);
        g_assert(buffer->get_insert()->get_iter() == iter);
      }
      {
        event.keyval = GDK_KEY_bracketleft;
        buffer->set_text("  [[]");
        auto iter = buffer->begin();
        iter.forward_chars(4);
        buffer->place_cursor(iter);
        view.on_key_press_event(&event);
        g_assert(buffer->get_text() == "  [[[]]");
        iter = buffer->begin();
        iter.forward_chars(5);
        g_assert(buffer->get_insert()->get_iter() == iter);
      }
      {
        event.keyval = GDK_KEY_bracketright;
        buffer->set_text("  []]");
        auto iter = buffer->begin();
        iter.forward_chars(3);
        buffer->place_cursor(iter);
        view.on_key_press_event(&event);
        g_assert(buffer->get_text() == "  []]");
        iter = buffer->begin();
        iter.forward_chars(4);
        g_assert(buffer->get_insert()->get_iter() == iter);
      }
      {
        event.keyval = GDK_KEY_parenleft;
        buffer->set_text("  (()");
        auto iter = buffer->begin();
        iter.forward_chars(4);
        buffer->place_cursor(iter);
        view.on_key_press_event(&event);
        g_assert(buffer->get_text() == "  ((())");
        iter = buffer->begin();
        iter.forward_chars(5);
        g_assert(buffer->get_insert()->get_iter() == iter);
      }
      {
        event.keyval = GDK_KEY_parenright;
        buffer->set_text("  ())");
        auto iter = buffer->begin();
        iter.forward_chars(3);
        buffer->place_cursor(iter);
        view.on_key_press_event(&event);
        g_assert(buffer->get_text() == "  ())");
        iter = buffer->begin();
        iter.forward_chars(4);
        g_assert(buffer->get_insert()->get_iter() == iter);
      }
      event.keyval = GDK_KEY_braceleft;
    }
    {
      buffer->set_text("  {}}");
      auto iter = buffer->end();
      iter.backward_chars(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  {{}}");
      iter = buffer->end();
      iter.backward_chars(2);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  {} //}");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->begin();
      iter.forward_chars(3);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  {{}} //}");
      iter = buffer->begin();
      iter.forward_chars(4);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("namespace test {\n"
                       "\n"
                       "}");
      auto iter = buffer->get_iter_at_line(1);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "namespace test {\n"
                                     "{}\n"
                                     "}");
      iter = buffer->get_iter_at_line(1);
      iter.forward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("namespace test {\n"
                       "\n"
                       "}\n"
                       "}");
      auto iter = buffer->get_iter_at_line(1);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "namespace test {\n"
                                     "{\n"
                                     "}\n"
                                     "}");
      iter = buffer->get_iter_at_line(1);
      iter.forward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("{\n"
                       "}\n"
                       "}");
      auto iter = buffer->get_iter_at_line(1);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "{\n"
                                     "{}\n"
                                     "}");
      iter = buffer->get_iter_at_line(1);
      iter.forward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("{\n"
                       "  \n"
                       "}");
      auto iter = buffer->get_iter_at_line(1);
      iter.forward_chars(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "{\n"
                                     "  {}\n"
                                     "}");
      iter = buffer->get_iter_at_line(1);
      iter.forward_chars(3);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("{\n"
                       "  }\n"
                       "}");
      auto iter = buffer->get_iter_at_line(1);
      iter.forward_chars(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "{\n"
                                     "  {}\n"
                                     "}");
      iter = buffer->get_iter_at_line(1);
      iter.forward_chars(3);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  int main()\n"
                       "  ");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main()\n"
                                     "  {}");
      auto iter = buffer->end();
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  else if(true)\n"
                       "    ");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  else if(true)\n"
                                     "  {}");
      auto iter = buffer->end();
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  else if(true)//comment\n"
                       "    ");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  else if(true)//comment\n"
                                     "  {}");
      auto iter = buffer->end();
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  if(true)\n"
                       "  ");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true)\n"
                                     "  {}");
      auto iter = buffer->end();
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  if(true) \n"
                       "    ;");
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true) {\n"
                                     "    ;");
      iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  if(true) \n"
                       "  ;");
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true) {}\n"
                                     "  ;");
      iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  if(true &&\n"
                       "     false) \n"
                       "  ;");
      auto iter = buffer->get_iter_at_line(1);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true &&\n"
                                     "     false) {}\n"
                                     "  ;");
      iter = buffer->get_iter_at_line(1);
      iter.forward_to_line_end();
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  if(true &&\n"
                       "     false) \n"
                       "    ;");
      auto iter = buffer->get_iter_at_line(1);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true &&\n"
                                     "     false) {\n"
                                     "    ;");
      iter = buffer->get_iter_at_line(1);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  if(true)  // test\n"
                       "  ;");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_chars(11);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true) {} // test\n"
                                     "  ;");
      iter = buffer->get_iter_at_line(0);
      iter.forward_chars(12);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  if(true)  // test\n"
                       "    ;");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_chars(11);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(true) { // test\n"
                                     "    ;");
      iter = buffer->get_iter_at_line(0);
      iter.forward_chars(12);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  a= if(true) {\n"
                       "    ;");
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_chars(4);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  a={} if(true) {\n"
                                     "    ;");
      iter = buffer->get_iter_at_line(0);
      iter.forward_chars(5);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  if(a==) {\n"
                       "    ;");
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_chars(8);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if(a=={}) {\n"
                                     "    ;");
      iter = buffer->get_iter_at_line(0);
      iter.forward_chars(9);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("namespace test \n"
                       "{\n"
                       "  \n"
                       "}\n"
                       "}");
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "namespace test {\n"
                                     "{\n"
                                     "  \n"
                                     "}\n"
                                     "}");
      iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("namespace test \n"
                       "class C {\n"
                       "  \n"
                       "}\n"
                       "}");
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "namespace test {\n"
                                     "class C {\n"
                                     "  \n"
                                     "}\n"
                                     "}");
      iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("namespace test \n"
                       "{\n"
                       "  \n"
                       "}\n");
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "namespace test {}\n"
                                     "{\n"
                                     "  \n"
                                     "}\n");
      iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("namespace test \n"
                       "class C {\n"
                       "  \n"
                       "}\n");
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "namespace test {}\n"
                                     "class C {\n"
                                     "  \n"
                                     "}\n");
      iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    Config::get().source.smart_inserts = false;
    Config::get().source.smart_brackets = false;


    event.keyval = GDK_KEY_braceright;
    {
      buffer->set_text("  int main() {\n"
                       "    ");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main() {\n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  int main() {//comment\n"
                       "    ");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  int main() {//comment\n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }

    Config::get().source.smart_inserts = true;
    {
      buffer->set_text("  {}");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  {}");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    Config::get().source.smart_inserts = false;


    event.keyval = GDK_KEY_BackSpace;
    {
      buffer->set_text("  int main()\n");
      auto iter = buffer->begin();
      iter.forward_chars(2);
      buffer->place_cursor(iter);
      assert(view.on_key_press_event_basic(&event) == false);
      assert(view.on_key_press_event_bracket_language(&event) == false);
    }
    {
      buffer->set_text("  int main()");
      auto iter = buffer->begin();
      iter.forward_chars(2);
      buffer->place_cursor(iter);
      assert(view.on_key_press_event_basic(&event) == false);
      assert(view.on_key_press_event_bracket_language(&event) == false);
    }

    Config::get().source.smart_inserts = true;
    {
      buffer->set_text("  {}");
      auto iter = buffer->get_insert()->get_iter();
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  {}}");
      auto iter = buffer->end();
      iter.backward_chars(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  }");
      iter = buffer->end();
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  {{}");
      auto iter = buffer->end();
      iter.backward_char();
      buffer->place_cursor(iter);
      assert(view.on_key_press_event_smart_brackets(&event) == false);
      assert(view.on_key_press_event_smart_inserts(&event) == false);
      assert(view.on_key_press_event_bracket_language(&event) == false);
      assert(view.on_key_press_event_basic(&event) == false);
    }
    {
      buffer->set_text("  ())");
      auto iter = buffer->end();
      iter.backward_chars(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  )");
      iter = buffer->end();
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  []]");
      auto iter = buffer->end();
      iter.backward_chars(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  ]");
      iter = buffer->end();
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("class C {\n"
                       "  {}\n"
                       "public:\n"
                       "}\n");
      auto iter = buffer->get_iter_at_line(1);
      iter.forward_to_line_end();
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "class C {\n"
                                     "  \n"
                                     "public:\n"
                                     "}\n");
      iter = buffer->get_iter_at_line(1);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    Config::get().source.smart_inserts = false;


    event.keyval = GDK_KEY_semicolon;
    Config::get().source.smart_inserts = true;
    {
      buffer->set_text("test()");
      auto iter = buffer->end();
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "test();");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text(")test()");
      auto iter = buffer->end();
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == ")test();");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("(test()");
      auto iter = buffer->end();
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "(test(;)");
      iter = buffer->end();
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    Config::get().source.smart_inserts = false;


    event.keyval = GDK_KEY_Tab;
    {
      buffer->set_text("test\n"
                       "\n"
                       "test");
      auto iter = buffer->get_iter_at_line(1);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "test\n"
                                     "  \n"
                                     "test");
      iter = buffer->get_iter_at_line(1);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test\n"
                       "\n"
                       "    test");
      auto iter = buffer->get_iter_at_line(1);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test\n"
                                     "    \n"
                                     "    test");
      iter = buffer->get_iter_at_line(1);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("    test\n"
                       "\n"
                       "  test");
      auto iter = buffer->get_iter_at_line(1);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "    test\n"
                                     "    \n"
                                     "  test");
      iter = buffer->get_iter_at_line(1);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("    test\n"
                       "\n"
                       "\n"
                       "\n"
                       "  test");
      auto iter = buffer->get_iter_at_line(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "    test\n"
                                     "\n"
                                     "    \n"
                                     "\n"
                                     "  test");
      iter = buffer->get_iter_at_line(2);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(true,\n"
                       "       false) {\n"
                       "\n"
                       "    test");
      auto iter = buffer->get_iter_at_line(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(true,\n"
                                     "       false) {\n"
                                     "    \n"
                                     "    test");
      iter = buffer->get_iter_at_line(2);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(true,\n"
                       "       false) { //test\n"
                       "\n"
                       "    test");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(true,\n"
                                     "       false) { //test\n"
                                     "    \n"
                                     "    test");
      iter = buffer->get_iter_at_line(2);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(true,\n"
                       "       false)\n"
                       "\n"
                       "    test");
      auto iter = buffer->get_iter_at_line(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(true,\n"
                                     "       false)\n"
                                     "    \n"
                                     "    test");
      iter = buffer->get_iter_at_line(2);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(true,\n"
                       "       false) //test\n"
                       "\n"
                       "    test");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(true,\n"
                                     "       false) //test\n"
                                     "    \n"
                                     "    test");
      iter = buffer->get_iter_at_line(2);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("    if(true)\n"
                       "      ;\n"
                       "");
      auto iter = buffer->get_iter_at_line(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "    if(true)\n"
                                     "      ;\n"
                                     "    ");
      iter = buffer->get_iter_at_line(2);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("    if(true)\n"
                       "      ;\n"
                       "\n"
                       "\n");
      auto iter = buffer->get_iter_at_line(3);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "    if(true)\n"
                                     "      ;\n"
                                     "\n"
                                     "    \n");
      iter = buffer->get_iter_at_line(3);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      view.set_tab_char_and_size('\t', 1);
      buffer->set_text("\t\tif(true)\n"
                       "\t\t\t;\n"
                       "");
      auto iter = buffer->get_iter_at_line(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "\t\tif(true)\n"
                                     "\t\t\t;\n"
                                     "\t\t");
      iter = buffer->get_iter_at_line(2);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
      view.set_tab_char_and_size(' ', 2);
    }
    {
      buffer->set_text("    if(true) /*test*/\n"
                       "      ;\n"
                       "");
      auto iter = buffer->get_iter_at_line(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "    if(true) /*test*/\n"
                                     "      ;\n"
                                     "    ");
      iter = buffer->get_iter_at_line(2);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
  }


  {
    auto language = language_manager->get_language("python");
    Source::View view(source_file, language);
    view.get_source_buffer()->set_highlight_syntax(true);
    view.set_tab_char_and_size(' ', 2);
    auto buffer = view.get_buffer();
    event.keyval = GDK_KEY_Return;
    {
      buffer->set_text("  if True:");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  if True:\n"
                                     "    ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
  }


  {
    auto language = language_manager->get_language("js");
    Source::View view(source_file, language);
    view.get_source_buffer()->set_highlight_syntax(true);
    view.set_tab_char_and_size(' ', 2);
    auto buffer = view.get_buffer();
    event.keyval = GDK_KEY_Return;
    {
      buffer->set_text("  (");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  (\n"
                                     "    \n"
                                     "  )");
      auto iter = buffer->end();
      iter.backward_chars(4);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  [");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  [\n"
                                     "    \n"
                                     "  ]");
      auto iter = buffer->end();
      iter.backward_chars(4);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  []");
      auto iter = buffer->end();
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  [\n"
                                     "    \n"
                                     "  ]");
      iter = buffer->end();
      iter.backward_chars(4);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test []");
      auto iter = buffer->end();
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test [\n"
                                     "    \n"
                                     "  ]");
      iter = buffer->end();
      iter.backward_chars(4);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test([]");
      auto iter = buffer->end();
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test([\n"
                                     "    \n"
                                     "  ]");
      iter = buffer->end();
      iter.backward_chars(4);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test([])");
      auto iter = buffer->end();
      iter.backward_chars(2);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test([\n"
                                     "    \n"
                                     "  ])");
      iter = buffer->end();
      iter.backward_chars(5);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  function test() {return 0;");
      auto iter = buffer->begin();
      iter.forward_chars(19);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  function test() {\n"
                                     "    return 0;\n"
                                     "  }");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  return (0");
      auto iter = buffer->begin();
      iter.forward_chars(10);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  return (\n"
                                     "    0\n"
                                     "  )");
      g_assert(buffer->get_insert()->get_iter().get_line() == 1);
      g_assert(buffer->get_insert()->get_iter().get_line_offset() == 4);
    }
    {
      buffer->set_text("  test\n"
                       "    test");
      auto iter = buffer->end();
      iter.backward_chars(9);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test\n"
                                     "    \n"
                                     "    test");
      iter = buffer->end();
      iter.backward_chars(9);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(test\n"
                       "    test");
      auto iter = buffer->end();
      iter.backward_chars(9);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(test\n"
                                     "    \n"
                                     "    test");
      iter = buffer->end();
      iter.backward_chars(9);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(\n"
                       "    test\n"
                       "    test");
      auto iter = buffer->end();
      iter.backward_chars(9);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "    test\n"
                                     "    \n"
                                     "    test");
      iter = buffer->end();
      iter.backward_chars(9);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(\n"
                       "    test\n"
                       "      test");
      auto iter = buffer->end();
      iter.backward_chars(11);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "    test\n"
                                     "      \n"
                                     "      test");
      iter = buffer->end();
      iter.backward_chars(11);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(\n"
                       "    <br>\n"
                       "      <br>");
      auto iter = buffer->end();
      iter.backward_chars(11);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "    <br>\n"
                                     "      \n"
                                     "      <br>");
      iter = buffer->end();
      iter.backward_chars(11);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(\n"
                       "    <br>");
      auto iter = buffer->end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "    <br>\n"
                                     "    ");
      iter = buffer->end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(\n"
                       "    <div>");
      auto iter = buffer->end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "    <div>\n"
                                     "      \n"
                                     "    </div>");
      iter = buffer->end();
      iter.backward_chars(11);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(\n"
                       "    <div>\n");
      auto iter = buffer->end();
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "    <div>\n"
                                     "      \n"
                                     "    </div>\n");
      iter = buffer->end();
      iter.backward_chars(12);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(\n"
                       "    <div>\n"
                       "      <div>\n"
                       "    </div>\n");
      auto iter = buffer->get_iter_at_line(3);
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "    <div>\n"
                                     "      <div>\n"
                                     "        \n"
                                     "      </div>\n"
                                     "    </div>\n");
      iter = buffer->get_iter_at_line(4);
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(\n"
                       "    <div>\n"
                       "      <div>\n"
                       "      {'hello'}\n"
                       "    </div>\n");
      auto iter = buffer->get_iter_at_line(3);
      iter.backward_char();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "    <div>\n"
                                     "      <div>\n"
                                     "        \n"
                                     "      </div>\n"
                                     "      {'hello'}\n"
                                     "    </div>\n");
      iter = buffer->get_iter_at_line(4);
      iter.backward_char();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(\n"
                       "    <div onClick={() => {}}>");
      auto iter = buffer->end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "    <div onClick={() => {}}>\n"
                                     "      \n"
                                     "    </div>");
      iter = buffer->end();
      iter.backward_chars(11);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(\n"
                       "    <>");
      auto iter = buffer->end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "    <>\n"
                                     "      \n"
                                     "    </>");
      iter = buffer->end();
      iter.backward_chars(8);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(\n"
                       "    <div>\n"
                       "    </div>\n");
      auto iter = buffer->end();
      iter.backward_chars(12);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "    <div>\n"
                                     "      \n"
                                     "    </div>\n");
      iter = buffer->end();
      iter.backward_chars(12);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(\n"
                       "    <div></div>");
      auto iter = buffer->end();
      iter.backward_chars(6);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "    <div>\n"
                                     "      \n"
                                     "    </div>");
      iter = buffer->end();
      iter.backward_chars(11);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(\n"
                       "    <div></DIV>");
      auto iter = buffer->end();
      iter.backward_chars(6);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "    <div>\n"
                                     "      </DIV>");
      iter = buffer->end();
      iter.backward_chars(6);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(\n"
                       "    <div><div>");
      auto iter = buffer->end();
      iter.backward_chars(5);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "    <div>\n"
                                     "      <div>");
      iter = buffer->end();
      iter.backward_chars(5);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test(\n"
                       "    <div>test</div>");
      auto iter = buffer->end();
      iter.backward_chars(10);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test(\n"
                                     "    <div>\n"
                                     "      test\n"
                                     "    </div>");
      iter = buffer->end();
      iter.backward_chars(15);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("f(\n"
                       "  () => {\n"
                       "    a();\n"
                       "  }\n"
                       ")\n");
      auto iter = buffer->get_iter_at_line(0);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "f(\n"
                                     "  \n"
                                     "  () => {\n"
                                     "    a();\n"
                                     "  }\n"
                                     ")\n");
      iter = buffer->get_iter_at_line(1);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("f(\n"
                       "  () => {\n"
                       "    a();\n"
                       "  }\n"
                       ")\n");
      auto iter = buffer->get_iter_at_line(1);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "f(\n"
                                     "  () => {\n"
                                     "    \n"
                                     "    a();\n"
                                     "  }\n"
                                     ")\n");
      iter = buffer->get_iter_at_line(2);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("f(\n"
                       "  () => {\n"
                       "    a();\n"
                       "  }\n"
                       ")\n");
      auto iter = buffer->get_iter_at_line(1);
      iter.forward_chars(3);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "f(\n"
                                     "  (\n"
                                     "    \n"
                                     "  ) => {\n"
                                     "    a();\n"
                                     "  }\n"
                                     ")\n");
      iter = buffer->get_iter_at_line(2);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("f(\n"
                       "  () => {\n"
                       "    a();\n"
                       "  }\n"
                       ")\n");
      auto iter = buffer->get_iter_at_line(2);
      iter.forward_chars(6);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "f(\n"
                                     "  () => {\n"
                                     "    a(\n"
                                     "      \n"
                                     "    );\n"
                                     "  }\n"
                                     ")\n");
      iter = buffer->get_iter_at_line(3);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("  test\n"
                       "    .test();");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  test\n"
                                     "    .test();\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("  {}");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "  {}\n"
                                     "  ");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text("{}");
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "{}\n"
                                     "");
      g_assert(buffer->get_insert()->get_iter() == buffer->end());
    }
    {
      buffer->set_text(R"(function test() {
  let a = {
    red: 0.0,
    green: 0.0,
    blue: 1.0,
  };
}
)");
      auto iter = buffer->begin();
      iter.forward_chars(79);
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == R"(function test() {
  let a = {
    red: 0.0,
    green: 0.0,
    blue: 1.0,
  };
  
}
)");
      iter = buffer->begin();
      iter.forward_chars(82);
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
  }
  {
    auto language = language_manager->get_language("markdown");
    Source::View view(source_file, language);
    view.spellcheck_all = true;
    view.get_source_buffer()->set_highlight_syntax(true);
    view.set_tab_char_and_size(' ', 2);
    auto buffer = view.get_buffer();
    event.keyval = GDK_KEY_Return;
    {
      buffer->set_text("\n"
                       "    * [test](https://test.org)\n");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      auto iter = buffer->get_iter_at_line(1);
      iter.forward_to_line_end();
      buffer->place_cursor(iter);
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "\n"
                                     "    * [test](https://test.org)\n"
                                     "    \n");
      iter = buffer->get_iter_at_line(2);
      iter.forward_to_line_end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
    {
      buffer->set_text("\n"
                       "    * [test](https://test.org)");
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      view.on_key_press_event(&event);
      g_assert(buffer->get_text() == "\n"
                                     "    * [test](https://test.org)\n"
                                     "    ");
      auto iter = buffer->end();
      g_assert(buffer->get_insert()->get_iter() == iter);
    }
  }
}
