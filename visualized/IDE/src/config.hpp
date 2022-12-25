#pragma once
#include "dispatcher.hpp"
#include "json.hpp"
#include <boost/filesystem.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class Config {
public:
  class Menu {
  public:
    std::unordered_map<std::string, std::string> keys;
  };

  class Theme {
  public:
    std::string name;
    std::string variant;
    std::string font;
  };

  class Terminal {
  public:
    int history_size;
    std::string font;
    bool clear_on_compile;
    bool clear_on_run_command;
    bool hide_entry_on_run_command;
  };

  class Project {
  public:
    class CMake {
    public:
      std::string command;
      std::string compile_command;
    };
    class Meson {
    public:
      std::string command;
      std::string compile_command;
    };

    std::string default_build_path;
    std::string debug_build_path;
    CMake cmake;
    Meson meson;
    std::string default_build_management_system;
    bool save_on_compile_or_run;
    std::string ctags_command;
    std::string grep_command;
    std::string cargo_command;
    std::string python_command;
    std::string markdown_command;
  };

  class Source {
  public:
    class DocumentationSearch {
    public:
      std::string separator;
      std::unordered_map<std::string, std::string> queries;
    };

    std::string style;
    std::string font;
    std::string spellcheck_language;

    bool add_missing_newline_at_end_of_file_on_save;
    bool remove_trailing_whitespace_characters_on_save;
    std::string show_whitespace_characters;

    bool format_style_on_save;
    bool format_style_on_save_if_style_file_found;

    bool smart_brackets;
    bool smart_inserts;

    bool show_map;
    unsigned map_font_size;
    bool show_git_diff;
    bool show_background_pattern;
    bool show_right_margin;
    unsigned right_margin_position;

    bool auto_tab_char_and_size;
    char default_tab_char;
    unsigned default_tab_size;
    bool tab_indents_line;
    std::string word_wrap;
    bool highlight_current_line;
    bool show_line_numbers;
    bool enable_multiple_cursors;
    bool auto_reload_changed_files;
    bool search_for_selection;

    std::string clang_format_style;
    unsigned clang_usages_threads;

    bool clang_tidy_enable;
    std::string clang_tidy_checks;

    bool clang_detailed_preprocessing_record = false;

    bool debug_place_cursor_at_stop;

    std::unordered_map<std::string, DocumentationSearch> documentation_searches;
  };

  class Log {
  public:
    bool libclang;
    bool language_server;
  };

private:
  Config();

public:
  static Config &get() {
    static Config instance;
    return instance;
  }

  void load();

  Menu menu;
  Theme theme;
  Terminal terminal;
  Project project;
  Source source;
  Log log;

  boost::filesystem::path home_path;
  boost::filesystem::path home_juci_path;
  std::string home_oset_path;

private:
  /// Used to dispatch Terminal outputs after juCi++ GUI setup and configuration
  Dispatcher dispatcher;

  void update(JSON &cfg);
  void make_version_dependent_corrections(JSON &cfg, const JSON &default_cfg, const std::string &version);
  void add_missing_nodes(JSON &cfg, const JSON &default_cfg);
  void remove_deprecated_nodes(JSON &cfg, const JSON &default_cfg);
  void read(const JSON &cfg);

  /// If you add or remove nodes from the default_config, increase the juci
  /// version number (JUCI_VERSION) in ../CMakeLists.txt to automatically apply
  /// the changes to user's ~/.juci/config/config.json files
  std::string default_config();
  const char *juci_light_style();
  const char *juci_dark_style();
  const char *juci_dark_blue_style();
};
