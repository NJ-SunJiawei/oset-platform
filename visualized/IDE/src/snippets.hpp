#pragma once

#include <gdk/gdk.h>
#include <regex>
#include <string>
#include <vector>

class Snippets {
public:
  class Snippet {
  public:
    std::string prefix;
    guint key;
    GdkModifierType modifier;
    std::string body;
    std::string description;
  };

  static Snippets &get() {
    static Snippets instance;
    return instance;
  }

  std::vector<std::pair<std::regex, std::vector<Snippet>>> snippets;
  void load();
};
