#pragma once
#include "dispatcher.hpp"
#include "mutex.hpp"
#include "process.hpp"
#include "source_base.hpp"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <functional>
#include <gtkmm.h>
#include <iostream>
#include <tuple>

class Terminal : public Source::CommonView {
  Terminal();

public:
  static Terminal &get() {
    static Terminal instance;
    return instance;
  }

  int process(const std::string &command, const boost::filesystem::path &path = "", bool use_pipes = true);
  int process(std::istream &stdin_stream, std::ostream &stdout_stream, const std::string &command, const boost::filesystem::path &path = "", std::ostream *stderr_stream = nullptr);
  /// The callback is run in the main thread.
  std::shared_ptr<TinyProcessLib::Process> async_process(const std::string &command, const boost::filesystem::path &path = "", std::function<void(int exit_status)> callback = nullptr, bool quiet = false);
  void kill_last_async_process(bool force = false);
  void kill_async_processes(bool force = false);

  /// Must be called from main thread
  void print(std::string message, bool bold = false);
  /// Callable from any thread.
  void async_print(std::string message, bool bold = false);

  void configure();

  void clear();

  std::function<void()> scroll_to_bottom;

  void paste();

protected:
  bool on_motion_notify_event(GdkEventMotion *motion_event) override;
  bool on_button_press_event(GdkEventButton *button_event) override;
  bool on_key_press_event(GdkEventKey *event) override;

private:
  Dispatcher dispatcher;
  Glib::RefPtr<Gtk::TextTag> bold_tag;
  Glib::RefPtr<Gtk::TextTag> link_tag;
  Glib::RefPtr<Gtk::TextTag> invisible_tag;
  Glib::RefPtr<Gtk::TextTag> red_tag, green_tag, yellow_tag, blue_tag, magenta_tag, cyan_tag, gray_tag;
  Glib::RefPtr<Gdk::Cursor> link_mouse_cursor;
  Glib::RefPtr<Gdk::Cursor> default_mouse_cursor;

  struct Link {
    int start_pos, end_pos;
    std::string path;
    int line, line_index;
  };
  static boost::optional<Link> find_link(const std::string &line);

  Mutex processes_mutex;
  std::vector<std::shared_ptr<TinyProcessLib::Process>> processes GUARDED_BY(processes_mutex);
  Glib::ustring stdin_buffer;
};
