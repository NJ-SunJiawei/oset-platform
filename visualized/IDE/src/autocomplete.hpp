#pragma once
#include "dispatcher.hpp"
#include "mutex.hpp"
#include "tooltips.hpp"
#include <atomic>
#include <boost/optional.hpp>
#include <thread>

class Autocomplete {
  Source::BaseView *view;
  bool &interactive_completion;
  /// If view buffer should be passed to add_rows. Empty buffer is passed if not.
  /// Also, some utilities, like libclang, require that autocomplete is started at the beginning of a word.
  bool pass_buffer_and_strip_word;

  Dispatcher dispatcher;

public:
  enum class State {
    idle,
    starting,
    restarting,
    canceled
  };

  Mutex prefix_mutex;
  Glib::ustring prefix GUARDED_BY(prefix_mutex);
  std::vector<std::string> rows;
  Tooltips tooltips;

  /// Never changed outside main thread
  std::atomic<State> state = {State::idle};

  std::thread thread;

  std::function<bool()> is_processing = [] { return true; };
  std::function<void()> reparse = [] {};
  std::function<void()> cancel_reparse = [] {};
  std::function<std::unique_ptr<LockGuard>()> get_parse_lock = [] { return nullptr; };
  std::function<void()> stop_parse = [] {};

  std::function<bool(guint last_keyval)> is_continue_key = [this](guint keyval) { return view->is_token_char(gdk_keyval_to_unicode(keyval)); };
  std::function<bool(guint last_keyval)> is_restart_key = [](guint) { return false; };
  std::function<bool()> run_check = [] { return false; };

  std::function<void()> before_add_rows = [] {};
  std::function<void()> after_add_rows = [] {};
  std::function<void()> on_add_rows_error = [] {};

  /// The handler is not run in the main loop if use_thread is true. Should return false on error.
  std::function<bool(std::string &buffer, int line, int line_index)> add_rows = [](std::string &, int, int) { return true; };

  std::function<void()> on_show = [] {};
  std::function<void()> on_hide = [] {};
  std::function<void(boost::optional<unsigned int>, const std::string &)> on_change;
  std::function<void(unsigned int, const std::string &, bool)> on_select;

  std::function<std::function<void(Tooltip &tooltip)>(unsigned int)> set_tooltip_buffer = [](unsigned int index) { return nullptr; };
  std::function<void()> clear_tooltips = [this] { tooltips.hide(); };

  Autocomplete(Source::BaseView *view, bool &interactive_completion, guint &last_keyval, bool pass_buffer_and_strip_word, bool use_thread);

  void run();
  void stop();

private:
  void setup_dialog();
  bool use_thread;
};
