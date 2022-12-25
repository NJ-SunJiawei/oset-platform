#pragma once
#include "mutex.hpp"
#include <functional>
#include <gtkmm.h>
#include <list>

class Dispatcher {
private:
  Mutex functions_mutex;
  std::list<std::function<void()>> functions GUARDED_BY(functions_mutex);
  Glib::Dispatcher dispatcher;
  sigc::connection connection;

  void connect();

public:
  /// Must be called from main GUI thread
  Dispatcher();
  ~Dispatcher();

  /// Queue function to main GUI thread.
  /// Can be called from any thread.
  template <typename T>
  void post(T &&function) {
    LockGuard lock(functions_mutex);
    functions.emplace_back(std::forward<T>(function));
    dispatcher();
  }

  /// Must be called from main GUI thread
  void disconnect();

  /// Must be called from main GUI thread
  void reset();
};
