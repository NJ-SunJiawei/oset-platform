#pragma once

#include "autocomplete.hpp"
#include "source.hpp"
#include <atomic>

namespace Source {
  class GenericView : public View {
  public:
    GenericView(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language);

  private:
    void parse_language_file(const boost::property_tree::ptree &pt);

    std::set<std::string> keywords;

    std::vector<std::pair<Gtk::TextIter, Gtk::TextIter>> get_words(const Gtk::TextIter &start, const Gtk::TextIter &end);

    std::map<std::string, size_t> buffer_words;
    bool show_prefix_buffer_word = false; /// To avoid showing the current word if it is unique in document
    void setup_buffer_words();

    Autocomplete autocomplete;
    std::vector<std::string> autocomplete_comment;
    std::vector<std::string> autocomplete_insert;
    void setup_autocomplete();
  };
} // namespace Source
