#pragma once
#include "source.hpp"
#include <boost/optional.hpp>
#include <gtkmm.h>
#include <iostream>
#include <list>
#include <map>
#include <sigc++/sigc++.h>
#include <type_traits>
#include <vector>

class Notebook : public Gtk::Paned {
  class TabLabel : public Gtk::EventBox {
  public:
    TabLabel(const std::function<void()> &on_close);
    Gtk::Label label;
  };

  class CursorLocation {
  public:
    CursorLocation(Source::View *view, const Gtk::TextIter &iter) : view(view), mark(iter, false) {}
    Source::View *view;
    Source::Mark mark;
  };

  Notebook();

public:
  static Notebook &get() {
    static Notebook instance;
    return instance;
  }

  std::vector<Gtk::Notebook> notebooks;

  size_t size();
  Source::View *get_view(size_t index);
  Source::View *get_current_view();
  std::vector<Source::View *> &get_views();
  enum class Position {
    left,
    right,
    infer,
    split
  };
  bool open(Source::View *view);
  bool open(const boost::filesystem::path &file_path, Position position = Position::infer);
  void install_rust();
  void open_uri(const std::string &uri);
  void configure(size_t index);
  bool save(size_t index);
  bool save_current();
  bool close(size_t index);
  bool close(Source::View *view);
  bool close_current();
  void next();
  void previous();
  void toggle_split();
  std::vector<std::pair<size_t, Source::View *>> get_notebook_views();

  Gtk::Label status_location;
  Gtk::Label status_file_path;
  Gtk::Label status_branch;
  Gtk::Label status_diagnostics;
  Gtk::Label status_state;
  void update_status(Source::BaseView *view);
  void clear_status();

  std::function<void(Source::View *)> on_focus_page;
  std::function<void(Source::View *)> on_change_page;
  std::function<void(Source::View *)> on_close_page;

  /// Cursor history
  std::list<CursorLocation> cursor_locations;
  std::list<CursorLocation>::iterator current_cursor_location = cursor_locations.end();
  bool disable_next_update_cursor_locations = false;
  void delete_cursor_locations(Source::View *view);

private:
  /// Throws on out of bounds arguments
  Source::View *get_view(size_t notebook_index, int page);
  void focus_view(Source::View *view);
  /// Throws if view is not found
  size_t get_index(Source::View *view);
  /// Throws on out of bounds index
  std::pair<size_t, int> get_notebook_page(size_t index);
  /// Throws if view is not found
  std::pair<size_t, int> get_notebook_page(Source::View *view);

  std::vector<Source::View *> source_views; // Is NOT freed in destructor, this is intended for quick program exit.
  std::vector<std::unique_ptr<Gtk::Widget>> source_maps;
  std::vector<std::unique_ptr<Gtk::ScrolledWindow>> scrolled_windows;
  std::vector<std::unique_ptr<Gtk::Box>> hboxes;
  std::vector<std::unique_ptr<TabLabel>> tab_labels;

  bool split = false;
  boost::optional<size_t> last_index;

  void set_current_view(Source::View *view);
  Source::View *current_view = nullptr;
  Source::View *intermediate_view = nullptr;

  bool save_modified_dialog(size_t index);
};
