#pragma once
#include "source_base.hpp"
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <functional>
#include <gtkmm.h>
#include <list>
#include <set>
#include <string>
#include <unordered_map>

class Tooltip {
public:
  Tooltip(Gsv::View *view, const Gtk::TextIter &start_iter, const Gtk::TextIter &end_iter, std::function<void(Tooltip &)> set_buffer_);
  Tooltip(std::function<void(Tooltip &tooltip)> set_buffer_);
  ~Tooltip();

  void update();
  void show(bool disregard_drawn = false, const std::function<void()> &on_motion = nullptr);
  void hide(const boost::optional<std::pair<int, int>> &last_mouse_pos, const boost::optional<std::pair<int, int>> &mouse_pos);

  Gdk::Rectangle activation_rectangle;
  Source::Mark start_mark, end_mark;

  Glib::RefPtr<Gtk::TextBuffer> buffer;

  void insert_with_links_tagged(const std::string &text);
  void insert_markdown(const std::string &text);
  void insert_code(const std::string &code, boost::variant<std::string, Glib::RefPtr<Gsv::Language>> language = Glib::RefPtr<Gsv::Language>{}, bool block = false);
  void insert_doxygen(const std::string &input, bool remove_delimiters);
  /// Inserts python docstring
  void insert_docstring(const std::string &input);

  /// Remove empty lines at end of buffer
  void remove_trailing_newlines();

private:
  std::unique_ptr<Gtk::Window> window;
  Gtk::ScrolledWindow *scrolled_window = nullptr;

  Gsv::View *view;
  std::function<void(Tooltip &)> set_buffer;
  /// Initial maximum number of characters on each line
  int max_columns = 80;
  std::pair<int, int> size;
  Gdk::Rectangle rectangle;

  bool shown = false;

  Glib::RefPtr<Gtk::TextTag> link_tag;
  Glib::RefPtr<Gtk::TextTag> h1_tag;
  Glib::RefPtr<Gtk::TextTag> h2_tag;
  Glib::RefPtr<Gtk::TextTag> h3_tag;
  Glib::RefPtr<Gtk::TextTag> code_tag;
  Glib::RefPtr<Gtk::TextTag> code_block_tag;
  Glib::RefPtr<Gtk::TextTag> bold_tag;
  Glib::RefPtr<Gtk::TextTag> italic_tag;
  Glib::RefPtr<Gtk::TextTag> strikethrough_tag;

  std::map<Glib::RefPtr<Gtk::TextTag>, std::string> links;
  std::map<Glib::RefPtr<Gtk::TextTag>, std::string> reference_links;
  std::unordered_map<std::string, std::string> references;

  void wrap_lines();
  void create_tags();
};

class Tooltips {
public:
  static std::set<Tooltip *> shown_tooltips;
  static Gdk::Rectangle drawn_tooltips_rectangle;
  static void init() { drawn_tooltips_rectangle = Gdk::Rectangle(); }

  void show(const Gdk::Rectangle &rectangle, bool disregard_drawn = false);
  void show(bool disregard_drawn = false);
  void hide(const boost::optional<std::pair<int, int>> &last_mouse_pos = {}, const boost::optional<std::pair<int, int>> &mouse_pos = {});
  void clear() { tooltip_list.clear(); };

  template <typename... Ts>
  void emplace_back(Ts &&...params) {
    tooltip_list.emplace_back(std::forward<Ts>(params)...);
  }

  std::function<void()> on_motion;

private:
  std::list<Tooltip> tooltip_list;
};
