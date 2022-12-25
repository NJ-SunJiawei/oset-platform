#pragma once
#include "source_base.hpp"
#include <boost/optional.hpp>
#include <functional>
#include <gtkmm.h>
#include <unordered_map>

class SelectionDialogBase {
  class ListViewText : public Gtk::TreeView {
    class ColumnRecord : public Gtk::TreeModel::ColumnRecord {
    public:
      ColumnRecord() {
        add(text);
        add(index);
      }
      Gtk::TreeModelColumn<std::string> text;
      Gtk::TreeModelColumn<unsigned int> index;
    };

  public:
    bool use_markup;
    ColumnRecord column_record;
    ListViewText(bool use_markup);
    void append(const std::string &value);
    void erase_rows();
    void clear();

  private:
    Glib::RefPtr<Gtk::ListStore> list_store;
    Gtk::CellRendererText cell_renderer;
    unsigned int size = 0;
  };

  class SearchEntry : public Gtk::SearchEntry {
  public:
    SearchEntry() : Gtk::SearchEntry() {}
    bool on_key_press_event(GdkEventKey *event) override { return Gtk::SearchEntry::on_key_press_event(event); };
  };

public:
  SelectionDialogBase(Source::BaseView *view, const boost::optional<Gtk::TextIter> &start_iter, bool show_search_entry, bool use_markup);
  virtual ~SelectionDialogBase() {}
  void add_row(const std::string &row);
  void erase_rows();
  void set_cursor_at_last_row();
  void show();
  void hide();

  bool is_visible() { return window.is_visible(); }
  void get_position(int &root_x, int &root_y) { window.get_position(root_x, root_y); }

  std::function<void()> on_show;
  std::function<void()> on_hide;
  std::function<void(boost::optional<unsigned int> index, const std::string &text)> on_change;
  std::function<void(unsigned int index, const std::string &text, bool hide_window)> on_select;
  std::function<void(const std::string &text)> on_search_entry_changed;
  Source::Mark start_mark;

protected:
  void cursor_changed();

  Source::BaseView *view;
  Gtk::Window window;
  Gtk::Box vbox;
  Gtk::ScrolledWindow scrolled_window;
  ListViewText list_view_text;
  SearchEntry search_entry;
  bool show_search_entry;

  boost::optional<unsigned int> last_index;
};

class SelectionDialog : public SelectionDialogBase {
  SelectionDialog(Source::BaseView *view, const boost::optional<Gtk::TextIter> &start_iter, bool show_search_entry, bool use_markup);
  static std::unique_ptr<SelectionDialog> instance;

public:
  bool on_key_press(GdkEventKey *event);

  static void create(Source::BaseView *view, bool show_search_entry = true, bool use_markup = false) {
    instance = std::unique_ptr<SelectionDialog>(new SelectionDialog(view, view->get_buffer()->get_insert()->get_iter(), show_search_entry, use_markup));
  }
  static void create(bool show_search_entry = true, bool use_markup = false) {
    instance = std::unique_ptr<SelectionDialog>(new SelectionDialog(nullptr, {}, show_search_entry, use_markup));
  }
  static std::unique_ptr<SelectionDialog> &get() { return instance; }
};

class CompletionDialog : public SelectionDialogBase {
  CompletionDialog(Source::BaseView *view, const Gtk::TextIter &start_iter);
  static std::unique_ptr<CompletionDialog> instance;

public:
  bool on_key_release(GdkEventKey *event);
  bool on_key_press(GdkEventKey *event);

  static void create(Source::BaseView *view, const Gtk::TextIter &start_iter) {
    instance = std::unique_ptr<CompletionDialog>(new CompletionDialog(view, start_iter));
  }
  static std::unique_ptr<CompletionDialog> &get() { return instance; }

private:
  void select(bool hide_window = true);

  int show_offset;
  bool row_in_entry = false;
};
