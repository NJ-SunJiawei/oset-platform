#pragma once
#include <functional>
#include <gtkmm.h>
#include <string>
#include <unordered_map>

class Menu {
  Menu() = default;

public:
  static Menu &get() {
    static Menu instance;
    return instance;
  }

  void add_action(const std::string &name, const std::function<void()> &action);
  std::unordered_map<std::string, Glib::RefPtr<Gio::SimpleAction>> actions;
  std::map<std::pair<guint, GdkModifierType>, std::vector<Glib::RefPtr<Gio::SimpleAction>>> accelerators_with_multiple_actions;
  void set_keys();

  void build();

  Glib::RefPtr<Gio::Menu> juci_menu;
  Glib::RefPtr<Gio::Menu> window_menu;
  std::unique_ptr<Gtk::Menu> right_click_line_menu;
  std::unique_ptr<Gtk::Menu> right_click_selected_menu;
  std::function<void()> toggle_menu_items = [] {};

private:
  Glib::RefPtr<Gtk::Builder> builder;
};
