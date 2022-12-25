#include "dispatcher.hpp"
#include <vector>

Dispatcher::Dispatcher() {
  connect();
}

void Dispatcher::connect() {
  connection = dispatcher.connect([this] {
    std::vector<std::list<std::function<void()>>::iterator> its;
    {
      LockGuard lock(functions_mutex);
      if(functions.empty())
        return;
      its.reserve(functions.size());
      for(auto it = functions.begin(); it != functions.end(); ++it)
        its.emplace_back(it);
    }
    for(auto &it : its)
      (*it)();
    {
      LockGuard lock(functions_mutex);
      for(auto &it : its)
        functions.erase(it);
    }
  });
}

Dispatcher::~Dispatcher() {
  disconnect();
}

void Dispatcher::disconnect() {
  connection.disconnect();
}

void Dispatcher::reset() {
  LockGuard lock(functions_mutex);
  disconnect();
  functions.clear();
  connect();
}
