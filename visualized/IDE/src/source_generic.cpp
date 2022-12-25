#include "source_generic.hpp"
#include "filesystem.hpp"
#include "info.hpp"
#include "selection_dialog.hpp"
#include "snippets.hpp"
#include "terminal.hpp"
#include "utility.hpp"
#include <algorithm>
#include <boost/algorithm/string.hpp>

Source::GenericView::GenericView(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language) : BaseView(file_path, language), View(file_path, language, true), autocomplete(this, interactive_completion, last_keyval, false, false) {
  if(language) {
    auto language_manager = LanguageManager::get_default();
    auto search_paths = language_manager->get_search_path();
    bool found_language_file = false;
    boost::filesystem::path language_file;
    boost::system::error_code ec;
    for(auto &search_path : search_paths) {
      boost::filesystem::path p(static_cast<std::string>(search_path) + '/' + static_cast<std::string>(language->get_id()) + ".lang");
      if(boost::filesystem::exists(p, ec) && boost::filesystem::is_regular_file(p, ec)) {
        language_file = p;
        found_language_file = true;
        break;
      }
    }

    if(found_language_file) {
      boost::property_tree::ptree pt;
      try {
        boost::property_tree::xml_parser::read_xml(language_file.string(), pt);
        parse_language_file(pt);
      }
      catch(const std::exception &e) {
        Terminal::get().print("\e[31mError\e[m: error parsing language file " + filesystem::get_short_path(language_file).string() + ": " + e.what() + '\n', true);
      }
    }
  }

  setup_buffer_words();

  setup_autocomplete();
}

void Source::GenericView::parse_language_file(const boost::property_tree::ptree &pt) {
  bool case_insensitive = false;
  for(auto &node : pt) {
    if(node.first == "<xmlcomment>") {
      auto data = static_cast<std::string>(node.second.data());
      std::transform(data.begin(), data.end(), data.begin(), ::tolower);
      if(data.find("case insensitive") != std::string::npos)
        case_insensitive = true;
    }
    else if(node.first == "keyword") {
      auto data = static_cast<std::string>(node.second.data());
      keywords.emplace(data);

      if(case_insensitive) {
        std::transform(data.begin(), data.end(), data.begin(), ::tolower);
        keywords.emplace(data);
      }
    }
    try {
      parse_language_file(node.second);
    }
    catch(const std::exception &e) {
    }
  }
}

std::vector<std::pair<Gtk::TextIter, Gtk::TextIter>> Source::GenericView::get_words(const Gtk::TextIter &start, const Gtk::TextIter &end) {
  std::vector<std::pair<Gtk::TextIter, Gtk::TextIter>> words;

  auto iter = start;
  while(iter && iter < end) {
    if(is_token_char(*iter)) {
      auto word = get_token_iters(iter);
      if(!(*word.first >= '0' && *word.first <= '9') && (word.second.get_offset() - word.first.get_offset()) >= 3) // Minimum word length: 3
        words.emplace_back(word.first, word.second);
      iter = word.second;
    }
    iter.forward_char();
  }

  return words;
}

void Source::GenericView::setup_buffer_words() {
  {
    auto words = get_words(get_buffer()->begin(), get_buffer()->end());
    for(auto &word : words) {
      auto result = buffer_words.emplace(get_buffer()->get_text(word.first, word.second), 1);
      if(!result.second)
        ++(result.first->second);
    }
  }

  // Remove changed word at insert
  get_buffer()->signal_insert().connect(
      [this](const Gtk::TextIter &iter_, const Glib::ustring &text, int bytes) {
        auto iter = iter_;
        if(!is_token_char(*iter))
          iter.backward_char();

        if(is_token_char(*iter)) {
          auto word = get_token_iters(iter);
          if(word.second.get_offset() - word.first.get_offset() >= 3) {
            auto it = buffer_words.find(get_buffer()->get_text(word.first, word.second));
            if(it != buffer_words.end()) {
              if(it->second > 1)
                --(it->second);
              else
                buffer_words.erase(it);
            }
          }
        }
      },
      false);

  // Add all words between start and end of insert
  get_buffer()->signal_insert().connect([this](const Gtk::TextIter &iter, const Glib::ustring &text, int bytes) {
    auto start = iter;
    auto end = iter;
    start.backward_chars(text.size());
    if(!is_token_char(*start))
      start.backward_char();
    end.forward_char();

    auto words = get_words(start, end);
    for(auto &word : words) {
      auto result = buffer_words.emplace(get_buffer()->get_text(word.first, word.second), 1);
      if(!result.second)
        ++(result.first->second);
    }
  });

  // Remove words within text that was removed
  get_buffer()->signal_erase().connect(
      [this](const Gtk::TextIter &start_, const Gtk::TextIter &end_) {
        auto start = start_;
        auto end = end_;
        if(!is_token_char(*start))
          start.backward_char();
        end.forward_char();
        auto words = get_words(start, end);
        for(auto &word : words) {
          auto it = buffer_words.find(get_buffer()->get_text(word.first, word.second));
          if(it != buffer_words.end()) {
            if(it->second > 1)
              --(it->second);
            else
              buffer_words.erase(it);
          }
        }
      },
      false);

  // Add new word resulting from erased text
  get_buffer()->signal_erase().connect([this](const Gtk::TextIter &start_, const Gtk::TextIter & /*end*/) {
    auto start = start_;
    if(!is_token_char(*start))
      start.backward_char();
    if(is_token_char(*start)) {
      auto word = get_token_iters(start);
      if(word.second.get_offset() - word.first.get_offset() >= 3) {
        auto result = buffer_words.emplace(get_buffer()->get_text(word.first, word.second), 1);
        if(!result.second)
          ++(result.first->second);
      }
    }
  });
}

void Source::GenericView::setup_autocomplete() {
  non_interactive_completion = [this] {
    if(CompletionDialog::get() && CompletionDialog::get()->is_visible())
      return;
    autocomplete.run();
  };

  autocomplete.run_check = [this]() {
    auto prefix_start = get_buffer()->get_insert()->get_iter();
    auto prefix_end = prefix_start;

    size_t count = 0;
    while(prefix_start.backward_char() && is_token_char(*prefix_start))
      ++count;

    if(prefix_start != prefix_end && !is_token_char(*prefix_start))
      prefix_start.forward_char();

    if((count >= 3 && !(*prefix_start >= '0' && *prefix_start <= '9')) || !interactive_completion) {
      LockGuard lock(autocomplete.prefix_mutex);
      autocomplete.prefix = get_buffer()->get_text(prefix_start, prefix_end);

      if(interactive_completion)
        show_prefix_buffer_word = buffer_words.find(autocomplete.prefix) != buffer_words.end();
      else {
        auto it = buffer_words.find(autocomplete.prefix);
        show_prefix_buffer_word = !(it == buffer_words.end() || it->second == 1);
      }
      return true;
    }

    return false;
  };

  autocomplete.add_rows = [this](std::string & /*buffer*/, int /*line*/, int /*line_index*/) {
    if(autocomplete.state == Autocomplete::State::starting) {
      autocomplete_comment.clear();
      autocomplete_insert.clear();

      std::string prefix;
      {
        LockGuard lock(autocomplete.prefix_mutex);
        prefix = autocomplete.prefix;
      }
      for(auto &keyword : keywords) {
        if(starts_with(keyword, prefix)) {
          autocomplete.rows.emplace_back(keyword);
          autocomplete_insert.emplace_back(keyword);
          autocomplete_comment.emplace_back("");
        }
      }
      {
        for(auto &buffer_word : buffer_words) {
          if((show_prefix_buffer_word || buffer_word.first.size() > prefix.size()) &&
             starts_with(buffer_word.first, prefix) &&
             keywords.find(buffer_word.first) == keywords.end()) {
            autocomplete.rows.emplace_back(buffer_word.first);
            auto insert = buffer_word.first;
            boost::replace_all(insert, "$", "\\$");
            autocomplete_insert.emplace_back(insert);
            autocomplete_comment.emplace_back("");
          }
        }
      }
      if(snippets_mutex.try_lock()) { // To avoid deadlock when inserting snippets
        if(snippets) {
          for(auto &snippet : *snippets) {
            if(starts_with(snippet.prefix, prefix)) {
              autocomplete.rows.emplace_back(snippet.prefix);
              autocomplete_insert.emplace_back(snippet.body);
              autocomplete_comment.emplace_back(snippet.description);
            }
          }
        }
        snippets_mutex.unlock();
      }
    }
    return true;
  };

  autocomplete.on_show = [this] {
    hide_tooltips();
  };

  autocomplete.on_hide = [this] {
    autocomplete_comment.clear();
    autocomplete_insert.clear();
  };

  autocomplete.on_select = [this](unsigned int index, const std::string &text, bool hide_window) {
    get_buffer()->erase(CompletionDialog::get()->start_mark->get_iter(), get_buffer()->get_insert()->get_iter());

    if(hide_window)
      insert_snippet(CompletionDialog::get()->start_mark->get_iter(), autocomplete_insert[index]);
    else
      get_buffer()->insert(CompletionDialog::get()->start_mark->get_iter(), text);
  };

  autocomplete.set_tooltip_buffer = [this](unsigned int index) -> std::function<void(Tooltip & tooltip)> {
    auto tooltip_str = autocomplete_comment[index];
    if(tooltip_str.empty())
      return nullptr;
    return [tooltip_str = std::move(tooltip_str)](Tooltip &tooltip) {
      tooltip.insert_with_links_tagged(tooltip_str);
    };
  };
}
