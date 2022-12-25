#pragma once

#include "mutex.hpp"
#include "snippets.hpp"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <gtksourceviewmm.h>
#include <list>
#include <regex>
#include <set>
#include <vector>

namespace Source {
  /// RAII-style text mark. Use instead of Gtk::TextBuffer::create_mark and Gtk::TextBuffer::delete_mark,
  /// since Gtk::TextBuffer::delete_mark is not called upon Glib::RefPtr<Gtk::TextMark> deletion
  class Mark : public Glib::RefPtr<Gtk::TextMark> {
  public:
    Mark(const Gtk::TextIter &iter, bool left_gravity = true) : Glib::RefPtr<Gtk::TextMark>(iter.get_buffer()->create_mark(iter, left_gravity)) {}
    Mark() = default;
    Mark(Mark &&) = default;
    Mark &operator=(Mark &&) = default;
    Mark(const Mark &) = delete;
    Mark &operator=(const Mark &) = delete;
    ~Mark() {
      if(*this)
        (*this)->get_buffer()->delete_mark(*this);
    }
  };

  /// Also used for terminal
  class CommonView : public Gsv::View {
  public:
    CommonView(const Glib::RefPtr<Gsv::Language> &language = {});
    ~CommonView() override;

  protected:
    Glib::RefPtr<Gsv::Language> language;

  public:
    std::string language_id;
    void set_language(const Glib::RefPtr<Gsv::Language> &language);
    /// Checks if parameter languages contain language_id
    bool is_language(const std::initializer_list<std::string> &languages);

  protected:
    bool is_c = false;
    bool is_cpp = false;
    /// Set to true if language is html or js (including typescript)
    bool is_js = false;

    bool keep_clipboard = false;

    bool on_key_press_event(GdkEventKey *event) override;
    bool on_motion_notify_event(GdkEventMotion *motion_event) override;

  public:
    void search_highlight(const std::string &text, bool case_sensitive, bool regex);
    void search_forward();
    void search_backward();
    void replace_forward(const std::string &replacement);
    void replace_backward(const std::string &replacement);
    void replace_all(const std::string &replacement);

    void cut();
    void cut_lines();
    void copy();
    void copy_lines();
    bool disable_spellcheck = false;

    std::function<void(int number)> update_search_occurrences;

  private:
    GtkSourceSearchContext *search_context;
    GtkSourceSearchSettings *search_settings;
    static void search_occurrences_updated(GtkWidget *widget, GParamSpec *property, gpointer data);
  };

  class BaseView : public CommonView {
  public:
    BaseView(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language);
    ~BaseView() override;
    boost::filesystem::path file_path;

    bool load(bool not_undoable_action = false);
    /// Set new text more optimally and without unnecessary scrolling
    void replace_text(const std::string &new_text);
    virtual void rename(const boost::filesystem::path &path);
    virtual bool save() = 0;

    Glib::RefPtr<Gio::FileMonitor> monitor;
    sigc::connection monitor_changed_connection;
    sigc::connection delayed_monitor_changed_connection;

    virtual void configure() = 0;
    virtual void hide_tooltips() = 0;
    virtual void hide_dialogs() = 0;

    void set_tab_char_and_size(char tab_char, unsigned tab_size);
    std::pair<char, unsigned> get_tab_char_and_size() { return {tab_char, tab_size}; }

    /// Safely returns iter given line and an offset using either byte index or character offset. Defaults to using byte index.
    virtual Gtk::TextIter get_iter_at_line_pos(int line, int pos);
    /// Safely returns iter given line and character offset
    Gtk::TextIter get_iter_at_line_offset(int line, int offset);
    /// Safely returns iter given line and byte index
    Gtk::TextIter get_iter_at_line_index(int line, int index);

    Gtk::TextIter get_iter_at_line_end(int line_nr);

    /// Safely places cursor at line using get_iter_at_line_pos.
    void place_cursor_at_line_pos(int line, int pos);
    /// Safely places cursor at line offset
    void place_cursor_at_line_offset(int line, int offset);
    /// Safely places cursor at line index
    void place_cursor_at_line_index(int line, int index);

    virtual void scroll_to_cursor_delayed(bool center, bool show_tooltips) {}

    std::function<void(BaseView *view)> update_tab_label;
    std::function<void(BaseView *view)> update_status_location;
    std::function<void(BaseView *view)> update_status_file_path;
    std::function<void(BaseView *view)> update_status_diagnostics;
    std::function<void(BaseView *view)> update_status_state;
    std::tuple<size_t, size_t, size_t> status_diagnostics;
    std::string status_state;
    std::function<void(BaseView *view)> update_status_branch;
    std::string status_branch;

    void paste();

    std::string get_selected_text();

    void set_snippets();

  protected:
    boost::optional<std::time_t> last_write_time;
    void monitor_file();
    void check_last_write_time(boost::optional<std::time_t> last_write_time_ = {});

    bool is_bracket_language = false;

    unsigned tab_size;
    char tab_char;
    std::string tab;
    std::pair<char, unsigned> find_tab_char_and_size();

    /// Apple key for MacOS, and control key otherwise
    GdkModifierType primary_modifier_mask;

    /// Move iter to line start. Depending on iter position, before or after indentation.
    /// Works with wrapped lines.
    Gtk::TextIter get_smart_home_iter(const Gtk::TextIter &iter);
    /// Move iter to line end. Depending on iter position, before or after indentation.
    /// Works with wrapped lines.
    /// Note that smart end goes FIRST to end of line to avoid hiding empty chars after expressions.
    Gtk::TextIter get_smart_end_iter(const Gtk::TextIter &iter);

  public:
    std::string get_line(const Gtk::TextIter &iter);
    std::string get_line(const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark);
    std::string get_line(int line_nr);
    std::string get_line();

  protected:
    std::string get_line_before(const Gtk::TextIter &iter);
    std::string get_line_before(const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark);
    std::string get_line_before();
    Gtk::TextIter get_tabs_end_iter(const Gtk::TextIter &iter);
    Gtk::TextIter get_tabs_end_iter(const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark);
    Gtk::TextIter get_tabs_end_iter(int line_nr);
    Gtk::TextIter get_tabs_end_iter();

  public:
    bool is_token_char(gunichar chr);

  protected:
    std::pair<Gtk::TextIter, Gtk::TextIter> get_token_iters(Gtk::TextIter iter);
    std::string get_token(const Gtk::TextIter &iter);
    std::string get_token(const Glib::ustring &string, size_t pos = 0);
    void remove_trailing_whitespace_characters();
    void add_missing_newline_at_end_of_file();
    void remove_whitespace_characters(const Gtk::TextIter &iter);

    bool enable_multiple_cursors = false;

  public:
    bool enable_multiple_cursors_placements = false;

  protected:
    Glib::RefPtr<Gtk::TextTag> extra_cursor_selection;

    bool on_key_press_event(GdkEventKey *event) override;
    bool on_key_press_event_extra_cursors(GdkEventKey *event);

    class ExtraCursor {
      Glib::RefPtr<Gtk::TextTag> extra_cursor_selection;

    public:
      ExtraCursor(const Glib::RefPtr<Gtk::TextTag> &extra_cursor_selection, const Gtk::TextIter &start_iter, const Gtk::TextIter &end_iter, bool snippet, int line_offset = 0);
      ExtraCursor(const Glib::RefPtr<Gtk::TextTag> &extra_cursor_selection, const Gtk::TextIter &start_iter, bool snippet, int line_offset = 0) : ExtraCursor(extra_cursor_selection, start_iter, start_iter, snippet, line_offset) {}
      ~ExtraCursor();

      void move(const Gtk::TextIter &iter, bool selection_activated);
      void move_selection_bound(const Gtk::TextIter &iter);

      Mark insert;
      Mark selection_bound;
      /// Used when moving cursor up/down lines
      int line_offset;
      /// Set to true when the extra cursor corresponds to a snippet parameter
      bool snippet;
    };
    std::list<ExtraCursor> extra_cursors;

    void setup_extra_cursor_signals();
    bool extra_cursors_signals_set = false;

    /// After inserting a snippet, one can use tab to select the next parameter
    bool keep_snippet_marks = false;
    Mutex snippets_mutex;
    std::vector<Snippets::Snippet> *snippets GUARDED_BY(snippets_mutex) = nullptr;
    class SnippetParameter {
    public:
      SnippetParameter(const Gtk::TextIter &start_iter, const Gtk::TextIter &end_iter)
          : start(start_iter, false), end(end_iter, false), size(end_iter.get_offset() - start_iter.get_offset()) {}
      Mark start, end;
      /// Used to check if the parameter has been deleted, and should be passed on next tab
      int size;
    };
    std::list<std::list<SnippetParameter>> snippet_parameters_list;
    Glib::RefPtr<Gtk::TextTag> snippet_parameter_tag;
    void insert_snippet(Gtk::TextIter iter, const std::string &snippet);
    bool select_snippet_parameter();
    bool clear_snippet_marks();
  };
} // namespace Source
