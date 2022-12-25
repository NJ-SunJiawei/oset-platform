#pragma once
#include "source_base.hpp"
#include <atomic>
#include <boost/filesystem.hpp>
#include <gtkmm.h>

class Window : public Gtk::ApplicationWindow {
  Window();

public:
  static Window &get() {
    static Window instance;
    return instance;
  }
  void add_widgets();
  void save_session();
  void load_session(std::vector<boost::filesystem::path> &directories, std::vector<std::pair<boost::filesystem::path, size_t>> &files, std::vector<std::pair<int, int>> &file_offsets, boost::filesystem::path &current_file, bool read_directories_and_files);

protected:
  bool on_key_press_event(GdkEventKey *event) override;
  bool on_delete_event(GdkEventAny *event) override;

private:
  Gtk::AboutDialog about;
  Gtk::ScrolledWindow directories_scrolled_window, terminal_scrolled_window;
  Gtk::Overlay status_overlay;

  void configure();
  void set_menu_actions();
  void search_and_replace_entry();
  void update_search_and_replace_entry();
  void set_tab_entry();
  void goto_line_entry();
  void rename_token_entry();
  std::string last_search;
  std::string last_replace;
  std::string last_find_pattern;
  std::string last_run_command;
  std::string last_run_debug_command;
  bool case_sensitive_search = true;
  bool regex_search = false;
  bool search_entry_shown = false;
  /// Last source view focused
  Source::CommonView *focused_view = nullptr;
  bool find_pattern_case_sensitive = true;
  bool find_pattern_extended_regex = false;
};
