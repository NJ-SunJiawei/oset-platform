#pragma once
#include "process.hpp"
#include "source_diff.hpp"
#include "source_spellcheck.hpp"
#include "tooltips.hpp"
#include <boost/filesystem.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <limits>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace Source {
  /// Workaround for buggy Gsv::LanguageManager::get_default()
  class LanguageManager {
  public:
    static Glib::RefPtr<Gsv::LanguageManager> get_default();
  };
  /// Workaround for buggy Gsv::StyleSchemeManager::get_default()
  class StyleSchemeManager {
  public:
    static Glib::RefPtr<Gsv::StyleSchemeManager> get_default();
  };

  Glib::RefPtr<Gsv::Language> guess_language(const boost::filesystem::path &file_path);

  class Offset {
  public:
    Offset() = default;
    Offset(unsigned line, unsigned index, boost::filesystem::path file_path_ = {}) : line(line), index(index), file_path(std::move(file_path_)) {}
    operator bool() const { return !file_path.empty(); }
    bool operator<(const Offset &rhs) const {
      return file_path < rhs.file_path ||
             (file_path == rhs.file_path && (line < rhs.line ||
                                             (line == rhs.line && index < rhs.index)));
    }
    bool operator==(const Offset &rhs) const { return (file_path == rhs.file_path && line == rhs.line && index == rhs.index); }

    unsigned line = 0;
    unsigned index = 0;
    boost::filesystem::path file_path;
  };

  class FixIt {
  public:
    enum class Type {
      insert,
      replace,
      erase
    };

    FixIt(std::string source_, std::string path_, std::pair<Offset, Offset> offsets_);

    std::string string(BaseView &view);

    Type type;
    std::string source;
    std::string path;
    std::pair<Offset, Offset> offsets;

    bool operator<(const FixIt &rhs) const {

      return type < rhs.type ||
             (type == rhs.type && (source < rhs.source ||
                                   (source == rhs.source && (path < rhs.path ||
                                                             (path == rhs.path && offsets < rhs.offsets)))));
    }
    bool operator==(const FixIt &rhs) const {
      return type == rhs.type && source == rhs.source && path == rhs.path && offsets == rhs.offsets;
    }
  };

  class View : public SpellCheckView, public DiffView {
  public:
    static std::set<View *> non_deleted_views;
    static std::set<View *> views;

    View(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language, bool is_generic_view = false);
    ~View() override;

    bool save() override;
    void configure() override;

    std::function<void()> non_interactive_completion;
    std::function<void(bool, bool)> format_style;
    std::function<std::vector<Offset>()> get_declaration_locations;
    std::function<std::vector<Offset>()> get_type_declaration_locations;
    std::function<std::vector<Offset>()> get_implementation_locations;
    std::function<std::vector<Offset>()> get_declaration_or_implementation_locations;
    std::function<std::vector<std::pair<Offset, std::string>>()> get_usages;
    std::function<std::string()> get_method;
    std::function<std::vector<std::pair<Offset, std::string>>()> get_methods;
    std::function<std::vector<std::string>()> get_token_data;
    std::function<std::string()> get_token_spelling;
    std::function<void(const std::string &text)> rename_similar_tokens;
    std::function<void()> goto_next_diagnostic;
    std::function<std::vector<FixIt>()> get_fix_its;
    std::function<void()> toggle_comments;
    std::function<std::tuple<Source::Offset, std::string, size_t>()> get_documentation_template;
    std::function<void(int)> toggle_breakpoint;

    void hide_tooltips() override;
    void hide_dialogs() override;

    void scroll_to_cursor_delayed(bool center, bool show_tooltips) override;

    void extend_selection();
    void shrink_selection();

    void show_or_hide(); /// Show or hide text selection

    bool soft_reparse_needed = false;
    bool full_reparse_needed = false;
    virtual void soft_reparse(bool delayed = false) { soft_reparse_needed = false; }
    virtual void full_reparse() { full_reparse_needed = false; }

    static std::unique_ptr<TinyProcessLib::Process> prettier_background_process;

  protected:
    std::atomic<bool> parsed = {true};
    Tooltips diagnostic_tooltips;
    Tooltips type_tooltips;
    sigc::connection delayed_tooltips_connection;

    Glib::RefPtr<Gtk::TextTag> similar_symbol_tag;
    sigc::connection delayed_tag_similar_symbols_connection;
    virtual void apply_similar_symbol_tag() {}
    bool similar_symbol_tag_applied = false;
    Glib::RefPtr<Gtk::TextTag> clickable_tag;
    sigc::connection delayed_tag_clickable_connection;
    virtual void apply_clickable_tag(const Gtk::TextIter &iter) {}
    bool clickable_tag_applied = false;

    Glib::RefPtr<Gtk::TextTag> hide_tag;

    virtual void show_diagnostic_tooltips(const Gdk::Rectangle &rectangle) { diagnostic_tooltips.show(rectangle); }
    void add_diagnostic_tooltip(const Gtk::TextIter &start, const Gtk::TextIter &end, bool error, std::function<void(Tooltip &)> &&set_buffer);
    void clear_diagnostic_tooltips();
    std::set<int> diagnostic_offsets;
    void place_cursor_at_next_diagnostic();
    virtual void show_type_tooltips(const Gdk::Rectangle &rectangle) {}
    gdouble on_motion_last_x = 0.0;
    gdouble on_motion_last_y = 0.0;

    std::vector<FixIt> fix_its;

    /// Returns true if code iter (not part of comment or string, and not whitespace) is found.
    /// Iter will not be moved if iter is already a code iter.
    bool backward_to_code(Gtk::TextIter &iter);
    /// Returns true if code iter (not part of comment or string, and not whitespace) is found.
    /// Iter will not be moved if iter is already a code iter.
    bool forward_to_code(Gtk::TextIter &iter);
    /// Iter will not be moved if iter is already a code iter (not part of comment or string, and not whitespace) or at line start
    bool backward_to_code_or_line_start(Gtk::TextIter &iter);
    /// Iter will not be moved if iter is already a code iter (not part of comment or string, and not whitespace) or at line end
    bool forward_to_code_or_line_end(Gtk::TextIter &iter);
    /// If closing bracket is found, continues until the open bracket.
    /// Returns if open bracket is found that has no corresponding closing bracket.
    /// Else, return at start of line.
    Gtk::TextIter get_start_of_expression(Gtk::TextIter iter);
    /// Iter will not be moved if iter is already at close symbol.
    bool find_close_symbol_forward(Gtk::TextIter iter, Gtk::TextIter &found_iter, unsigned int positive_char, unsigned int negative_char);
    /// Iter will not be moved if iter is already at open symbol.
    bool find_open_symbol_backward(Gtk::TextIter iter, Gtk::TextIter &found_iter, unsigned int positive_char, unsigned int negative_char);
    long symbol_count(Gtk::TextIter iter, unsigned int positive_char, unsigned int negative_char = std::numeric_limits<unsigned int>::max());
    bool is_templated_function(Gtk::TextIter iter, Gtk::TextIter &parenthesis_end_iter);
    /// If insert is at an possible argument. Also based on last key press.
    bool is_possible_argument();

    bool on_key_press_event(GdkEventKey *event) override;
    bool on_key_press_event_basic(GdkEventKey *event);
    bool on_key_press_event_bracket_language(GdkEventKey *event);
    bool on_key_press_event_smart_brackets(GdkEventKey *event);
    bool on_key_press_event_smart_inserts(GdkEventKey *event);
    bool on_button_press_event(GdkEventButton *event) override;

    bool interactive_completion = true;

  private:
    void setup_signals();
    void setup_format_style(bool is_generic_view);

    Gsv::DrawSpacesFlags parse_show_whitespace_characters(const std::string &text);

    Gsv::GutterRendererText *line_renderer;

    bool use_fixed_continuation_indenting = true;
    guint previous_non_modifier_keyval = 0;

    bool keep_previous_extended_selections = false;
    std::vector<std::pair<Gtk::TextIter, Gtk::TextIter>> previous_extended_selections;
  };
} // namespace Source
