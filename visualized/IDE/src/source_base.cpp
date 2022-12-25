#include "source_base.hpp"
#include "config.hpp"
#include "filesystem.hpp"
#include "git.hpp"
#include "info.hpp"
#include "selection_dialog.hpp"
#include "terminal.hpp"
#include "utility.hpp"
#include <fstream>
#include <gtksourceview/gtksource.h>
#include <regex>

Source::CommonView::CommonView(const Glib::RefPtr<Gsv::Language> &language) : Gsv::View() {
  set_tab_width(4); // Visual size of a \t hardcoded to be equal to visual size of 4 spaces

  set_language(language);
  if(is_language({"chdr", "c"}))
    is_c = true;
  else if(is_language({"cpphdr", "cpp"}))
    is_cpp = true;
  else if(is_language({"js", "html"}))
    is_js = true;

  search_settings = gtk_source_search_settings_new();
  gtk_source_search_settings_set_wrap_around(search_settings, true);
  search_context = gtk_source_search_context_new(get_source_buffer()->gobj(), search_settings);
  gtk_source_search_context_set_highlight(search_context, true);
  g_signal_connect(search_context, "notify::occurrences-count", G_CALLBACK(search_occurrences_updated), this);
}

Source::CommonView::~CommonView() {
  g_clear_object(&search_context);
  g_clear_object(&search_settings);
}

void Source::CommonView::set_language(const Glib::RefPtr<Gsv::Language> &language) {
  this->language = language;
  language_id = language ? language->get_id() : "";
}

bool Source::CommonView::is_language(const std::initializer_list<std::string> &languages) {
  if(!language)
    return false;
  return std::any_of(languages.begin(), languages.end(), [this](const std::string &e) {
    return e == language_id;
  });
}

bool Source::CommonView::on_key_press_event(GdkEventKey *event) {
  if(event->keyval == GDK_KEY_Home && event->state & GDK_CONTROL_MASK) {
    auto iter = get_buffer()->begin();
    scroll_to(iter); // Home key should always scroll to start, even though cursor does not move
    return Gsv::View::on_key_press_event(event);
  }
  else if(event->keyval == GDK_KEY_End && event->state & GDK_CONTROL_MASK) {
    auto iter = get_buffer()->end();
    scroll_to(iter); // End key should always scroll to start, even though cursor does not move
    return Gsv::View::on_key_press_event(event);
  }
  return Gsv::View::on_key_press_event(event);
}

bool Source::CommonView::on_motion_notify_event(GdkEventMotion *event) {
  // Workaround for drag-and-drop crash on MacOS
  // TODO 2023: check if this bug has been fixed
#ifdef __APPLE__
  if((event->state & GDK_BUTTON1_MASK) > 0 && (event->state & GDK_SHIFT_MASK) == 0) {
    int x, y;
    window_to_buffer_coords(Gtk::TextWindowType::TEXT_WINDOW_TEXT, event->x, event->y, x, y);
    Gtk::TextIter iter;
    get_iter_at_location(iter, x, y);
    get_buffer()->select_range(iter, get_buffer()->get_selection_bound()->get_iter());
    return true;
  }
#endif
  return Gsv::View::on_motion_notify_event(event);
}

void Source::CommonView::search_highlight(const std::string &text, bool case_sensitive, bool regex) {
  gtk_source_search_settings_set_case_sensitive(search_settings, case_sensitive);
  gtk_source_search_settings_set_regex_enabled(search_settings, regex);
  gtk_source_search_settings_set_search_text(search_settings, text.c_str());
  search_occurrences_updated(nullptr, nullptr, this);
}

void Source::CommonView::search_forward() {
  Gtk::TextIter start, end;
  get_buffer()->get_selection_bounds(start, end);
  Gtk::TextIter match_start, match_end;
#if defined(GTK_SOURCE_MAJOR_VERSION) && (GTK_SOURCE_MAJOR_VERSION > 3 || (GTK_SOURCE_MAJOR_VERSION == 3 && GTK_SOURCE_MINOR_VERSION >= 22))
  gboolean has_wrapped_around;
  if(gtk_source_search_context_forward2(search_context, end.gobj(), match_start.gobj(), match_end.gobj(), &has_wrapped_around)) {
    get_buffer()->select_range(match_start, match_end);
    scroll_to(get_buffer()->get_insert());
  }
#else
  if(gtk_source_search_context_forward(search_context, end.gobj(), match_start.gobj(), match_end.gobj())) {
    get_buffer()->select_range(match_start, match_end);
    scroll_to(get_buffer()->get_insert());
  }
#endif
}

void Source::CommonView::search_backward() {
  Gtk::TextIter start, end;
  get_buffer()->get_selection_bounds(start, end);
  Gtk::TextIter match_start, match_end;
#if defined(GTK_SOURCE_MAJOR_VERSION) && (GTK_SOURCE_MAJOR_VERSION > 3 || (GTK_SOURCE_MAJOR_VERSION == 3 && GTK_SOURCE_MINOR_VERSION >= 22))
  gboolean has_wrapped_around;
  if(gtk_source_search_context_backward2(search_context, start.gobj(), match_start.gobj(), match_end.gobj(), &has_wrapped_around)) {
    get_buffer()->select_range(match_start, match_end);
    scroll_to(get_buffer()->get_insert());
  }
#else
  if(gtk_source_search_context_backward(search_context, start.gobj(), match_start.gobj(), match_end.gobj())) {
    get_buffer()->select_range(match_start, match_end);
    scroll_to(get_buffer()->get_insert());
  }
#endif
}

void Source::CommonView::replace_forward(const std::string &replacement) {
  Gtk::TextIter start, end;
  get_buffer()->get_selection_bounds(start, end);
  Gtk::TextIter match_start, match_end;
#if defined(GTK_SOURCE_MAJOR_VERSION) && (GTK_SOURCE_MAJOR_VERSION > 3 || (GTK_SOURCE_MAJOR_VERSION == 3 && GTK_SOURCE_MINOR_VERSION >= 22))
  gboolean has_wrapped_around;
  if(gtk_source_search_context_forward2(search_context, start.gobj(), match_start.gobj(), match_end.gobj(), &has_wrapped_around)) {
    auto offset = match_start.get_offset();
    gtk_source_search_context_replace2(search_context, match_start.gobj(), match_end.gobj(), replacement.c_str(), replacement.size(), nullptr);
    Glib::ustring replacement_ustring = replacement;
    get_buffer()->select_range(get_buffer()->get_iter_at_offset(offset), get_buffer()->get_iter_at_offset(offset + replacement_ustring.size()));
    scroll_to(get_buffer()->get_insert());
  }
#else
  if(gtk_source_search_context_forward(search_context, start.gobj(), match_start.gobj(), match_end.gobj())) {
    auto offset = match_start.get_offset();
    gtk_source_search_context_replace(search_context, match_start.gobj(), match_end.gobj(), replacement.c_str(), replacement.size(), nullptr);
    Glib::ustring replacement_ustring = replacement;
    get_buffer()->select_range(get_buffer()->get_iter_at_offset(offset), get_buffer()->get_iter_at_offset(offset + replacement_ustring.size()));
    scroll_to(get_buffer()->get_insert());
  }
#endif
}

void Source::CommonView::replace_backward(const std::string &replacement) {
  Gtk::TextIter start, end;
  get_buffer()->get_selection_bounds(start, end);
  Gtk::TextIter match_start, match_end;
#if defined(GTK_SOURCE_MAJOR_VERSION) && (GTK_SOURCE_MAJOR_VERSION > 3 || (GTK_SOURCE_MAJOR_VERSION == 3 && GTK_SOURCE_MINOR_VERSION >= 22))
  gboolean has_wrapped_around;
  if(gtk_source_search_context_backward2(search_context, end.gobj(), match_start.gobj(), match_end.gobj(), &has_wrapped_around)) {
    auto offset = match_start.get_offset();
    gtk_source_search_context_replace2(search_context, match_start.gobj(), match_end.gobj(), replacement.c_str(), replacement.size(), nullptr);
    get_buffer()->select_range(get_buffer()->get_iter_at_offset(offset), get_buffer()->get_iter_at_offset(offset + replacement.size()));
    scroll_to(get_buffer()->get_insert());
  }
#else
  if(gtk_source_search_context_backward(search_context, end.gobj(), match_start.gobj(), match_end.gobj())) {
    auto offset = match_start.get_offset();
    gtk_source_search_context_replace(search_context, match_start.gobj(), match_end.gobj(), replacement.c_str(), replacement.size(), nullptr);
    get_buffer()->select_range(get_buffer()->get_iter_at_offset(offset), get_buffer()->get_iter_at_offset(offset + replacement.size()));
    scroll_to(get_buffer()->get_insert());
  }
#endif
}

void Source::CommonView::replace_all(const std::string &replacement) {
  gtk_source_search_context_replace_all(search_context, replacement.c_str(), replacement.size(), nullptr);
}

void Source::CommonView::search_occurrences_updated(GtkWidget *widget, GParamSpec *property, gpointer data) {
  auto view = static_cast<Source::BaseView *>(data);
  if(view->update_search_occurrences)
    view->update_search_occurrences(gtk_source_search_context_get_occurrences_count(view->search_context));
}

void Source::CommonView::cut() {
  if(!get_editable())
    return copy();

  disable_spellcheck = true;
  ScopeGuard guard{[this] {
    disable_spellcheck = false;
  }};

  if(!get_buffer()->get_has_selection())
    cut_lines();
  else if(language_id == "diff") {
    Gtk::TextIter start, end;
    get_buffer()->get_selection_bounds(start, end);
    Glib::ustring selection;
    selection.reserve(end.get_offset() - start.get_offset());
    for(auto iter = start; iter < end; ++iter) {
      if(iter.starts_line() && *iter == '@') {
        selection = get_buffer()->get_text(start, end);
        break;
      }
      if(!(iter.starts_line() && (*iter == '-' || *iter == '+')))
        selection += *iter;
    }
    Gtk::Clipboard::get()->set_text(selection);
    get_buffer()->erase(start, end);
  }
  else
    get_buffer()->cut_clipboard(Gtk::Clipboard::get());
  keep_clipboard = true;
}

void Source::CommonView::cut_lines() {
  if(!get_editable())
    return copy_lines();

  disable_spellcheck = true;
  ScopeGuard guard{[this] {
    disable_spellcheck = false;
  }};

  Gtk::TextIter start, end;
  get_buffer()->get_selection_bounds(start, end);
  start = get_buffer()->get_iter_at_line(start.get_line());

  if(!end.ends_line())
    end.forward_to_line_end();
  end.forward_char();

  if(language_id == "diff") {
    Glib::ustring selection;
    selection.reserve(end.get_offset() - start.get_offset());
    for(auto iter = start; iter < end; ++iter) {
      if(iter.starts_line() && *iter == '@') {
        selection = get_buffer()->get_text(start, end);
        break;
      }
      if(!(iter.starts_line() && (*iter == '-' || *iter == '+')))
        selection += *iter;
    }
    if(keep_clipboard)
      Gtk::Clipboard::get()->set_text(Gtk::Clipboard::get()->wait_for_text() + selection);
    else
      Gtk::Clipboard::get()->set_text(selection);
  }
  else {
    if(keep_clipboard)
      Gtk::Clipboard::get()->set_text(Gtk::Clipboard::get()->wait_for_text() + get_buffer()->get_text(start, end));
    else
      Gtk::Clipboard::get()->set_text(get_buffer()->get_text(start, end));
  }
  get_buffer()->erase(start, end);
  keep_clipboard = true;
}

void Source::CommonView::copy() {
  if(!get_buffer()->get_has_selection())
    copy_lines();
  else if(language_id == "diff") {
    Gtk::TextIter start, end;
    get_buffer()->get_selection_bounds(start, end);
    Glib::ustring selection;
    selection.reserve(end.get_offset() - start.get_offset());
    for(auto iter = start; iter < end; ++iter) {
      if(iter.starts_line() && *iter == '@') {
        selection = get_buffer()->get_text(start, end);
        break;
      }
      if(!(iter.starts_line() && (*iter == '-' || *iter == '+')))
        selection += *iter;
    }
    Gtk::Clipboard::get()->set_text(selection);
  }
  else
    get_buffer()->copy_clipboard(Gtk::Clipboard::get());
}

void Source::CommonView::copy_lines() {
  Gtk::TextIter start, end;
  get_buffer()->get_selection_bounds(start, end);
  start = get_buffer()->get_iter_at_line(start.get_line());

  if(!end.ends_line())
    end.forward_to_line_end();
  end.forward_char();

  if(language_id == "diff") {
    Glib::ustring selection;
    selection.reserve(end.get_offset() - start.get_offset());
    for(auto iter = start; iter < end; ++iter) {
      if(iter.starts_line() && *iter == '@') {
        selection = get_buffer()->get_text(start, end);
        break;
      }
      if(!(iter.starts_line() && (*iter == '-' || *iter == '+')))
        selection += *iter;
    }
    Gtk::Clipboard::get()->set_text(selection);
  }
  else
    Gtk::Clipboard::get()->set_text(get_buffer()->get_text(start, end));
}

Source::BaseView::BaseView(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language) : CommonView(language), file_path(file_path), status_diagnostics(0, 0, 0) {
  get_style_context()->add_class("juci_source_view");

  load(true);
  get_buffer()->place_cursor(get_buffer()->get_iter_at_offset(0));

  signal_focus_in_event().connect([this](GdkEventFocus *event) {
    if(last_write_time)
      check_last_write_time();
    return false;
  });

  monitor_file();

  if(language) {
    get_source_buffer()->set_language(language);
    get_source_buffer()->set_highlight_syntax(true);
    if(is_language({"chdr", "cpphdr", "c", "cpp", "objc", "java", "js", "proto", "c-sharp", "html", "cuda",
                    "php", "rust", "swift", "go", "scala", "opencl", "json", "css", "glsl", "groovy"}))
      is_bracket_language = true;
  }

  tab_char = Config::get().source.default_tab_char;
  tab_size = Config::get().source.default_tab_size;
  if(language) {
    if(is_language({"python", "rust", "julia"})) {
      tab_char = ' ';
      tab_size = 4;
    }
    else if(language_id == "go") {
      tab_char = '\t';
      tab_size = 1;
    }
  }
  if(Config::get().source.auto_tab_char_and_size) {
    auto tab_char_and_size = find_tab_char_and_size();
    if(tab_char_and_size.second != 0) {
      tab_char = tab_char_and_size.first;
      tab_size = tab_char_and_size.second;
    }
  }
  set_tab_char_and_size(tab_char, tab_size);

#ifdef __APPLE__
  primary_modifier_mask = GDK_MOD2_MASK;
#else
  primary_modifier_mask = GDK_CONTROL_MASK;
#endif


  set_snippets();

  snippet_parameter_tag = get_buffer()->create_tag();
  Gdk::RGBA rgba;
  rgba.set_rgba(0.5, 0.5, 0.5, 0.4);
  snippet_parameter_tag->property_background_rgba() = rgba;
  snippet_parameter_tag->property_background_set() = true;

  extra_cursor_selection = get_buffer()->create_tag();

  get_buffer()->signal_mark_set().connect([this](const Gtk::TextIter &iter, const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark) {
    if(mark->get_name() == "insert") {
      keep_clipboard = false;

      if(!keep_snippet_marks)
        clear_snippet_marks();
    }
  });

  get_buffer()->signal_changed().connect([this] {
    keep_clipboard = false;
  });
}

Source::BaseView::~BaseView() {
  monitor_changed_connection.disconnect();
  delayed_monitor_changed_connection.disconnect();
}

bool Source::BaseView::load(bool not_undoable_action) {
  boost::system::error_code ec;
  last_write_time = boost::filesystem::last_write_time(file_path, ec);
  if(ec)
    last_write_time.reset();

  disable_spellcheck = true;
  if(not_undoable_action)
    get_source_buffer()->begin_not_undoable_action();
  ScopeGuard guard{[this, not_undoable_action] {
    if(not_undoable_action)
      get_source_buffer()->end_not_undoable_action();
    disable_spellcheck = false;
  }};

  if(boost::filesystem::exists(file_path, ec)) {
    try {
      auto io_channel = Glib::IOChannel::create_from_file(file_path.string(), "r");
      Glib::ustring text;
      if(get_buffer()->size() == 0) {
        Glib::IOStatus status;
        do {
          status = io_channel->read(text, 131072);
          get_buffer()->insert_at_cursor(text);
        } while(status == Glib::IOStatus::IO_STATUS_NORMAL);
      }
      else {
        io_channel->read_to_end(text);
        replace_text(text.raw());
      }
    }
    catch(const Glib::Error &error) {
      Terminal::get().print("\e[31mError\e[m: could not read file " + filesystem::get_short_path(file_path).string() + ": " + error.what() + '\n', true);
      return false;
    }
  }

  get_buffer()->set_modified(false);
  return true;
}

void Source::BaseView::replace_text(const std::string &new_text) {
  get_buffer()->begin_user_action();

  if(get_buffer()->size() == 0) {
    get_buffer()->insert_at_cursor(new_text);
    get_buffer()->end_user_action();
    return;
  }
  else if(new_text.empty()) {
    get_buffer()->set_text(new_text);
    get_buffer()->end_user_action();
    return;
  }

  auto iter = get_buffer()->get_insert()->get_iter();
  int cursor_line_nr = iter.get_line();
  int cursor_line_offset = iter.ends_line() ? std::numeric_limits<int>::max() : iter.get_line_offset();

  std::vector<std::pair<const char *, const char *>> new_lines;

  const char *line_start = new_text.c_str();
  for(const char &chr : new_text) {
    if(chr == '\n') {
      new_lines.emplace_back(line_start, &chr + 1);
      line_start = &chr + 1;
    }
  }
  if(new_text.empty() || new_text.back() != '\n')
    new_lines.emplace_back(line_start, &new_text[new_text.size()]);

  try {
    auto hunks = Git::Repository::Diff::get_hunks(get_buffer()->get_text().raw(), new_text);

    for(auto it = hunks.rbegin(); it != hunks.rend(); ++it) {
      bool place_cursor = false;
      Gtk::TextIter start;
      if(it->old_lines.second != 0) {
        start = get_buffer()->get_iter_at_line(it->old_lines.first - 1);
        auto end = get_buffer()->get_iter_at_line(it->old_lines.first - 1 + it->old_lines.second);

        if(cursor_line_nr >= start.get_line() && cursor_line_nr < end.get_line()) {
          if(it->new_lines.second != 0) {
            place_cursor = true;
            int line_diff = cursor_line_nr - start.get_line();
            cursor_line_nr += static_cast<int>(0.5 + (static_cast<float>(line_diff) / it->old_lines.second) * it->new_lines.second) - line_diff;
          }
        }

        get_buffer()->erase(start, end);
        start = get_buffer()->get_iter_at_line(it->old_lines.first - 1);
      }
      else
        start = get_buffer()->get_iter_at_line(it->old_lines.first);
      if(it->new_lines.second != 0) {
        get_buffer()->insert(start, new_lines[it->new_lines.first - 1].first, new_lines[it->new_lines.first - 1 + it->new_lines.second - 1].second);
        if(place_cursor)
          place_cursor_at_line_offset(cursor_line_nr, cursor_line_offset);
      }
    }
  }
  catch(...) {
    Terminal::get().print("\e[31mError\e[m: could not replace text in buffer\n", true);
  }

  get_buffer()->end_user_action();
}

void Source::BaseView::rename(const boost::filesystem::path &path) {
  file_path = path;

  boost::system::error_code ec;
  last_write_time = boost::filesystem::last_write_time(file_path, ec);
  if(ec)
    last_write_time.reset();
  monitor_file();

  if(update_status_file_path)
    update_status_file_path(this);
  if(update_tab_label)
    update_tab_label(this);
}

void Source::BaseView::monitor_file() {
#ifdef __APPLE__ // TODO: Gio file monitor is bugged on MacOS
  class Recursive {
  public:
    static void f(BaseView *view, boost::optional<std::time_t> previous_last_write_time = {}, bool check_called = false) {
      view->delayed_monitor_changed_connection.disconnect();
      view->delayed_monitor_changed_connection = Glib::signal_timeout().connect(
          [view, previous_last_write_time, check_called]() {
            boost::system::error_code ec;
            auto last_write_time = boost::filesystem::last_write_time(view->file_path, ec);
            if(!ec && last_write_time != view->last_write_time) {
              if(last_write_time == previous_last_write_time) { // If no change has happened in the last second (std::time_t is in seconds).
                if(!check_called)                               // To avoid several info messages when file is changed but not reloaded.
                  view->check_last_write_time(last_write_time);
                Recursive::f(view, last_write_time, true);
                return false;
              }
              Recursive::f(view, last_write_time);
              return false;
            }
            Recursive::f(view);
            return false;
          },
          1000);
    }
  };
  delayed_monitor_changed_connection.disconnect();
  if(last_write_time)
    Recursive::f(this);
#else
  if(last_write_time) {
    monitor = Gio::File::create_for_path(file_path.string())->monitor_file(Gio::FileMonitorFlags::FILE_MONITOR_NONE);
    monitor_changed_connection.disconnect();
    monitor_changed_connection = monitor->signal_changed().connect([this](const Glib::RefPtr<Gio::File> &file,
                                                                          const Glib::RefPtr<Gio::File> &,
                                                                          Gio::FileMonitorEvent monitor_event) {
      if(monitor_event != Gio::FileMonitorEvent::FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
        delayed_monitor_changed_connection.disconnect();
        delayed_monitor_changed_connection = Glib::signal_timeout().connect(
            [this]() {
              check_last_write_time();
              return false;
            },
            1000); // Has to wait 1 second (std::time_t is in seconds)
      }
    });
  }
#endif
}

void Source::BaseView::check_last_write_time(boost::optional<std::time_t> last_write_time_) {
  if(!this->last_write_time)
    return;

  if(Config::get().source.auto_reload_changed_files && !get_buffer()->get_modified()) {
    boost::system::error_code ec;
    auto last_write_time = last_write_time_.value_or(boost::filesystem::last_write_time(file_path, ec));
    if(!ec && last_write_time != this->last_write_time) {
      if(load())
        return;
    }
  }
  else if(has_focus()) {
    boost::system::error_code ec;
    auto last_write_time = last_write_time_.value_or(boost::filesystem::last_write_time(file_path, ec));
    if(!ec && last_write_time != this->last_write_time)
      Info::get().print("Caution: " + file_path.filename().string() + " was changed outside of juCi++");
  }
}

std::pair<char, unsigned> Source::BaseView::find_tab_char_and_size() {
  std::map<char, size_t> tab_chars;
  std::map<unsigned, size_t> tab_sizes;
  auto iter = get_buffer()->begin();
  long tab_count = -1;
  long last_tab_count = 0;
  bool single_quoted = false;
  bool double_quoted = false;
  // For bracket languages, TODO: add more language ids
  if(is_bracket_language && language_id != "html") {
    bool line_comment = false;
    bool comment = false;
    bool bracket_last_line = false;
    char last_char = 0;
    long last_tab_diff = -1;
    while(iter) {
      if(iter.starts_line()) {
        line_comment = false;
        single_quoted = false;
        double_quoted = false;
        tab_count = 0;
        if(last_char == '{' || (!is_c && !is_cpp && last_char == '['))
          bracket_last_line = true;
        else
          bracket_last_line = false;
      }
      if(bracket_last_line && tab_count != -1) {
        if(*iter == ' ') {
          tab_chars[' ']++;
          tab_count++;
        }
        else if(*iter == '\t') {
          tab_chars['\t']++;
          tab_count++;
        }
        else {
          auto line_iter = iter;
          char last_line_char = 0;
          while(line_iter && !line_iter.ends_line()) {
            if(*line_iter != ' ' && *line_iter != '\t')
              last_line_char = *line_iter;
            if(*line_iter == '(')
              break;
            line_iter.forward_char();
          }
          if(last_line_char == ':' || *iter == '#') {
            tab_count = 0;
            if((iter.get_line() + 1) < get_buffer()->get_line_count()) {
              iter = get_buffer()->get_iter_at_line(iter.get_line() + 1);
              continue;
            }
          }
          else if(!iter.ends_line()) {
            if(tab_count != last_tab_count)
              tab_sizes[std::abs(tab_count - last_tab_count)]++;
            last_tab_diff = std::abs(tab_count - last_tab_count);
            last_tab_count = tab_count;
            last_char = 0;
          }
        }
      }

      auto prev_iter = iter;
      prev_iter.backward_char();
      auto prev_prev_iter = prev_iter;
      prev_prev_iter.backward_char();
      if(!double_quoted && *iter == '\'' && !(*prev_iter == '\\' && *prev_prev_iter != '\\'))
        single_quoted = !single_quoted;
      else if(!single_quoted && *iter == '\"' && !(*prev_iter == '\\' && *prev_prev_iter != '\\'))
        double_quoted = !double_quoted;
      else if(!single_quoted && !double_quoted) {
        auto next_iter = iter;
        next_iter.forward_char();
        if(*iter == '/' && *next_iter == '/')
          line_comment = true;
        else if(*iter == '/' && *next_iter == '*')
          comment = true;
        else if(*iter == '*' && *next_iter == '/') {
          iter.forward_chars(2);
          comment = false;
        }
      }
      if(!single_quoted && !double_quoted && !comment && !line_comment && *iter != ' ' && *iter != '\t' && !iter.ends_line())
        last_char = *iter;
      if(!single_quoted && !double_quoted && !comment && !line_comment && (*iter == '}' || (!is_c && !is_cpp && last_char == ']')) && tab_count != -1 && last_tab_diff != -1)
        last_tab_count -= last_tab_diff;
      if(*iter != ' ' && *iter != '\t')
        tab_count = -1;

      iter.forward_char();
    }
  }
  else {
    long para_count = 0;
    while(iter) {
      if(iter.starts_line())
        tab_count = 0;
      if(tab_count != -1 && para_count == 0 && single_quoted == false && double_quoted == false) {
        if(*iter == ' ') {
          tab_chars[' ']++;
          tab_count++;
        }
        else if(*iter == '\t') {
          tab_chars['\t']++;
          tab_count++;
        }
        else if(!iter.ends_line()) {
          if(tab_count != last_tab_count)
            tab_sizes[std::abs(tab_count - last_tab_count)]++;
          last_tab_count = tab_count;
        }
      }
      auto prev_iter = iter;
      prev_iter.backward_char();
      auto prev_prev_iter = prev_iter;
      prev_prev_iter.backward_char();
      if(!double_quoted && *iter == '\'' && !(*prev_iter == '\\' && *prev_prev_iter != '\\'))
        single_quoted = !single_quoted;
      else if(!single_quoted && *iter == '\"' && !(*prev_iter == '\\' && *prev_prev_iter != '\\'))
        double_quoted = !double_quoted;
      else if(!single_quoted && !double_quoted) {
        if(*iter == '(')
          para_count++;
        else if(*iter == ')')
          para_count--;
      }
      if(*iter != ' ' && *iter != '\t')
        tab_count = -1;

      iter.forward_char();
    }
  }

  char found_tab_char = 0;
  size_t occurences = 0;
  for(auto &tab_char : tab_chars) {
    if(tab_char.second > occurences) {
      found_tab_char = tab_char.first;
      occurences = tab_char.second;
    }
  }
  unsigned found_tab_size = 0;
  occurences = 0;
  for(auto &tab_size : tab_sizes) {
    if(tab_size.second > occurences) {
      found_tab_size = tab_size.first;
      occurences = tab_size.second;
    }
  }
  return {found_tab_char, found_tab_size};
}

void Source::BaseView::set_tab_char_and_size(char tab_char_, unsigned tab_size_) {
  tab_char = tab_char_;
  tab_size = tab_size_;

  tab.clear();
  for(unsigned c = 0; c < tab_size; c++)
    tab += tab_char;
}

Gtk::TextIter Source::BaseView::get_iter_at_line_pos(int line, int pos) {
  return get_iter_at_line_index(line, pos);
}

Gtk::TextIter Source::BaseView::get_iter_at_line_offset(int line, int offset) {
  line = std::min(line, get_buffer()->get_line_count() - 1);
  if(line < 0)
    line = 0;
  auto iter = get_iter_at_line_end(line);
  offset = std::min(offset, iter.get_line_offset());
  if(offset < 0)
    offset = 0;
  return get_buffer()->get_iter_at_line_offset(line, offset);
}

Gtk::TextIter Source::BaseView::get_iter_at_line_index(int line, int index) {
  line = std::min(line, get_buffer()->get_line_count() - 1);
  if(line < 0)
    line = 0;
  auto iter = get_iter_at_line_end(line);
  index = std::min(index, iter.get_line_index());
  if(index < 0)
    index = 0;
  return get_buffer()->get_iter_at_line_index(line, index);
}

Gtk::TextIter Source::BaseView::get_iter_at_line_end(int line_nr) {
  if(line_nr >= get_buffer()->get_line_count())
    return get_buffer()->end();
  else if(line_nr + 1 < get_buffer()->get_line_count()) {
    auto iter = get_buffer()->get_iter_at_line(line_nr + 1);
    iter.backward_char();
    return iter;
  }
  else {
    auto iter = get_buffer()->get_iter_at_line(line_nr);
    while(!iter.ends_line() && iter.forward_char()) {
    }
    return iter;
  }
}

void Source::BaseView::place_cursor_at_line_pos(int line, int pos) {
  get_buffer()->place_cursor(get_iter_at_line_pos(line, pos));
}

void Source::BaseView::place_cursor_at_line_offset(int line, int offset) {
  get_buffer()->place_cursor(get_iter_at_line_offset(line, offset));
}

void Source::BaseView::place_cursor_at_line_index(int line, int index) {
  get_buffer()->place_cursor(get_iter_at_line_index(line, index));
}

Gtk::TextIter Source::BaseView::get_smart_home_iter(const Gtk::TextIter &iter) {
  auto start_line_iter = get_buffer()->get_iter_at_line(iter.get_line());
  auto start_sentence_iter = start_line_iter;
  while(!start_sentence_iter.ends_line() &&
        (*start_sentence_iter == ' ' || *start_sentence_iter == '\t') &&
        start_sentence_iter.forward_char()) {
  }

  if(iter > start_sentence_iter || iter == start_line_iter)
    return start_sentence_iter;
  else
    return start_line_iter;
}

Gtk::TextIter Source::BaseView::get_smart_end_iter(const Gtk::TextIter &iter) {
  auto end_line_iter = get_iter_at_line_end(iter.get_line());
  auto end_sentence_iter = end_line_iter;
  while(!end_sentence_iter.starts_line() &&
        (*end_sentence_iter == ' ' || *end_sentence_iter == '\t' || end_sentence_iter.ends_line()) &&
        end_sentence_iter.backward_char()) {
  }
  if(!end_sentence_iter.ends_line() && *end_sentence_iter != ' ' && *end_sentence_iter != '\t')
    end_sentence_iter.forward_char();

  if(iter == end_line_iter)
    return end_sentence_iter;
  else
    return end_line_iter;
}

std::string Source::BaseView::get_line(const Gtk::TextIter &iter) {
  auto line_start_it = get_buffer()->get_iter_at_line(iter.get_line());
  auto line_end_it = get_iter_at_line_end(iter.get_line());
  std::string line(get_buffer()->get_text(line_start_it, line_end_it));
  return line;
}
std::string Source::BaseView::get_line(const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark) {
  return get_line(mark->get_iter());
}
std::string Source::BaseView::get_line(int line_nr) {
  return get_line(get_buffer()->get_iter_at_line(line_nr));
}
std::string Source::BaseView::get_line() {
  return get_line(get_buffer()->get_insert());
}

std::string Source::BaseView::get_line_before(const Gtk::TextIter &iter) {
  auto line_it = get_buffer()->get_iter_at_line(iter.get_line());
  std::string line(get_buffer()->get_text(line_it, iter));
  return line;
}
std::string Source::BaseView::get_line_before(const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark) {
  return get_line_before(mark->get_iter());
}
std::string Source::BaseView::get_line_before() {
  return get_line_before(get_buffer()->get_insert());
}

Gtk::TextIter Source::BaseView::get_tabs_end_iter(const Gtk::TextIter &iter) {
  return get_tabs_end_iter(iter.get_line());
}
Gtk::TextIter Source::BaseView::get_tabs_end_iter(const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark) {
  return get_tabs_end_iter(mark->get_iter());
}
Gtk::TextIter Source::BaseView::get_tabs_end_iter(int line_nr) {
  auto sentence_iter = get_buffer()->get_iter_at_line(line_nr);
  while((*sentence_iter == ' ' || *sentence_iter == '\t') && !sentence_iter.ends_line() && sentence_iter.forward_char()) {
  }
  return sentence_iter;
}
Gtk::TextIter Source::BaseView::get_tabs_end_iter() {
  return get_tabs_end_iter(get_buffer()->get_insert());
}

bool Source::BaseView::is_token_char(gunichar chr) {
  return (chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z') || (chr >= '0' && chr <= '9') || chr == '_' || chr == '$' || chr >= 128 ||
         (language_id == "css" && chr == '-');
}

std::pair<Gtk::TextIter, Gtk::TextIter> Source::BaseView::get_token_iters(Gtk::TextIter iter) {
  auto start = iter;
  auto end = iter;

  while(iter.backward_char() && is_token_char(*iter))
    start = iter;
  while(is_token_char(*end) && end.forward_char()) {
  }

  return {start, end};
}

std::string Source::BaseView::get_token(const Gtk::TextIter &iter) {
  auto range = get_token_iters(iter);
  return get_buffer()->get_text(range.first, range.second);
}

std::string Source::BaseView::get_token(const Glib::ustring &string, size_t pos) {
  if(pos >= string.size())
    return {};
  auto start = pos;
  auto end = pos;
  for(auto i = pos - 1; i != Glib::ustring::npos && is_token_char(string[i]); --i)
    start = pos;
  for(; end < string.size() && is_token_char(string[end]); ++end) {
  }
  return string.substr(start, end - start);
}

void Source::BaseView::remove_trailing_whitespace_characters() {
  auto buffer = get_buffer();
  for(int line = 0; line < buffer->get_line_count(); line++) {
    auto iter = buffer->get_iter_at_line(line);
    auto end_iter = get_iter_at_line_end(line);
    if(iter == end_iter)
      continue;
    iter = end_iter;
    while(!iter.starts_line() && (*iter == ' ' || *iter == '\t' || iter.ends_line()))
      iter.backward_char();
    if(*iter != ' ' && *iter != '\t')
      iter.forward_char();
    if(iter == end_iter)
      continue;
    buffer->erase(iter, end_iter);
  }
}

void Source::BaseView::add_missing_newline_at_end_of_file() {
  auto iter = get_buffer()->end();
  if(!iter.starts_line())
    get_buffer()->insert(iter, "\n");
}

void Source::BaseView::remove_whitespace_characters(const Gtk::TextIter &iter) {
  auto start_blank_iter = iter;
  auto end_blank_iter = iter;
  while((*end_blank_iter == ' ' || *end_blank_iter == '\t') &&
        !end_blank_iter.ends_line() && end_blank_iter.forward_char()) {
  }
  if(!start_blank_iter.starts_line()) {
    start_blank_iter.backward_char();
    while((*start_blank_iter == ' ' || *start_blank_iter == '\t') &&
          !start_blank_iter.starts_line() && start_blank_iter.backward_char()) {
    }
    if(*start_blank_iter != ' ' && *start_blank_iter != '\t')
      start_blank_iter.forward_char();
  }

  if(start_blank_iter.starts_line())
    get_buffer()->erase(iter, end_blank_iter);
  else
    get_buffer()->erase(start_blank_iter, end_blank_iter);
}

void Source::BaseView::paste() {
  disable_spellcheck = true;
  ScopeGuard spellcheck_guard{[this] {
    disable_spellcheck = false;
  }};

  if(CompletionDialog::get())
    CompletionDialog::get()->hide();

  enable_multiple_cursors = true;
  ScopeGuard multiple_cursors_guard{[this] {
    enable_multiple_cursors = false;
  }};

  std::string text = Gtk::Clipboard::get()->wait_for_text();

  // Replace carriage returns (which leads to crash) with newlines
  for(size_t c = 0; c < text.size(); c++) {
    if(text[c] == '\r') {
      if((c + 1) < text.size() && text[c + 1] == '\n')
        text.replace(c, 2, "\n");
      else
        text.replace(c, 1, "\n");
    }
  }

  // Exception for when pasted text is only whitespaces
  bool only_whitespaces = true;
  for(auto &chr : text) {
    if(chr != '\n' && chr != '\r' && chr != ' ' && chr != '\t') {
      only_whitespaces = false;
      break;
    }
  }
  if(only_whitespaces) {
    Gtk::Clipboard::get()->set_text(text);
    get_buffer()->paste_clipboard(Gtk::Clipboard::get());
    scroll_to_cursor_delayed(false, false);
    return;
  }

  get_buffer()->begin_user_action();
  if(get_buffer()->get_has_selection()) {
    Gtk::TextIter start, end;
    get_buffer()->get_selection_bounds(start, end);
    get_buffer()->erase(start, end);
  }
  auto iter = get_buffer()->get_insert()->get_iter();
  auto tabs_end_iter = get_tabs_end_iter();
  auto prefix_tabs = get_line_before(iter < tabs_end_iter ? iter : tabs_end_iter);

  size_t start_line = 0;
  size_t end_line = 0;
  bool paste_line = false;
  bool first_paste_line = true;
  auto paste_line_tabs = static_cast<size_t>(-1);
  bool first_paste_line_has_tabs = false;
  for(size_t c = 0; c < text.size(); c++) {
    if(text[c] == '\n') {
      end_line = c;
      paste_line = true;
    }
    else if(c == text.size() - 1) {
      end_line = c + 1;
      paste_line = true;
    }
    if(paste_line) {
      bool empty_line = true;
      std::string line = text.substr(start_line, end_line - start_line);
      size_t tabs = 0;
      for(auto chr : line) {
        if(chr == tab_char)
          tabs++;
        else {
          empty_line = false;
          break;
        }
      }
      if(first_paste_line) {
        if(tabs != 0) {
          first_paste_line_has_tabs = true;
          paste_line_tabs = tabs;
        }
        else if(language_id == "python") { // Special case for Python code where the first line ends with ':'
          char last_char = 0;
          for(auto &chr : line) {
            if(chr != ' ' && chr != '\t')
              last_char = chr;
          }
          if(last_char == ':') {
            first_paste_line_has_tabs = true;
            paste_line_tabs = tabs;
          }
        }
        first_paste_line = false;
      }
      else if(!empty_line)
        paste_line_tabs = std::min(paste_line_tabs, tabs);

      start_line = end_line + 1;
      paste_line = false;
    }
  }
  if(paste_line_tabs == static_cast<size_t>(-1))
    paste_line_tabs = 0;
  start_line = 0;
  end_line = 0;
  paste_line = false;
  first_paste_line = true;
  for(size_t c = 0; c < text.size(); c++) {
    if(text[c] == '\n') {
      end_line = c;
      paste_line = true;
    }
    else if(c == text.size() - 1) {
      end_line = c + 1;
      paste_line = true;
    }
    if(paste_line) {
      std::string line = text.substr(start_line, end_line - start_line);
      size_t line_tabs = 0;
      for(auto chr : line) {
        if(chr == tab_char)
          line_tabs++;
        else
          break;
      }
      auto tabs = paste_line_tabs;
      if(!(first_paste_line && !first_paste_line_has_tabs) && line_tabs < paste_line_tabs) {
        tabs = line_tabs;
      }

      if(first_paste_line) {
        if(first_paste_line_has_tabs)
          get_buffer()->insert_at_cursor(text.substr(start_line + tabs, end_line - start_line - tabs));
        else
          get_buffer()->insert_at_cursor(text.substr(start_line, end_line - start_line));
        first_paste_line = false;
      }
      else
        get_buffer()->insert_at_cursor('\n' + prefix_tabs + text.substr(start_line + tabs, end_line - start_line - tabs));
      start_line = end_line + 1;
      paste_line = false;
    }
  }
  // Add final newline if present in text
  if(text.size() > 0 && text.back() == '\n')
    get_buffer()->insert_at_cursor('\n' + prefix_tabs);
  get_buffer()->end_user_action();
  scroll_to_cursor_delayed(false, false);
}

std::string Source::BaseView::get_selected_text() {
  Gtk::TextIter start, end;
  if(!get_buffer()->get_selection_bounds(start, end)) {
    return {};
  }
  return get_buffer()->get_text(start, end);
}

bool Source::BaseView::on_key_press_event(GdkEventKey *event) {
  // Move cursor one paragraph down
  if((event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_KP_Down) && (event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_META_MASK)) == GDK_CONTROL_MASK) {
    auto selection_start_iter = get_buffer()->get_selection_bound()->get_iter();
    auto iter = get_buffer()->get_iter_at_line(get_buffer()->get_insert()->get_iter().get_line());
    bool empty_line = false;
    bool text_found = false;
    for(;;) {
      if(!iter)
        break;
      if(iter.starts_line())
        empty_line = true;
      if(empty_line && !iter.ends_line() && *iter != ' ' && *iter != '\t')
        empty_line = false;
      if(!text_found && !iter.ends_line() && *iter != ' ' && *iter != '\t')
        text_found = true;
      if(empty_line && text_found && iter.ends_line())
        break;
      iter.forward_char();
    }
    iter = get_buffer()->get_iter_at_line(iter.get_line());
    if((event->state & GDK_SHIFT_MASK) > 0)
      get_buffer()->select_range(iter, selection_start_iter);
    else
      get_buffer()->place_cursor(iter);
    scroll_to(get_buffer()->get_insert());
    return true;
  }
  // Move cursor one paragraph up
  else if((event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up) && (event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_META_MASK)) == GDK_CONTROL_MASK) {
    auto selection_start_iter = get_buffer()->get_selection_bound()->get_iter();
    auto iter = get_buffer()->get_iter_at_line(get_buffer()->get_insert()->get_iter().get_line());
    iter.backward_char();
    bool empty_line = false;
    bool text_found = false;
    bool move_to_start = false;
    for(;;) {
      if(!iter)
        break;
      if(iter.ends_line())
        empty_line = true;
      if(empty_line && !iter.ends_line() && *iter != ' ' && *iter != '\t')
        empty_line = false;
      if(!text_found && !iter.ends_line() && *iter != ' ' && *iter != '\t')
        text_found = true;
      if(empty_line && text_found && iter.starts_line())
        break;
      if(iter.is_start()) {
        move_to_start = true;
        break;
      }
      iter.backward_char();
    }
    if(empty_line && !move_to_start) {
      iter = get_iter_at_line_end(iter.get_line());
      iter.forward_char();
      if(!iter.starts_line()) // For CR+LF
        iter.forward_char();
    }
    if((event->state & GDK_SHIFT_MASK) > 0)
      get_buffer()->select_range(iter, selection_start_iter);
    else
      get_buffer()->place_cursor(iter);
    scroll_to(get_buffer()->get_insert());
    return true;
  }

  // Smart Home
  if((event->keyval == GDK_KEY_Home || event->keyval == GDK_KEY_KP_Home) && (event->state & GDK_CONTROL_MASK) == 0) {
    enable_multiple_cursors = false;
    for(auto &extra_cursor : extra_cursors) {
      extra_cursor.move(get_smart_home_iter(extra_cursor.insert->get_iter()), event->state & GDK_SHIFT_MASK);
      extra_cursor.line_offset = extra_cursor.insert->get_iter().get_line_offset();
    }
    auto insert = get_buffer()->get_insert();
    auto iter = get_smart_home_iter(get_buffer()->get_insert()->get_iter());
    get_buffer()->move_mark(insert, iter);
    if((event->state & GDK_SHIFT_MASK) == 0)
      get_buffer()->move_mark_by_name("selection_bound", iter);
    scroll_to(get_buffer()->get_insert());
    enable_multiple_cursors = true;
    return true;
  }
  // Smart End
  if((event->keyval == GDK_KEY_End || event->keyval == GDK_KEY_KP_End) && (event->state & GDK_CONTROL_MASK) == 0) {
    enable_multiple_cursors = false;
    for(auto &extra_cursor : extra_cursors) {
      extra_cursor.move(get_smart_end_iter(extra_cursor.insert->get_iter()), event->state & GDK_SHIFT_MASK);
      extra_cursor.line_offset = std::max(extra_cursor.line_offset, extra_cursor.insert->get_iter().get_line_offset());
    }
    auto insert = get_buffer()->get_insert();
    auto iter = get_smart_end_iter(get_buffer()->get_insert()->get_iter());
    get_buffer()->move_mark(insert, iter);
    if((event->state & GDK_SHIFT_MASK) == 0)
      get_buffer()->move_mark_by_name("selection_bound", iter);
    scroll_to(get_buffer()->get_insert());
    enable_multiple_cursors = true;
    return true;
  }

  // Move cursors left one word
  if((event->keyval == GDK_KEY_Left || event->keyval == GDK_KEY_KP_Left) && (event->state & GDK_CONTROL_MASK) > 0) {
    enable_multiple_cursors = false;
    for(auto &extra_cursor : extra_cursors) {
      auto iter = extra_cursor.insert->get_iter();
      iter.backward_word_start();
      extra_cursor.line_offset = iter.get_line_offset();
      extra_cursor.move(iter, event->state & GDK_SHIFT_MASK);
    }
    auto insert = get_buffer()->get_insert();
    auto iter = insert->get_iter();
    iter.backward_word_start();
    get_buffer()->move_mark(insert, iter);
    if((event->state & GDK_SHIFT_MASK) == 0)
      get_buffer()->move_mark_by_name("selection_bound", iter);
    enable_multiple_cursors = true;
    return true;
  }
  // Move cursors right one word
  if((event->keyval == GDK_KEY_Right || event->keyval == GDK_KEY_KP_Right) && (event->state & GDK_CONTROL_MASK) > 0) {
    enable_multiple_cursors = false;
    for(auto &extra_cursor : extra_cursors) {
      auto iter = extra_cursor.insert->get_iter();
      iter.forward_word_end();
      extra_cursor.line_offset = iter.get_line_offset();
      extra_cursor.move(iter, event->state & GDK_SHIFT_MASK);
      extra_cursor.insert->get_buffer()->apply_tag(extra_cursor_selection, extra_cursor.insert->get_iter(), extra_cursor.selection_bound->get_iter());
    }
    auto insert = get_buffer()->get_insert();
    auto iter = insert->get_iter();
    iter.forward_word_end();
    get_buffer()->move_mark(insert, iter);
    if((event->state & GDK_SHIFT_MASK) == 0)
      get_buffer()->move_mark_by_name("selection_bound", iter);
    enable_multiple_cursors = true;
    return true;
  }

  return Source::CommonView::on_key_press_event(event);
}

bool Source::BaseView::on_key_press_event_extra_cursors(GdkEventKey *event) {
  if(event->keyval == GDK_KEY_Escape) {
    if(clear_snippet_marks())
      return true;
    if(!extra_cursors.empty()) {
      extra_cursors.clear();
      return true;
    }
  }

  if(!Config::get().source.enable_multiple_cursors)
    return false;

  setup_extra_cursor_signals();

  unsigned create_cursor_mask = GDK_MOD1_MASK;
  unsigned move_last_created_cursor_mask = GDK_SHIFT_MASK | GDK_MOD1_MASK;

  // Move last created cursor
  if((event->keyval == GDK_KEY_Left || event->keyval == GDK_KEY_KP_Left) && (event->state & move_last_created_cursor_mask) == move_last_created_cursor_mask) {
    if(extra_cursors.empty())
      return false;
    auto iter = extra_cursors.back().insert->get_iter();
    iter.backward_char();
    extra_cursors.back().move(iter, false);
    extra_cursors.back().line_offset = iter.get_line_offset();
    return true;
  }
  if((event->keyval == GDK_KEY_Right || event->keyval == GDK_KEY_KP_Right) && (event->state & move_last_created_cursor_mask) == move_last_created_cursor_mask) {
    if(extra_cursors.empty())
      return false;
    auto iter = extra_cursors.back().insert->get_iter();
    iter.forward_char();
    extra_cursors.back().move(iter, false);
    extra_cursors.back().line_offset = iter.get_line_offset();
    return true;
  }
  if((event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up) && (event->state & move_last_created_cursor_mask) == move_last_created_cursor_mask) {
    if(extra_cursors.empty())
      return false;
    auto iter = extra_cursors.back().insert->get_iter();
    iter.backward_line();
    auto end_line_iter = iter;
    if(!end_line_iter.ends_line())
      end_line_iter.forward_to_line_end();
    iter.forward_chars(std::min(extra_cursors.back().line_offset, end_line_iter.get_line_offset()));
    extra_cursors.back().move(iter, false);
    return true;
  }
  if((event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_KP_Down) && (event->state & move_last_created_cursor_mask) == move_last_created_cursor_mask) {
    if(extra_cursors.empty())
      return false;
    auto iter = extra_cursors.back().insert->get_iter();
    iter.forward_line();
    auto end_line_iter = iter;
    if(!end_line_iter.ends_line())
      end_line_iter.forward_to_line_end();
    iter.forward_chars(std::min(extra_cursors.back().line_offset, end_line_iter.get_line_offset()));
    extra_cursors.back().move(iter, false);
    return true;
  }

  // Create extra cursor
  if((event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up) && (event->state & create_cursor_mask) == create_cursor_mask) {
    auto iter = get_buffer()->get_insert()->get_iter();
    auto insert_line_offset = iter.get_line_offset();
    auto offset = iter.get_offset();
    for(auto &extra_cursor : extra_cursors) { // Find topmost cursor
      if(!extra_cursor.snippet)
        offset = std::min(offset, extra_cursor.insert->get_iter().get_offset());
    }
    auto test_iter = get_buffer()->get_iter_at_offset(offset);
    iter = test_iter;
    iter.backward_line();
    if(iter.get_line() != test_iter.get_line()) {
      auto end_line_iter = iter;
      if(!end_line_iter.ends_line())
        end_line_iter.forward_to_line_end();
      iter.forward_chars(std::min(insert_line_offset, end_line_iter.get_line_offset()));
      extra_cursors.emplace_back(extra_cursor_selection, iter, false, insert_line_offset);
    }
    return true;
  }
  if((event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_KP_Down) && (event->state & create_cursor_mask) == create_cursor_mask) {
    auto iter = get_buffer()->get_insert()->get_iter();
    auto insert_line_offset = iter.get_line_offset();
    auto offset = iter.get_offset();
    for(auto &extra_cursor : extra_cursors) { // Find bottommost cursor
      if(!extra_cursor.snippet)
        offset = std::max(offset, extra_cursor.insert->get_iter().get_offset());
    }
    auto test_iter = get_buffer()->get_iter_at_offset(offset);
    iter = test_iter;
    iter.forward_line();
    if(iter.get_line() != test_iter.get_line()) {
      auto end_line_iter = iter;
      if(!end_line_iter.ends_line())
        end_line_iter.forward_to_line_end();
      iter.forward_chars(std::min(insert_line_offset, end_line_iter.get_line_offset()));
      extra_cursors.emplace_back(extra_cursor_selection, iter, false, insert_line_offset);
    }
    return true;
  }

  // Move cursors up/down
  if((event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up)) {
    enable_multiple_cursors = false;
    for(auto &extra_cursor : extra_cursors) {
      auto iter = extra_cursor.insert->get_iter();
      iter.backward_line();
      auto end_line_iter = iter;
      if(!end_line_iter.ends_line())
        end_line_iter.forward_to_line_end();
      iter.forward_chars(std::min(extra_cursor.line_offset, end_line_iter.get_line_offset()));
      extra_cursor.move(iter, event->state & GDK_SHIFT_MASK);
    }
    return false;
  }
  if((event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_KP_Down)) {
    enable_multiple_cursors = false;
    for(auto &extra_cursor : extra_cursors) {
      auto iter = extra_cursor.insert->get_iter();
      iter.forward_line();
      auto end_line_iter = iter;
      if(!end_line_iter.ends_line())
        end_line_iter.forward_to_line_end();
      iter.forward_chars(std::min(extra_cursor.line_offset, end_line_iter.get_line_offset()));
      extra_cursor.move(iter, event->state & GDK_SHIFT_MASK);
    }
    return false;
  }

  // Move cursors left/right of selection
  if((event->keyval == GDK_KEY_Left || event->keyval == GDK_KEY_KP_Left)) {
    enable_multiple_cursors = false;
    bool shift_pressed = (event->state & GDK_SHIFT_MASK) != 0;
    for(auto &extra_cursor : extra_cursors) {
      auto iter = extra_cursor.insert->get_iter();
      if(!shift_pressed && get_buffer()->get_has_selection())
        iter = std::min(iter, extra_cursor.selection_bound->get_iter());
      else
        iter.backward_char();
      extra_cursor.move(iter, shift_pressed);
    }
    return false;
  }
  if((event->keyval == GDK_KEY_Right || event->keyval == GDK_KEY_KP_Right)) {
    enable_multiple_cursors = false;
    bool shift_pressed = (event->state & GDK_SHIFT_MASK) != 0;
    for(auto &extra_cursor : extra_cursors) {
      auto iter = extra_cursor.insert->get_iter();
      if(!shift_pressed && get_buffer()->get_has_selection())
        iter = std::max(iter, extra_cursor.selection_bound->get_iter());
      else
        iter.forward_char();
      extra_cursor.move(iter, shift_pressed);
    }
    return false;
  }

  return false;
}

void Source::BaseView::setup_extra_cursor_signals() {
  if(extra_cursors_signals_set)
    return;
  extra_cursors_signals_set = true;

  extra_cursor_selection->set_priority(get_buffer()->get_tag_table()->get_size() - 1);

  auto last_insert = std::make_shared<Mark>(get_buffer()->get_insert()->get_iter(), false);
  auto last_selection_bound = std::make_shared<Mark>(get_buffer()->get_selection_bound()->get_iter(), false);
  get_buffer()->signal_mark_set().connect([this, last_insert, last_selection_bound](const Gtk::TextIter &iter, const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark) mutable {
    if(mark->get_name() == "insert") {
      if(enable_multiple_cursors || enable_multiple_cursors_placements) {
        auto set_enable_multiple_cursors = enable_multiple_cursors;
        if(set_enable_multiple_cursors)
          enable_multiple_cursors = false;
        auto offset_diff = mark->get_iter().get_offset() - (*last_insert)->get_iter().get_offset();
        if(offset_diff != 0) {
          for(auto &extra_cursor : extra_cursors) {
            auto iter = extra_cursor.insert->get_iter();
            iter.forward_chars(offset_diff);
            extra_cursor.move(iter, true);
            extra_cursor.line_offset = iter.get_line_offset();
          }
        }
        if(set_enable_multiple_cursors)
          enable_multiple_cursors = true;
      }
      get_buffer()->move_mark(*last_insert, mark->get_iter());
    }

    if(mark->get_name() == "selection_bound") {
      if(enable_multiple_cursors || enable_multiple_cursors_placements) {
        auto set_enable_multiple_cursors = enable_multiple_cursors;
        if(set_enable_multiple_cursors)
          enable_multiple_cursors = false;
        auto offset_diff = mark->get_iter().get_offset() - (*last_selection_bound)->get_iter().get_offset();
        if(offset_diff != 0) {
          for(auto &extra_cursor : extra_cursors) {
            auto iter = extra_cursor.selection_bound->get_iter();
            iter.forward_chars(offset_diff);
            extra_cursor.move_selection_bound(iter);
          }
        }
        if(set_enable_multiple_cursors)
          enable_multiple_cursors = true;
      }
      get_buffer()->move_mark(*last_selection_bound, mark->get_iter());
    }
  });

  get_buffer()->signal_insert().connect([this](const Gtk::TextIter &iter, const Glib::ustring &text, int bytes) {
    if(enable_multiple_cursors && !extra_cursors.empty()) {
      enable_multiple_cursors = false;
      auto offset = iter.get_offset() - get_buffer()->get_insert()->get_iter().get_offset();
      if(offset > 0)
        offset -= text.size();
      for(auto &extra_cursor : extra_cursors) {
        auto start_iter = extra_cursor.insert->get_iter();
        auto end_iter = extra_cursor.selection_bound->get_iter();
        if(start_iter != end_iter) // Erase selection if any
          get_buffer()->erase(start_iter, end_iter);

        auto iter = extra_cursor.insert->get_iter();
        iter.forward_chars(offset);
        get_buffer()->insert(iter, text);

        extra_cursor.line_offset = extra_cursor.insert->get_iter().get_line_offset();
      }
      enable_multiple_cursors = true;
    }
  });

  auto erase_backward_length = std::make_shared<int>(0);
  auto erase_forward_length = std::make_shared<int>(0);
  auto erase_selection = std::make_shared<bool>(false);
  get_buffer()->signal_erase().connect(
      [this, erase_backward_length, erase_forward_length, erase_selection](const Gtk::TextIter &iter_start, const Gtk::TextIter &iter_end) {
        if(enable_multiple_cursors && (!extra_cursors.empty())) {
          auto insert_offset = get_buffer()->get_insert()->get_iter().get_offset();
          *erase_backward_length = insert_offset - iter_start.get_offset();
          *erase_forward_length = iter_end.get_offset() - insert_offset;

          if(*erase_backward_length == 0) {
            auto iter = get_buffer()->get_insert()->get_iter();
            iter.forward_chars(*erase_forward_length);
            if(iter == get_buffer()->get_selection_bound()->get_iter())
              *erase_selection = true;
          }
          else if(*erase_forward_length == 0) {
            auto iter = get_buffer()->get_insert()->get_iter();
            iter.backward_chars(*erase_backward_length);
            if(iter == get_buffer()->get_selection_bound()->get_iter())
              *erase_selection = true;
          }
        }
      },
      false);
  get_buffer()->signal_erase().connect([this, erase_backward_length, erase_forward_length, erase_selection](const Gtk::TextIter & /*iter_start*/, const Gtk::TextIter & /*iter_end*/) {
    if(enable_multiple_cursors && (*erase_backward_length != 0 || *erase_forward_length != 0)) {
      enable_multiple_cursors = false;
      for(auto &extra_cursor : extra_cursors) {
        if(*erase_selection)
          get_buffer()->erase(extra_cursor.insert->get_iter(), extra_cursor.selection_bound->get_iter());
        else {
          auto start_iter = extra_cursor.insert->get_iter();
          auto end_iter = extra_cursor.selection_bound->get_iter();
          if(start_iter != end_iter) // Erase selection if any
            get_buffer()->erase(start_iter, end_iter);

          start_iter = extra_cursor.insert->get_iter();
          end_iter = start_iter;
          end_iter.forward_chars(*erase_forward_length);
          start_iter.backward_chars(*erase_backward_length);
          get_buffer()->erase(start_iter, end_iter);

          extra_cursor.line_offset = extra_cursor.insert->get_iter().get_line_offset();
        }
      }
      enable_multiple_cursors = true;
      *erase_backward_length = 0;
      *erase_forward_length = 0;
      *erase_selection = false;
    }
  });
}

void Source::BaseView::set_snippets() {
  LockGuard lock(snippets_mutex);

  snippets = nullptr;

  if(!language_id.empty()) {
    for(auto &pair : Snippets::get().snippets) {
      std::smatch sm;
      if(std::regex_match(language_id, sm, pair.first)) {
        snippets = &pair.second;
        break;
      }
    }
  }
}

void Source::BaseView::insert_snippet(Gtk::TextIter iter, const std::string &snippet) {
  std::map<size_t, std::vector<std::pair<size_t, size_t>>> parameter_offsets_and_sizes_map;

  std::string insert;
  insert.reserve(snippet.size());

  bool erase_line = false, erase_word = false;

  size_t i = 0;

  auto parse_number = [&](int &number) {
    if(i >= snippet.size())
      throw std::out_of_range("unexpected end");
    std::string str;
    for(; i < snippet.size() && snippet[i] >= '0' && snippet[i] <= '9'; ++i)
      str += snippet[i];
    if(str.empty())
      return false;
    try {
      number = std::stoi(str);
      return true;
    }
    catch(...) {
      return false;
    }
  };
  auto compare_variable = [&](const char *text) {
    if(starts_with(snippet, i, text)) {
      i += strlen(text);
      return true;
    }
    return false;
  };

  enum class ParseVariableResult {
    parsed,
    skipped,
    not_found
  };

  auto parse_variable = [&]() -> ParseVariableResult {
    if(i >= snippet.size())
      throw std::out_of_range("unexpected end");
    if(compare_variable("TM_SELECTED_TEXT")) {
      Gtk::TextIter start, end;
      if(get_buffer()->get_selection_bounds(start, end)) {
        insert += get_buffer()->get_text(start, end);
        return ParseVariableResult::parsed;
      }
      return ParseVariableResult::skipped;
    }
    else if(compare_variable("TM_CURRENT_LINE")) {
      auto start = get_buffer()->get_iter_at_line(iter.get_line());
      auto end = get_iter_at_line_end(iter.get_line());
      insert += get_buffer()->get_text(start, end);
      erase_line = true;
      return ParseVariableResult::parsed;
    }
    else if(compare_variable("TM_CURRENT_WORD")) {
      if(is_token_char(*iter)) {
        auto token_iters = get_token_iters(iter);
        insert += get_buffer()->get_text(token_iters.first, token_iters.second);
        erase_word = true;
        return ParseVariableResult::parsed;
      }
      return ParseVariableResult::skipped;
    }
    else if(compare_variable("TM_LINE_INDEX")) {
      insert += std::to_string(iter.get_line());
      return ParseVariableResult::parsed;
    }
    else if(compare_variable("TM_LINE_NUMBER")) {
      insert += std::to_string(iter.get_line() + 1);
      return ParseVariableResult::parsed;
    }
    else if(compare_variable("TM_FILENAME_BASE")) {
      insert += file_path.stem().string();
      return ParseVariableResult::parsed;
    }
    else if(compare_variable("TM_FILENAME")) {
      insert += file_path.filename().string();
      return ParseVariableResult::parsed;
    }
    else if(compare_variable("TM_DIRECTORY")) {
      insert += file_path.parent_path().string();
      return ParseVariableResult::parsed;
    }
    else if(compare_variable("TM_FILEPATH")) {
      insert += file_path.string();
      return ParseVariableResult::parsed;
    }
    else if(compare_variable("CLIPBOARD")) {
      insert += Gtk::Clipboard::get()->wait_for_text();
      return ParseVariableResult::parsed;
    }
    // TODO: support other variables
    return ParseVariableResult::not_found;
  };

  std::function<void(bool)> parse_snippet = [&](bool stop_at_curly_end) {
    int number;
    for(; i < snippet.size() && !(stop_at_curly_end && snippet[i] == '}');) {
      if(snippet[i] == '\\') {
        if(i + 1 < snippet.size() &&
           (snippet[i + 1] == '$' || snippet[i + 1] == '`' || snippet[i + 1] == '\\' || (stop_at_curly_end && snippet[i + 1] == '}'))) {
          insert += snippet[i + 1];
          i += 2;
        }
        else {
          insert += '\\';
          ++i;
        }
      }
      else if(snippet[i] == '$') {
        ++i;
        if(snippet.at(i) == '{') {
          ++i;
          int number;
          if(parse_number(number)) {
            std::string placeholder;
            if(snippet.at(i) == ':') {
              ++i;
              auto pos = insert.size();
              parse_snippet(true);
              placeholder = insert.substr(pos);
            }
            if(snippet.at(i) != '}')
              throw std::logic_error("closing } not found");
            ++i;
            auto placeholder_character_count = utf8_character_count(placeholder);
            parameter_offsets_and_sizes_map[number].emplace_back(utf8_character_count(insert) - placeholder_character_count, placeholder_character_count);
          }
          else {
            if(parse_variable() != ParseVariableResult::parsed) {
              if(snippet.at(i) == ':') { // Use default value
                ++i;
                if(parse_variable() != ParseVariableResult::parsed)
                  parse_snippet(true);
              }
            }
            else if(snippet.at(i) == ':') { // Skip default value
              while(snippet.at(++i) != '}') {
              }
            }
            if(snippet.at(i) != '}')
              throw std::logic_error("closing } not found");
            ++i;
          }
        }
        else if(parse_number(number))
          parameter_offsets_and_sizes_map[number].emplace_back(utf8_character_count(insert), 0);
        else if(parse_variable() == ParseVariableResult::not_found)
          insert += '$';
      }
      else {
        insert += snippet[i];
        ++i;
      }
    }
  };
  try {
    parse_snippet(false);
  }
  catch(...) {
    Terminal::get().print("\e[31mError\e[m: could not parse snippet: " + snippet + '\n', true);
    return;
  }

  // $0 should be last parameter
  auto it = parameter_offsets_and_sizes_map.find(0);
  if(it != parameter_offsets_and_sizes_map.end()) {
    parameter_offsets_and_sizes_map.emplace(parameter_offsets_and_sizes_map.rbegin()->first + 1, std::move(it->second));
    parameter_offsets_and_sizes_map.erase(it);
  }

  Mark mark(iter);

  get_source_buffer()->begin_user_action();
  Gtk::TextIter start, end;
  if(get_buffer()->get_selection_bounds(start, end)) {
    get_buffer()->erase(start, end);
    iter = mark->get_iter();
  }
  else if(erase_line) {
    auto start = get_buffer()->get_iter_at_line(iter.get_line());
    auto end = get_iter_at_line_end(iter.get_line());
    get_buffer()->erase(start, end);
    iter = mark->get_iter();
  }
  else if(erase_word && is_token_char(*iter)) {
    auto token_iters = get_token_iters(iter);
    get_buffer()->erase(token_iters.first, token_iters.second);
    iter = mark->get_iter();
  }
  get_buffer()->insert(iter, insert);
  get_source_buffer()->end_user_action();

  iter = mark->get_iter();
  for(auto &parameter_offsets_and_sized : parameter_offsets_and_sizes_map) {
    snippet_parameters_list.emplace_back();
    for(auto &offsets : parameter_offsets_and_sized.second) {
      auto start = iter;
      start.forward_chars(offsets.first);
      auto end = start;
      end.forward_chars(offsets.second);
      snippet_parameters_list.back().emplace_back(start, end);
    }
  }

  if(!parameter_offsets_and_sizes_map.empty())
    select_snippet_parameter();
}

bool Source::BaseView::select_snippet_parameter() {
  for(auto it = extra_cursors.begin(); it != extra_cursors.end();) {
    if(it->snippet)
      it = extra_cursors.erase(it);
    else
      ++it;
  }

  get_buffer()->remove_tag(snippet_parameter_tag, get_buffer()->begin(), get_buffer()->end());

  bool first = true;
  for(auto &parameters : snippet_parameters_list) {
    for(auto &parameter : parameters) {
      if(!first)
        get_buffer()->apply_tag(snippet_parameter_tag, parameter.start->get_iter(), parameter.end->get_iter());
    }
    first = false;
  }

  if(!snippet_parameters_list.empty()) {
    auto snippet_parameters_it = snippet_parameters_list.begin();
    bool first = true;
    for(auto &snippet_parameter : *snippet_parameters_it) {
      auto start = snippet_parameter.start->get_iter();
      auto end = snippet_parameter.end->get_iter();
      if(first) {
        if(snippet_parameter.size > 0 && start == end) { // If the parameter has been erased
          snippet_parameters_list.erase(snippet_parameters_it);
          return select_snippet_parameter();
        }
        keep_snippet_marks = true;
        get_buffer()->select_range(start, end);
        keep_snippet_marks = false;
        first = false;
      }
      else {
        extra_cursors.emplace_back(extra_cursor_selection, start, end, true);
        for(auto it = extra_cursors.begin(); it != extra_cursors.end(); ++it) {
          if(!it->snippet) {
            auto new_start = it->insert->get_iter();
            new_start.forward_chars(start.get_offset() - get_buffer()->get_insert()->get_iter().get_offset());
            auto new_end = new_start;
            new_end.forward_chars(end.get_offset() - start.get_offset());
            extra_cursors.emplace_back(extra_cursor_selection, new_start, new_end, true);
          }
        }

        setup_extra_cursor_signals();
      }
    }
    snippet_parameters_list.erase(snippet_parameters_it);
    return true;
  }
  return false;
}

bool Source::BaseView::clear_snippet_marks() {
  bool cleared = false;

  if(!snippet_parameters_list.empty()) {
    snippet_parameters_list.clear();
    cleared = true;
  }

  for(auto it = extra_cursors.begin(); it != extra_cursors.end();) {
    if(it->snippet) {
      it = extra_cursors.erase(it);
      cleared = true;
    }
    else
      ++it;
  }

  if(cleared)
    get_buffer()->remove_tag(snippet_parameter_tag, get_buffer()->begin(), get_buffer()->end());

  return cleared;
}

Source::BaseView::ExtraCursor::ExtraCursor(const Glib::RefPtr<Gtk::TextTag> &extra_cursor_selection, const Gtk::TextIter &start_iter, const Gtk::TextIter &end_iter, bool snippet, int line_offset)
    : extra_cursor_selection(extra_cursor_selection),
      insert(start_iter, false),
      selection_bound(end_iter, false),
      line_offset(line_offset), snippet(snippet) {
  insert->set_visible(true);
  if(start_iter != end_iter)
    insert->get_buffer()->apply_tag(extra_cursor_selection, insert->get_iter(), selection_bound->get_iter());
}

Source::BaseView::ExtraCursor::~ExtraCursor() {
  insert->get_buffer()->remove_tag(extra_cursor_selection, insert->get_iter(), selection_bound->get_iter());
  insert->set_visible(false);
}

void Source::BaseView::ExtraCursor::move(const Gtk::TextIter &iter, bool selection_activated) {
  insert->get_buffer()->remove_tag(extra_cursor_selection, insert->get_iter(), selection_bound->get_iter());
  insert->get_buffer()->move_mark(insert, iter);
  if(!selection_activated)
    selection_bound->get_buffer()->move_mark(selection_bound, iter);

  auto start = insert->get_iter();
  auto end = selection_bound->get_iter();
  if(start != end)
    insert->get_buffer()->apply_tag(extra_cursor_selection, start, end);
}

void Source::BaseView::ExtraCursor::move_selection_bound(const Gtk::TextIter &iter) {
  insert->get_buffer()->remove_tag(extra_cursor_selection, insert->get_iter(), selection_bound->get_iter());
  selection_bound->get_buffer()->move_mark(selection_bound, iter);
  auto start = insert->get_iter();
  auto end = selection_bound->get_iter();
  if(start != end)
    insert->get_buffer()->apply_tag(extra_cursor_selection, start, end);
}
