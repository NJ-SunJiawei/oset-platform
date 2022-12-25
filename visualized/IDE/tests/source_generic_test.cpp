#include "source_generic.hpp"
#include <glib.h>

int main() {
  auto app = Gtk::Application::create();
  Gsv::init();

  auto tests_path = boost::filesystem::canonical(JUCI_TESTS_PATH);
  auto source_file = tests_path / "tmp" / "source_file.md";

  auto language_manager = Source::LanguageManager::get_default();

  auto language = language_manager->get_language("markdown");
  Source::GenericView view(source_file, language);

  // Test buffer words
  {
    std::map<std::string, size_t> buffer_words = {};
    assert(view.buffer_words == buffer_words);

    view.get_buffer()->set_text("testing 1 2 3");
    buffer_words = {{"testing", 1}};
    assert(view.buffer_words == buffer_words);

    view.get_buffer()->set_text("");
    assert(view.buffer_words.empty());

    view.get_buffer()->set_text("\ntest ing te\n");
    buffer_words = {{"test", 1}, {"ing", 1}};
    assert(view.buffer_words == buffer_words);

    view.get_buffer()->set_text("test ing\ntest ing te\n");
    buffer_words = {{"test", 2}, {"ing", 2}};
    assert(view.buffer_words == buffer_words);

    auto start = view.get_buffer()->begin();
    start.forward_chars(4);
    auto end = start;
    end.forward_char();
    view.get_buffer()->erase(start, end);
    buffer_words = {{"testing", 1}, {"test", 1}, {"ing", 1}};
    assert(view.buffer_words == buffer_words);

    start = view.get_buffer()->begin();
    start.forward_chars(4);
    view.get_buffer()->insert(start, " ");
    buffer_words = {{"test", 2}, {"ing", 2}};
    assert(view.buffer_words == buffer_words);

    view.get_buffer()->erase(view.get_buffer()->begin(), view.get_buffer()->end());
    assert(view.buffer_words.empty());

    view.get_buffer()->set_text("this is this");
    buffer_words = {{"this", 2}};
    assert(view.buffer_words == buffer_words);

    start = view.get_buffer()->begin();
    start.forward_chars(4);
    end = start;
    end.forward_char();
    view.get_buffer()->erase(start, end);
    buffer_words = {{"thisis", 1}, {"this", 1}};
    assert(view.buffer_words == buffer_words);

    start = view.get_buffer()->begin();
    start.forward_chars(4);
    view.get_buffer()->insert(start, "\n\n");
    buffer_words = {{"this", 2}};
    assert(view.buffer_words == buffer_words);

    view.get_buffer()->set_text("test");
    buffer_words = {{"test", 1}};
    assert(view.buffer_words == buffer_words);

    start = view.get_buffer()->begin();
    end = start;
    end.forward_chars(2);
    view.get_buffer()->erase(start, end);
    assert(view.buffer_words.empty());

    start = view.get_buffer()->begin();
    view.get_buffer()->insert(start, "te");
    buffer_words = {{"test", 1}};
    assert(view.buffer_words == buffer_words);

    start = view.get_buffer()->begin();
    start.forward_chars(2);
    end = start;
    end.forward_chars(2);
    view.get_buffer()->erase(start, end);
    assert(view.buffer_words.empty());

    start = view.get_buffer()->begin();
    start.forward_chars(2);
    view.get_buffer()->insert(start, "st");
    buffer_words = {{"test", 1}};
    assert(view.buffer_words == buffer_words);

    view.get_buffer()->set_text("\ntest\n");
    buffer_words = {{"test", 1}};
    assert(view.buffer_words == buffer_words);

    start = view.get_buffer()->begin();
    start.forward_char();
    end = start;
    end.forward_chars(2);
    view.get_buffer()->erase(start, end);
    assert(view.buffer_words.empty());

    start = view.get_buffer()->begin();
    start.forward_char();
    view.get_buffer()->insert(start, "te");
    buffer_words = {{"test", 1}};
    assert(view.buffer_words == buffer_words);

    start = view.get_buffer()->begin();
    start.forward_chars(3);
    end = start;
    end.forward_chars(2);
    view.get_buffer()->erase(start, end);
    assert(view.buffer_words.empty());

    start = view.get_buffer()->begin();
    start.forward_chars(3);
    view.get_buffer()->insert(start, "st");
    buffer_words = {{"test", 1}};
    assert(view.buffer_words == buffer_words);

    start = view.get_buffer()->begin();
    start.forward_chars(2);
    view.get_buffer()->insert(start, "ee");
    buffer_words = {{"teeest", 1}};
    assert(view.buffer_words == buffer_words);

    start = view.get_buffer()->begin();
    start.forward_chars(2);
    end = start;
    end.forward_chars(2);
    view.get_buffer()->erase(start, end);
    buffer_words = {{"test", 1}};
    assert(view.buffer_words == buffer_words);

    view.get_buffer()->set_text("test");
    buffer_words = {{"test", 1}};
    assert(view.buffer_words == buffer_words);

    start = view.get_buffer()->begin();
    start.forward_chars(4);
    view.get_buffer()->insert(start, " testing");
    buffer_words = {{"test", 1}, {"testing", 1}};
    assert(view.buffer_words == buffer_words);

    start = view.get_buffer()->begin();
    start.forward_chars(2);
    end = start;
    end.forward_chars(7);
    view.get_buffer()->erase(start, end);
    buffer_words = {{"teing", 1}};
    assert(view.buffer_words == buffer_words);
  }
}