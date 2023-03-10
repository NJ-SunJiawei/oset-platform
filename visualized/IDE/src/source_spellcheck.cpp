#include "source_spellcheck.hpp"
#include "config.hpp"
#include "info.hpp"
#include "selection_dialog.hpp"
#include <iostream>

AspellConfig *Source::SpellCheckView::spellcheck_config = nullptr;

Source::SpellCheckView::SpellCheckView(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language) : BaseView(file_path, language) {
  if(!language ||                                     // Spellcheck all words if no spec is found
     is_language({"markdown", "latex", "html", "xml", // Spellcheck all non-symbol words (not only string or comment context classes)
                  "go"}))                             // Go spec does not contain string or comment context classes
    spellcheck_all = true;

  if(!spellcheck_config)
    spellcheck_config = new_aspell_config();
  spellcheck_checker = nullptr;
  spellcheck_error_tag = get_buffer()->create_tag("spellcheck_error");
  spellcheck_error_tag->property_underline() = Pango::Underline::UNDERLINE_ERROR;

  signal_key_press_event().connect(
      [](GdkEventKey *event) {
        if(SelectionDialog::get() && SelectionDialog::get()->is_visible()) {
          if(SelectionDialog::get()->on_key_press(event))
            return true;
        }

        return false;
      },
      false);

  // The following signal is added in case SpellCheckView is not subclassed
  signal_key_press_event().connect([this](GdkEventKey *event) {
    last_keyval = event->keyval;
    return false;
  });

  get_buffer()->signal_changed().connect([this]() {
    if(!spellcheck_checker)
      return;

    delayed_spellcheck_suggestions_connection.disconnect();

    auto iter = get_buffer()->get_insert()->get_iter();
    if(!is_word_iter(iter) && !iter.starts_line())
      iter.backward_char();

    if(disable_spellcheck) {
      if(is_word_iter(iter)) {
        auto word = get_word(iter);
        get_buffer()->remove_tag(spellcheck_error_tag, word.first, word.second);
      }
      return;
    }

    if(is_spellcheck_iter(iter)) {
      if(last_keyval == GDK_KEY_Return || last_keyval == GDK_KEY_KP_Enter) {
        auto previous_line_iter = iter;
        while(previous_line_iter.backward_char() && !previous_line_iter.ends_line()) {
        }
        if(previous_line_iter.backward_char()) {
          get_buffer()->remove_tag(spellcheck_error_tag, previous_line_iter, iter);
          if(is_spellcheck_iter(previous_line_iter)) {
            auto word = get_word(previous_line_iter);
            spellcheck_word(word.first, word.second);
          }
          auto word = get_word(iter);
          spellcheck_word(word.first, word.second);
        }
      }
      else {
        auto previous_iter = iter;
        // When for instance using space to split two words
        if(!iter.starts_line() && !iter.ends_line() && is_word_iter(iter) &&
           previous_iter.backward_char() && !previous_iter.starts_line() && !is_word_iter(previous_iter)) {
          auto first = previous_iter;
          if(first.backward_char()) {
            get_buffer()->remove_tag(spellcheck_error_tag, first, iter);
            auto word = get_word(first);
            spellcheck_word(word.first, word.second);
            word = get_word(iter);
            spellcheck_word(word.first, word.second);
          }
        }
        else {
          auto word = get_word(iter);
          spellcheck_word(word.first, word.second);
        }
      }
    }
    delayed_spellcheck_error_clear.disconnect();
    delayed_spellcheck_error_clear = Glib::signal_timeout().connect(
        [this]() {
          auto iter = get_buffer()->begin();
          Gtk::TextIter begin_no_spellcheck_iter;
          if(spellcheck_all) {
            bool spell_check = !get_source_buffer()->iter_has_context_class(iter, "no-spell-check");
            if(!spell_check)
              begin_no_spellcheck_iter = iter;
            while(iter != get_buffer()->end()) {
              if(!get_source_buffer()->iter_forward_to_context_class_toggle(iter, "no-spell-check"))
                iter = get_buffer()->end();

              spell_check = !spell_check;
              if(!spell_check)
                begin_no_spellcheck_iter = iter;
              else
                get_buffer()->remove_tag(spellcheck_error_tag, begin_no_spellcheck_iter, iter);
            }
            return false;
          }

          bool spell_check = get_source_buffer()->iter_has_context_class(iter, "string") || get_source_buffer()->iter_has_context_class(iter, "comment");
          if(!spell_check)
            begin_no_spellcheck_iter = iter;
          while(iter != get_buffer()->end()) {
            auto iter1 = iter;
            auto iter2 = iter;
            if(!get_source_buffer()->iter_forward_to_context_class_toggle(iter1, "string"))
              iter1 = get_buffer()->end();
            if(!get_source_buffer()->iter_forward_to_context_class_toggle(iter2, "comment"))
              iter2 = get_buffer()->end();

            if(iter2 < iter1)
              iter = iter2;
            else
              iter = iter1;
            spell_check = !spell_check;
            if(!spell_check)
              begin_no_spellcheck_iter = iter;
            else
              get_buffer()->remove_tag(spellcheck_error_tag, begin_no_spellcheck_iter, iter);
          }
          return false;
        },
        1000);
  });

  // In case of for instance text paste or undo/redo
  get_buffer()->signal_insert().connect(
      [this](const Gtk::TextIter &start_iter, const Glib::ustring &inserted_string, int) {
        if(!spellcheck_checker)
          return;

        if(disable_spellcheck) {
          auto iter = start_iter;
          if(!is_word_iter(iter) && !iter.starts_line())
            iter.backward_char();
          if(is_word_iter(iter)) {
            auto word = get_word(iter);
            get_buffer()->remove_tag(spellcheck_error_tag, word.first, word.second);
          }
        }
      },
      false);

  get_buffer()->signal_mark_set().connect([this](const Gtk::TextIter &iter, const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark) {
    if(mark->get_name() == "insert") {
      if(SelectionDialog::get())
        SelectionDialog::get()->hide();
      if(!spellcheck_checker)
        return;
      delayed_spellcheck_suggestions_connection.disconnect();
      if(get_buffer()->get_has_selection())
        return;
      delayed_spellcheck_suggestions_connection = Glib::signal_timeout().connect(
          [this]() {
            if(get_buffer()->get_insert()->get_iter().has_tag(spellcheck_error_tag)) {
              SelectionDialog::create(this, false);
              auto word = get_word(get_buffer()->get_insert()->get_iter());
              if(*word.first == '\'' && word.second.get_offset() - word.first.get_offset() >= 3) {
                auto before_end = word.second;
                if(before_end.backward_char() && *before_end == '\'') {
                  word.first.forward_char();
                  word.second.backward_char();
                }
              }
              auto suggestions = get_spellcheck_suggestions(word.first, word.second);
              if(suggestions.size() == 0)
                return false;
              for(auto &suggestion : suggestions)
                SelectionDialog::get()->add_row(suggestion);
              SelectionDialog::get()->on_select = [this, word](unsigned int index, const std::string &text, bool hide_window) {
                get_buffer()->begin_user_action();
                get_buffer()->erase(word.first, word.second);
                get_buffer()->insert(get_buffer()->get_insert()->get_iter(), text);
                get_buffer()->end_user_action();
              };
              hide_tooltips();
              SelectionDialog::get()->show();
            }
            return false;
          },
          500);
    }
  });

  signal_focus_out_event().connect([this](GdkEventFocus *event) {
    delayed_spellcheck_suggestions_connection.disconnect();
    return false;
  });

  signal_leave_notify_event().connect([this](GdkEventCrossing *) {
    delayed_spellcheck_suggestions_connection.disconnect();
    return false;
  });

  signal_tag_added_connection = get_buffer()->get_tag_table()->signal_tag_added().connect([this](const Glib::RefPtr<Gtk::TextTag> &tag) {
    if(tag->property_name() == "gtksourceview:context-classes:comment")
      comment_tag = tag;
    else if(tag->property_name() == "gtksourceview:context-classes:string")
      string_tag = tag;
    else if(tag->property_name() == "gtksourceview:context-classes:no-spell-check")
      no_spellcheck_tag = tag;
  });
  signal_tag_removed_connection = get_buffer()->get_tag_table()->signal_tag_removed().connect([this](const Glib::RefPtr<Gtk::TextTag> &tag) {
    if(tag->property_name() == "gtksourceview:context-classes:comment")
      comment_tag.reset();
    else if(tag->property_name() == "gtksourceview:context-classes:string")
      string_tag.reset();
    else if(tag->property_name() == "gtksourceview:context-classes:no-spell-check")
      no_spellcheck_tag.reset();
  });
}

Source::SpellCheckView::~SpellCheckView() {
  delayed_spellcheck_suggestions_connection.disconnect();
  delayed_spellcheck_error_clear.disconnect();

  if(spellcheck_checker)
    delete_aspell_speller(spellcheck_checker);

  signal_tag_added_connection.disconnect();
  signal_tag_removed_connection.disconnect();
}

void Source::SpellCheckView::configure() {
  if(Config::get().source.spellcheck_language.size() > 0) {
    aspell_config_replace(spellcheck_config, "lang", Config::get().source.spellcheck_language.c_str());
    aspell_config_replace(spellcheck_config, "encoding", "utf-8");
  }
  spellcheck_possible_err = new_aspell_speller(spellcheck_config);
  if(spellcheck_checker)
    delete_aspell_speller(spellcheck_checker);
  spellcheck_checker = nullptr;
  if(aspell_error_number(spellcheck_possible_err) != 0)
    std::cerr << "Spell check error: " << aspell_error_message(spellcheck_possible_err) << std::endl;
  else
    spellcheck_checker = to_aspell_speller(spellcheck_possible_err);
  get_buffer()->remove_tag(spellcheck_error_tag, get_buffer()->begin(), get_buffer()->end());
}

void Source::SpellCheckView::hide_dialogs() {
  delayed_spellcheck_suggestions_connection.disconnect();
  if(SelectionDialog::get())
    SelectionDialog::get()->hide();
}

void Source::SpellCheckView::spellcheck(const Gtk::TextIter &start, const Gtk::TextIter &end) {
  if(!spellcheck_checker)
    return;
  auto iter = start;
  while(iter && iter < end) {
    if(is_word_iter(iter)) {
      auto word = get_word(iter);
      spellcheck_word(word.first, word.second);
      iter = word.second;
    }
    iter.forward_char();
  }
}

void Source::SpellCheckView::spellcheck() {
  auto iter = get_buffer()->begin();
  Gtk::TextIter begin_spellcheck_iter;
  if(spellcheck_all) {
    bool spell_check = !get_source_buffer()->iter_has_context_class(iter, "no-spell-check");
    if(spell_check)
      begin_spellcheck_iter = iter;
    while(iter != get_buffer()->end()) {
      if(!get_source_buffer()->iter_forward_to_context_class_toggle(iter, "no-spell-check"))
        iter = get_buffer()->end();

      spell_check = !spell_check;
      if(spell_check)
        begin_spellcheck_iter = iter;
      else
        spellcheck(begin_spellcheck_iter, iter);
    }
  }
  else {
    bool spell_check = get_source_buffer()->iter_has_context_class(iter, "string") || get_source_buffer()->iter_has_context_class(iter, "comment");
    if(spell_check)
      begin_spellcheck_iter = iter;
    while(iter != get_buffer()->end()) {
      auto iter1 = iter;
      auto iter2 = iter;
      if(!get_source_buffer()->iter_forward_to_context_class_toggle(iter1, "string"))
        iter1 = get_buffer()->end();
      if(!get_source_buffer()->iter_forward_to_context_class_toggle(iter2, "comment"))
        iter2 = get_buffer()->end();

      if(iter2 < iter1)
        iter = iter2;
      else
        iter = iter1;
      spell_check = !spell_check;
      if(spell_check)
        begin_spellcheck_iter = iter;
      else
        spellcheck(begin_spellcheck_iter, iter);
    }
  }
}

void Source::SpellCheckView::remove_spellcheck_errors() {
  get_buffer()->remove_tag(spellcheck_error_tag, get_buffer()->begin(), get_buffer()->end());
}

void Source::SpellCheckView::goto_next_spellcheck_error() {
  auto iter = get_buffer()->get_insert()->get_iter();
  auto insert_iter = iter;
  bool wrapped = false;
  iter.forward_char();
  while(!wrapped || iter < insert_iter) {
    auto toggled_tags = iter.get_toggled_tags();
    for(auto &toggled_tag : toggled_tags) {
      if(toggled_tag == spellcheck_error_tag) {
        get_buffer()->place_cursor(iter);
        scroll_to(get_buffer()->get_insert(), 0.0, 1.0, 0.5);
        return;
      }
    }
    iter.forward_char();
    if(!wrapped && iter == get_buffer()->end()) {
      iter = get_buffer()->begin();
      wrapped = true;
    }
  }
  Info::get().print("No spelling errors found in current buffer");
}

bool Source::SpellCheckView::is_spellcheck_iter(const Gtk::TextIter &iter) {
  if(*iter == '\'') { // Handle iter inside ''
    auto previous_iter = iter;
    if(previous_iter.backward_char() && *previous_iter == '\'')
      return true;
  }
  if(spellcheck_all) {
    if(no_spellcheck_tag) {
      if(iter.has_tag(no_spellcheck_tag) || iter.begins_tag(no_spellcheck_tag) || iter.ends_tag(no_spellcheck_tag))
        return false;
      // workaround for gtksourceview bug
      if(iter.ends_line()) {
        auto previous_iter = iter;
        if(previous_iter.backward_char()) {
          if(*previous_iter == '\'' || *previous_iter == '"') {
            auto next_iter = iter;
            next_iter.forward_char();
            if(next_iter.begins_tag(no_spellcheck_tag) || next_iter.is_end())
              return false;
          }
        }
      }
      // for example, mark first " as not spellcheck iter in this case: r""
      if(*iter == '\'' || *iter == '"') {
        auto previous_iter = iter;
        if(previous_iter.backward_char() && *previous_iter != '\'' && *previous_iter != '\"' && previous_iter.ends_tag(no_spellcheck_tag))
          return false;
      }
    }
    return true;
  }
  if(comment_tag) {
    if(iter.has_tag(comment_tag) && !iter.begins_tag(comment_tag))
      return true;
    // Exception at the end of /**/
    else if(iter.ends_tag(comment_tag)) {
      auto previous_iter = iter;
      if(previous_iter.backward_char() && *previous_iter == '/') {
        auto previous_previous_iter = previous_iter;
        if(previous_previous_iter.backward_char() && *previous_previous_iter == '*') {
          auto start_iter = iter;
          if(start_iter.backward_to_tag_toggle(comment_tag)) {
            auto next_iter = start_iter;
            if(start_iter.begins_tag(comment_tag) && next_iter.forward_char() && *start_iter == '/' && *next_iter == '*')
              return false;
          }
        }
      }
      return true;
    }
  }
  if(string_tag) {
    if(iter.has_tag(string_tag) && !iter.begins_tag(string_tag)) {
      // Check for string literal symbols
      if(*iter == '\'' || *iter == '"') {
        auto previous_iter = iter;
        if(previous_iter.backward_char()) {
          if(previous_iter.begins_tag(string_tag)) {
            if(*previous_iter == 'L' || *previous_iter == 'u' || *previous_iter == 'U' || *previous_iter == 'R')
              return false;
          }
          else if(*previous_iter == '8' && previous_iter.backward_char() && previous_iter.begins_tag(string_tag) &&
                  *previous_iter == 'u')
            return false;
        }
      }
      return true;
    }
    // If iter is at the end of string_tag, with exception of after " and '
    else if(iter.ends_tag(string_tag)) {
      auto previous_iter = iter;
      if(previous_iter.backward_char() && (*previous_iter == '"' || *previous_iter == '\'')) {
        long backslash_count = 0;
        auto it = previous_iter;
        while(it.backward_char() && *it == '\\')
          ++backslash_count;
        if(backslash_count % 2 == 0) {
          auto start_iter = iter;
          if(start_iter.backward_to_tag_toggle(string_tag) && start_iter.begins_tag(string_tag)) {
            // Move passed string literal symbols
            if(*start_iter == 'L' || *start_iter == 'U' || *start_iter == 'R')
              start_iter.forward_char();
            else if(*start_iter == 'u') {
              start_iter.forward_char();
              if(*start_iter == '8')
                start_iter.forward_char();
            }

            if(*previous_iter == *start_iter)
              return false;
          }
        }
      }
      return true;
    }
  }
  return false;
}

bool Source::SpellCheckView::is_code_iter(const Gtk::TextIter &iter) {
  // Returns true, for instance for C++, if iter is at either characters of // or /*
  const auto is_comment_iter = [this](const Gtk::TextIter &iter) {
    if(!comment_tag)
      return false;
    if(iter.has_tag(comment_tag))
      return true;
#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION >= 20)
    if(iter.begins_tag(comment_tag))
      return true;
#else
    if(*iter == '/') {
      auto next_iter = iter;
      if(!next_iter.ends_line() && next_iter.forward_char() && next_iter.has_tag(comment_tag))
        return true;
    }
#endif
    return false;
  };

  if(is_comment_iter(iter))
    return false;

  bool is_code_iter = !is_spellcheck_iter(iter);

  // Treat closing " or ' as code iters
  if(!is_code_iter && (*iter == '\'' || *iter == '"')) {
    if(comment_tag && iter.ends_tag(comment_tag)) // ' or " at end of comments are not code iters
      return false;
    auto next_iter = iter;
    next_iter.forward_char();
    return !is_spellcheck_iter(next_iter);
  }

  if(is_bracket_language)
    return is_code_iter;

  // Non-bracket languages can have code iters inside (), [] and {}, while non-code iters outside of these brackets
  // Do not threat these closing code brackets as code iters
  if(is_code_iter && (*iter == ')' || *iter == ']' || *iter == '}')) {
    auto next_iter = iter;
    next_iter.forward_char();
    return !is_spellcheck_iter(next_iter);
  }

  return is_code_iter;
}

bool Source::SpellCheckView::is_word_iter(const Gtk::TextIter &iter) {
  auto previous_iter = iter;
  size_t backslash_count = 0;
  while(previous_iter.backward_char() && *previous_iter == '\\')
    ++backslash_count;
  if(backslash_count % 2 == 1)
    return false;
  if(Glib::Unicode::isalpha(*iter))
    return true;
  if(*iter == '\'') {
    if(string_tag && iter.has_tag(string_tag)) {
      auto start = iter;
      if(!start.begins_tag(string_tag))
        start.backward_to_tag_toggle(string_tag);
      if(*start == '\'')
        return false;
    }
    return !is_code_iter(iter);
  }
  return false;
}

std::pair<Gtk::TextIter, Gtk::TextIter> Source::SpellCheckView::get_word(Gtk::TextIter iter) {
  auto start = iter;
  auto end = iter;

  while(is_word_iter(iter)) {
    start = iter;
    if(!iter.backward_char())
      break;
  }
  while(is_word_iter(end)) {
    if(!end.forward_char())
      break;
  }

  return {start, end};
}

void Source::SpellCheckView::spellcheck_word(Gtk::TextIter start, Gtk::TextIter end) {
  if(*start == '\'' && end.get_offset() - start.get_offset() >= 3) {
    auto before_end = end;
    if(before_end.backward_char() && *before_end == '\'') {
      get_buffer()->remove_tag(spellcheck_error_tag, start, end);
      start.forward_char();
      end.backward_char();
    }
  }

  auto word = get_buffer()->get_text(start, end);
  if(word == "''")
    get_buffer()->remove_tag(spellcheck_error_tag, start, end);
  else if(word.size() > 0) {
    auto correct = aspell_speller_check(spellcheck_checker, word.data(), word.bytes());
    if(correct == 0)
      get_buffer()->apply_tag(spellcheck_error_tag, start, end);
    else
      get_buffer()->remove_tag(spellcheck_error_tag, start, end);
  }
}

std::vector<std::string> Source::SpellCheckView::get_spellcheck_suggestions(const Gtk::TextIter &start, const Gtk::TextIter &end) {
  auto word_with_error = get_buffer()->get_text(start, end);

  const AspellWordList *suggestions = aspell_speller_suggest(spellcheck_checker, word_with_error.data(), word_with_error.bytes());
  AspellStringEnumeration *elements = aspell_word_list_elements(suggestions);

  std::vector<std::string> words;
  const char *word;
  while((word = aspell_string_enumeration_next(elements)))
    words.emplace_back(word);
  delete_aspell_string_enumeration(elements);

  return words;
}
