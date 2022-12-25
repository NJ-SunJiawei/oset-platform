#pragma once

#include <boost/optional.hpp>
#include <gdk/gdk.h>
#include <regex>
#include <string>
#include <vector>

class Commands {
public:
  class Command {
  public:
    guint key;
    GdkModifierType modifier;
    boost::optional<std::regex> path;
    std::string compile;
    std::string run;
    bool debug;
    std::string debug_remote_host;
  };

  static Commands &get() {
    static Commands instance;
    return instance;
  }

  std::vector<Command> commands;
  void load();
};
