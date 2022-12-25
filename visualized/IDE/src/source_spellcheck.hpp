#pragma once
#include "source_base.hpp"
#include <aspell.h>

namespace Source {
  class SpellCheckView : virtual public Source::BaseView {
  public:
    SpellCheckView(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language);
    ~SpellCheckView() override;

    void configure() override;
    void hide_dialogs() override;

    void spellcheck();
    void remove_spellcheck_errors();
    void goto_next_spellcheck_error();

  protected:
    /// For instance, the iter before a closing " is a spellcheck iter, while an opening " is not a spellcheck iter.
    /// Otherwise, strings and comments should return true.
    bool is_spellcheck_iter(const Gtk::TextIter &iter);
    /// For instance, both opening and closing " are code iters.
    /// None of the comment characters, for instance //, are code iters.
    /// Otherwise, strings and comments should return false.
    bool is_code_iter(const Gtk::TextIter &iter);
    guint last_keyval = 0;

    Glib::RefPtr<Gtk::TextTag> comment_tag;
    Glib::RefPtr<Gtk::TextTag> string_tag;
    Glib::RefPtr<Gtk::TextTag> no_spellcheck_tag;

  private:
    bool spellcheck_all = false;

    Glib::RefPtr<Gtk::TextTag> spellcheck_error_tag;

    sigc::connection signal_tag_added_connection;
    sigc::connection signal_tag_removed_connection;

    static AspellConfig *spellcheck_config;
    AspellCanHaveError *spellcheck_possible_err;
    AspellSpeller *spellcheck_checker;
    bool is_word_iter(const Gtk::TextIter &iter);
    std::pair<Gtk::TextIter, Gtk::TextIter> get_word(Gtk::TextIter iter);
    void spellcheck_word(Gtk::TextIter start, Gtk::TextIter end);
    std::vector<std::string> get_spellcheck_suggestions(const Gtk::TextIter &start, const Gtk::TextIter &end);
    sigc::connection delayed_spellcheck_suggestions_connection;
    sigc::connection delayed_spellcheck_error_clear;

    void spellcheck(const Gtk::TextIter &start, const Gtk::TextIter &end);
  };
} // namespace Source
