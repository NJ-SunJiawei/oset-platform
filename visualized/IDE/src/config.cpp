#include "config.hpp"
#include "filesystem.hpp"
#include "process.hpp"
#include "terminal.hpp"
#include "utility.hpp"
#include <algorithm>
#include <exception>
#include <iostream>
#include <thread>

Config::Config() {
  home_path = filesystem::get_home_path();

  home_oset_path = IDE_INSTALL_PATH;//filesystem::get_current_path();

  if(home_path.empty())
    throw std::runtime_error("Could not find home path");
  home_juci_path = home_path / ".juci";
}

void Config::load() {
  auto config_dir = home_juci_path / "config";
  auto config_json = config_dir / "config.json";
  try {
    boost::filesystem::create_directories(config_dir);

    if(!boost::filesystem::exists(config_json))
      filesystem::write(config_json, default_config());

    auto juci_style_path = home_juci_path / "styles";
    boost::filesystem::create_directories(juci_style_path);

    juci_style_path /= "juci-light.xml";
    if(!boost::filesystem::exists(juci_style_path))
      filesystem::write(juci_style_path, juci_light_style());
    juci_style_path = juci_style_path.parent_path();
    juci_style_path /= "juci-dark.xml";
    if(!boost::filesystem::exists(juci_style_path))
      filesystem::write(juci_style_path, juci_dark_style());
    juci_style_path = juci_style_path.parent_path();
    juci_style_path /= "juci-dark-blue.xml";
    if(!boost::filesystem::exists(juci_style_path))
      filesystem::write(juci_style_path, juci_dark_blue_style());

    JSON cfg(config_json);
    update(cfg);
    read(cfg);
  }
  catch(const std::exception &e) {
    dispatcher.post([config_json = std::move(config_json), e_what = std::string(e.what())] {
      ::Terminal::get().print("\e[31mError\e[m: could not parse " + filesystem::get_short_path(config_json).string() + ": " + e_what + "\n", true);
    });
    JSON default_cfg(default_config());
    read(default_cfg);
  }
}

void Config::update(JSON &cfg) {
  auto version = cfg.string("version");
  if(version == JUCI_VERSION)
    return;
  JSON default_cfg(default_config());
  make_version_dependent_corrections(cfg, default_cfg, version);
  cfg.set("version", JUCI_VERSION);

  add_missing_nodes(cfg, default_cfg);
  remove_deprecated_nodes(cfg, default_cfg);

  cfg.to_file(home_juci_path / "config" / "config.json", 2);

  auto style_path = home_juci_path / "styles";
  filesystem::write(style_path / "juci-light.xml", juci_light_style());
  filesystem::write(style_path / "juci-dark.xml", juci_dark_style());
  filesystem::write(style_path / "juci-dark-blue.xml", juci_dark_blue_style());
}

void Config::make_version_dependent_corrections(JSON &cfg, const JSON &default_cfg, const std::string &version) {
  try {
    if(version_compare(version, "1.2.4") <= 0) {
      auto keybindings = cfg.object("keybindings");
      auto print = keybindings.string_optional("print");
      if(print && *print == "<primary>p") {
        keybindings.set("print", "");
        dispatcher.post([] {
          ::Terminal::get().print("Preference change: keybindings.print set to \"\"\n");
        });
      }
    }
  }
  catch(const std::exception &e) {
    std::cerr << "Error correcting preferences: " << e.what() << std::endl;
  }
}

void Config::add_missing_nodes(JSON &cfg, const JSON &default_cfg) {
  for(auto &default_cfg_child : default_cfg.children_or_empty()) {
    try {
      auto cfg_child = cfg.child(default_cfg_child.first);
      add_missing_nodes(cfg_child, default_cfg_child.second);
    }
    catch(...) {
      cfg.set(default_cfg_child.first, default_cfg_child.second);
    }
  }
}

void Config::remove_deprecated_nodes(JSON &cfg, const JSON &default_cfg) {
  auto children = cfg.children_or_empty();
  for(size_t i = 0; i < children.size();) {
    try {
      auto default_cfg_child = default_cfg.child(children[i].first);
      remove_deprecated_nodes(children[i].second, default_cfg_child);
      ++i;
    }
    catch(...) {
      cfg.remove(children[i].first);
      children = cfg.children_or_empty();
    }
  }
}

void Config::read(const JSON &cfg) {
  for(auto &keybinding : cfg.children("keybindings"))
    menu.keys[keybinding.first] = keybinding.second.string();

  auto source_json = cfg.object("source");
  source.style = source_json.string("style");
  source.font = source_json.string("font");
  source.add_missing_newline_at_end_of_file_on_save = source_json.boolean("add_missing_newline_at_end_of_file_on_save", JSON::ParseOptions::accept_string);
  source.remove_trailing_whitespace_characters_on_save = source_json.boolean("remove_trailing_whitespace_characters_on_save", JSON::ParseOptions::accept_string);
  source.show_whitespace_characters = source_json.string("show_whitespace_characters");
  source.format_style_on_save = source_json.boolean("format_style_on_save", JSON::ParseOptions::accept_string);
  source.format_style_on_save_if_style_file_found = source_json.boolean("format_style_on_save_if_style_file_found", JSON::ParseOptions::accept_string);
  source.smart_brackets = source_json.boolean("smart_brackets", JSON::ParseOptions::accept_string);
  source.smart_inserts = source_json.boolean("smart_inserts", JSON::ParseOptions::accept_string);
  if(source.smart_inserts)
    source.smart_brackets = true;
  source.show_map = source_json.boolean("show_map", JSON::ParseOptions::accept_string);
  source.map_font_size = source_json.integer("map_font_size", JSON::ParseOptions::accept_string);
  source.show_git_diff = source_json.boolean("show_git_diff", JSON::ParseOptions::accept_string);
  source.show_background_pattern = source_json.boolean("show_background_pattern", JSON::ParseOptions::accept_string);
  source.show_right_margin = source_json.boolean("show_right_margin", JSON::ParseOptions::accept_string);
  source.right_margin_position = source_json.integer("right_margin_position", JSON::ParseOptions::accept_string);
  source.spellcheck_language = source_json.string("spellcheck_language");
  auto default_tab_char_str = source_json.string("default_tab_char");
  if(default_tab_char_str.size() == 1)
    source.default_tab_char = default_tab_char_str[0];
  else
    source.default_tab_char = ' ';
  source.default_tab_size = source_json.integer("default_tab_size", JSON::ParseOptions::accept_string);
  source.auto_tab_char_and_size = source_json.boolean("auto_tab_char_and_size", JSON::ParseOptions::accept_string);
  source.tab_indents_line = source_json.boolean("tab_indents_line", JSON::ParseOptions::accept_string);
  source.word_wrap = source_json.string("word_wrap");
  source.highlight_current_line = source_json.boolean("highlight_current_line", JSON::ParseOptions::accept_string);
  source.show_line_numbers = source_json.boolean("show_line_numbers", JSON::ParseOptions::accept_string);
  source.enable_multiple_cursors = source_json.boolean("enable_multiple_cursors", JSON::ParseOptions::accept_string);
  source.auto_reload_changed_files = source_json.boolean("auto_reload_changed_files", JSON::ParseOptions::accept_string);
  source.search_for_selection = source_json.boolean("search_for_selection", JSON::ParseOptions::accept_string);
  source.clang_format_style = source_json.string("clang_format_style");
  source.clang_usages_threads = static_cast<unsigned>(source_json.integer("clang_usages_threads", JSON::ParseOptions::accept_string));
  source.clang_tidy_enable = source_json.boolean("clang_tidy_enable", JSON::ParseOptions::accept_string);
  source.clang_tidy_checks = source_json.string("clang_tidy_checks");
  source.clang_detailed_preprocessing_record = source_json.boolean("clang_detailed_preprocessing_record", JSON::ParseOptions::accept_string);
  source.debug_place_cursor_at_stop = source_json.boolean("debug_place_cursor_at_stop", JSON::ParseOptions::accept_string);

  for(auto &documentation_searches : cfg.children("documentation_searches")) {
    auto &documentation_search = source.documentation_searches[documentation_searches.first];
    documentation_search.separator = documentation_searches.second.string("separator");
    for(auto &i : documentation_searches.second.children("queries"))
      documentation_search.queries[i.first] = i.second.string();
  }

  auto theme_json = cfg.object("gtk_theme");
  theme.name = theme_json.string("name");
  theme.variant = theme_json.string("variant");
  theme.font = theme_json.string("font");

  auto project_json = cfg.object("project");
  project.default_build_path = project_json.string("default_build_path");
  project.debug_build_path = project_json.string("debug_build_path");
  auto cmake_json = project_json.object("cmake");
  project.cmake.command = cmake_json.string("command");
  project.cmake.compile_command = cmake_json.string("compile_command");
  auto meson_json = project_json.object("meson");
  project.meson.command = meson_json.string("command");
  project.meson.compile_command = meson_json.string("compile_command");
  project.default_build_management_system = project_json.string("default_build_management_system");
  project.save_on_compile_or_run = project_json.boolean("save_on_compile_or_run", JSON::ParseOptions::accept_string);
  project.ctags_command = project_json.string("ctags_command");
  project.grep_command = project_json.string("grep_command");
  project.cargo_command = project_json.string("cargo_command");
  project.python_command = project_json.string("python_command");
  project.markdown_command = project_json.string("markdown_command");

  auto terminal_json = cfg.object("terminal");
  terminal.history_size = terminal_json.integer("history_size", JSON::ParseOptions::accept_string);
  terminal.font = terminal_json.string("font");
  terminal.clear_on_compile = terminal_json.boolean("clear_on_compile", JSON::ParseOptions::accept_string);
  terminal.clear_on_run_command = terminal_json.boolean("clear_on_run_command", JSON::ParseOptions::accept_string);
  terminal.hide_entry_on_run_command = terminal_json.boolean("hide_entry_on_run_command", JSON::ParseOptions::accept_string);

  auto log_json = cfg.object("log");
  log.libclang = log_json.boolean("libclang", JSON::ParseOptions::accept_string);
  log.language_server = log_json.boolean("language_server", JSON::ParseOptions::accept_string);
}

std::string Config::default_config() {
  static auto get_config = [] {
    std::string cmake_version;
    unsigned thread_count = 0;
    std::stringstream ss;
    TinyProcessLib::Process process(
        "cmake --version", "",
        [&ss](const char *buffer, size_t n) {
          ss.write(buffer, n);
        },
        [](const char *buffer, size_t n) {});
    if(process.get_exit_status() == 0) {
      std::string line;
      if(std::getline(ss, line)) {
        auto pos = line.rfind(" ");
        if(pos != std::string::npos) {
          cmake_version = line.substr(pos + 1);
          thread_count = std::thread::hardware_concurrency();
        }
      }
    }

    return std::string(R"RAW({
  "version": ")RAW" +
                       std::string(JUCI_VERSION) +
                       R"RAW(",
  "gtk_theme": {
    "name_comment": "Use \"\" for default theme, At least these two exist on all systems: Adwaita, Raleigh",
    "name": "",
    "variant_comment": "Use \"\" for default variant, and \"dark\" for dark theme variant. Note that not all themes support dark variant, but for instance Adwaita does",
    "variant": "dark",
    "font_comment": "Set to override theme font, for instance: \"Arial 12\"",
    "font": ""
  },
  "source": {
    "style_comment": "Use \"\" for default style, and for instance juci-dark or juci-dark-blue together with dark gtk_theme variant. Styles from normal gtksourceview install: classic, cobalt, kate, oblivion, solarized-dark, solarized-light, tango",
    "style": "juci-dark",
    "font_comment": "Use \"\" for default font, and for instance \"Monospace 12\" to also set size",)RAW"
#ifdef __APPLE__
                       R"RAW(
    "font": "Menlo",)RAW"
#else
#ifdef _WIN32
                       R"RAW(
    "font": "Consolas",)RAW"
#else
                       R"RAW(
    "font": "Monospace",)RAW"
#endif
#endif
                       R"RAW(
    "add_missing_newline_at_end_of_file_on_save": true,
    "remove_trailing_whitespace_characters_on_save": false,
    "show_whitespace_characters_comment": "Determines what kind of whitespaces should be drawn. Use comma-separated list of: space, tab, newline, nbsp, leading, text, trailing or all",
    "show_whitespace_characters": "",
    "format_style_on_save_comment": "Performs style format on save if supported on language in buffer",
    "format_style_on_save": false,
    "format_style_on_save_if_style_file_found_comment": "Format style if format file is found, even if format_style_on_save is false",
    "format_style_on_save_if_style_file_found": true,
    "smart_brackets_comment": "If smart_inserts is enabled, this option is automatically enabled. When inserting an already closed bracket, the cursor might instead be moved, avoiding the need of arrow keys after autocomplete",
    "smart_brackets": true,
    "smart_inserts_comment": "When for instance inserting (, () gets inserted. Applies to: (), [], \", '. Also enables pressing ; inside an expression before a final ) to insert ; at the end of line, and deletions of empty insertions",
    "smart_inserts": true,
    "show_map": true,
    "map_font_size": 1,
    "show_git_diff": true,
    "show_background_pattern": true,
    "show_right_margin": false,
    "right_margin_position": 80,
    "spellcheck_language_comment": "Use \"\" to set language from your locale settings",
    "spellcheck_language": "en_US",
    "auto_tab_char_and_size_comment": "Use false to always use default tab char and size",
    "auto_tab_char_and_size": true,
    "default_tab_char_comment": "Use \"\t\" for regular tab",
    "default_tab_char": " ",
    "default_tab_size": 2,
    "tab_indents_line": true,
    "word_wrap_comment": "Specify language ids that should enable word wrap, for instance: chdr, c, cpphdr, cpp, js, python, or all to enable word wrap for all languages",
    "word_wrap": "markdown, latex",
    "highlight_current_line": true,
    "show_line_numbers": true,
    "enable_multiple_cursors": false,
    "auto_reload_changed_files": true,
    "search_for_selection": true,
    "clang_format_style_comment": "IndentWidth, AccessModifierOffset and UseTab are set automatically. See http://clang.llvm.org/docs/ClangFormatStyleOptions.html",
    "clang_format_style": "ColumnLimit: 0, NamespaceIndentation: All",
    "clang_tidy_enable_comment": "Enable clang-tidy in new C/C++ buffers",
    "clang_tidy_enable": false,
    "clang_tidy_checks_comment": "In new C/C++ buffers, these checks are appended to the value of 'Checks' in the .clang-tidy file, if any",
    "clang_tidy_checks": "",
    "clang_usages_threads_comment": "The number of threads used in finding usages in unparsed files. -1 corresponds to the number of cores available, and 0 disables the search",
    "clang_usages_threads": -1,
    "clang_detailed_preprocessing_record_comment": "Set to true to, at the cost of increased resource use, include all macro definitions and instantiations when parsing new C/C++ buffers. You should reopen buffers and delete build/.usages_clang after changing this option.",
    "clang_detailed_preprocessing_record": false,
    "debug_place_cursor_at_stop": false
  },
  "terminal": {
    "history_size": 10000,
    "font_comment": "Use \"\" to use source.font with slightly smaller size",
    "font": "",
    "clear_on_compile": true,
    "clear_on_run_command": false,
    "hide_entry_on_run_command": true
  },
  "project": {
    "default_build_path_comment": "Use <project_directory_name> to insert the project top level directory name",
    "default_build_path": "./build",
    "debug_build_path_comment": "Use <project_directory_name> to insert the project top level directory name, and <default_build_path> to insert your default_build_path setting.",
    "debug_build_path": "<default_build_path>/debug",
    "cmake": {)RAW"
#ifdef _WIN32
                       R"RAW(
      "command": "cmake -G\"MSYS Makefiles\"",)RAW"
#else
                       R"RAW(
      "command": "cmake",)RAW"
#endif
                       R"RAW(
      "compile_command": "cmake --build .)RAW" +
                       (thread_count > 1 && !cmake_version.empty() && version_compare(cmake_version, "3.12") >= 0 ? " --parallel " + std::to_string(thread_count) : "") +
                       R"RAW("
    },
    "meson": {
      "command": "meson",
      "compile_command": "ninja"
    },
    "default_build_management_system_comment": "Select which build management system to use when creating a new C or C++ project, for instance \"cmake\" or \"meson\"",
    "default_build_management_system": "cmake",
    "save_on_compile_or_run": true,)RAW"
#ifdef JUCI_USE_UCTAGS
                       R"RAW(
    "ctags_command": "uctags",)RAW"
#else
                       R"RAW(
    "ctags_command": "ctags",)RAW"
#endif
                       R"RAW(
    "grep_command": "grep",
    "cargo_command": "cargo",
    "python_command": "python3 -u",
    "markdown_command": "grip -b"
  },
  "keybindings": {
    "preferences": "<primary>comma",
    "snippets": "",
    "commands": "",
    "quit": "<primary>q",
	"oset_run": "<alt>r",
	"oset_stop": "<alt>d",
	"oset_command": "<alt>c",
	"oset_xcli": "<alt>x",
	"oset_xdebug": "",
    "file_new_file": "<primary>n",
    "file_new_folder": "<primary><shift>n",
    "file_open_file": "<primary>o",
    "file_open_folder": "<primary><shift>o",
    "file_find_file": "<primary>p",
    "file_switch_file_type": "<alt>o",
    "file_reload_file": "",
    "file_save": "<primary>s",
    "file_save_as": "<primary><shift>s",
    "file_close_file": "<primary>w",
    "file_close_folder": "",
    "file_close_project": "",
    "file_close_other_files": "",
    "file_print": "",
    "edit_undo": "<primary>z",
    "edit_redo": "<primary><shift>z",
    "edit_cut": "<primary>x",
    "edit_cut_lines": "<primary><shift>x",
    "edit_copy": "<primary>c",
    "edit_copy_lines": "<primary><shift>c",
    "edit_paste": "<primary>v",
    "edit_extend_selection": "<primary><shift>a",
    "edit_shrink_selection": "<primary><shift><alt>a",
    "edit_show_or_hide": "",
    "edit_find": "<primary>f",
    "source_spellcheck": "",
    "source_spellcheck_clear": "",
    "source_spellcheck_next_error": "<primary><shift>e",
    "source_git_next_diff": "<primary>k",
    "source_git_show_diff": "<alt>k",
    "source_indentation_set_buffer_tab": "",
    "source_indentation_auto_indent_buffer": "<primary><shift>i",
    "source_goto_line": "<primary>g",
    "source_center_cursor": "<primary>l",
    "source_cursor_history_back": "<alt>Left",
    "source_cursor_history_forward": "<alt>Right",
    "source_show_completion_comment": "Add completion keybinding to disable interactive autocompletion",
    "source_show_completion": "",
    "source_find_symbol": "<primary><shift>f",
    "source_find_pattern": "<alt><shift>f",
    "source_comments_toggle": "<primary>slash",
    "source_comments_add_documentation": "<primary><alt>slash",
    "source_find_documentation": "<primary><shift>d",
    "source_goto_declaration": "<primary>d",
    "source_goto_type_declaration": "<alt><shift>d",
    "source_goto_implementation": "<primary>i",
    "source_goto_usage": "<primary>u",
    "source_goto_method": "<primary>m",
    "source_rename": "<primary>r",
    "source_implement_method": "<primary><shift>m",
    "source_goto_next_diagnostic": "<primary>e",
    "source_apply_fix_its": "<control>space",
    "project_set_run_arguments": "",
    "project_compile_and_run": "<primary>Return",
    "project_compile": "<primary><shift>Return",
    "project_run_command": "<alt>Return",
    "project_kill_last_running": "<primary>Escape",
    "project_force_kill_last_running": "<primary><shift>Escape",
    "debug_set_run_arguments": "",
    "debug_start_continue": "<primary>y",
    "debug_stop": "<primary><shift>y",
    "debug_kill": "<primary><shift>k",
    "debug_step_over": "<primary>j",
    "debug_step_into": "<primary>t",
    "debug_step_out": "<primary><shift>t",
    "debug_backtrace": "<primary><shift>j",
    "debug_show_variables": "<primary><shift>b",
    "debug_run_command": "<alt><shift>Return",
    "debug_toggle_breakpoint": "<primary>b",
    "debug_show_breakpoints": "<primary><shift><alt>b",
    "debug_goto_stop": "<primary><shift>l",)RAW"
#ifdef __linux
                       R"RAW(
    "window_next_tab": "<primary>Tab",
    "window_previous_tab": "<primary><shift>Tab",)RAW"
#else
                       R"RAW(
    "window_next_tab": "<primary><alt>Right",
    "window_previous_tab": "<primary><alt>Left",)RAW"
#endif
                       R"RAW(
    "window_goto_tab": "",
    "window_toggle_split": "",
    "window_split_source_buffer": "",)RAW"
#ifdef __APPLE__
                       R"RAW(
    "window_toggle_full_screen": "<primary><control>f",)RAW"
#else
                       R"RAW(
    "window_toggle_full_screen": "F11",)RAW"
#endif
                       R"RAW(
    "window_toggle_directories": "",
    "window_toggle_terminal": "",
    "window_toggle_status": "",
    "window_toggle_menu": "",
    "window_toggle_tabs": "",
    "window_toggle_zen_mode": "",
    "window_clear_terminal": ""
  },
  "documentation_searches": {
    "clang": {
      "separator": "::",
      "queries": {
        "@empty": "https://www.google.com/search?q=c%2B%2B+",
        "std": "https://www.google.com/search?q=site:http://www.cplusplus.com/reference/+",
        "boost": "https://www.google.com/search?q=site:http://www.boost.org/doc/libs/1_59_0/+",
        "Gtk": "https://www.google.com/search?q=site:https://developer.gnome.org/gtkmm/stable/+",
        "@any": "https://www.google.com/search?q="
      }
    }
  },
  "log": {
    "libclang_comment": "Outputs diagnostics for new C/C++ buffers",
    "libclang": false,
    "language_server": false
  }
}
)RAW");
  };
  static auto config = get_config();
  return config;
}

const char *Config::juci_light_style() {
  return R"RAW(<?xml version="1.0" encoding="UTF-8"?>

<style-scheme id="juci-light" _name="juci" version="1.0">
  <author>juCi++ team</author>
  <_description>Default juCi++ style</_description>

  <!-- Palette -->
  <color name="white"                       value="#FFFFFF"/>
  <color name="black"                       value="#000000"/>
  <color name="gray"                        value="#888888"/>
  <color name="red"                         value="#CC0000"/>
  <color name="green"                       value="#008800"/>
  <color name="blue"                        value="#0000FF"/>
  <color name="dark-blue"                   value="#002299"/>
  <color name="yellow"                      value="#FFFF00"/>
  <color name="light-yellow"                value="#FFFF88"/>
  <color name="orange"                      value="#FF8800"/>
  <color name="purple"                      value="#990099"/>

  <style name="text"                        foreground="#000000" background="#FFFFFF"/>
  <style name="background-pattern"          background="#rgba(0,0,0,.03)"/>
  <style name="selection"                   background="#4A90D9"/>

  <!-- Current Line Highlighting -->
  <style name="current-line"                background="#rgba(0,0,0,.07)"/>

  <!-- Bracket Matching -->
  <style name="bracket-match"               foreground="white" background="gray" bold="true"/>
  <style name="bracket-mismatch"            foreground="white" background="#FF0000" bold="true"/>

  <!-- Search Matching -->
  <style name="search-match"                foreground="#000000" background="#FFFF00"/>

  <!-- Language specifics -->
  <style name="def:builtin"                 foreground="blue"/>
  <style name="def:constant"                foreground="blue"/>
  <style name="def:boolean"                 foreground="red"/>
  <style name="def:decimal"                 foreground="red"/>
  <style name="def:base-n-integer"          foreground="red"/>
  <style name="def:floating-point"          foreground="red"/>
  <style name="def:complex"                 foreground="red"/>
  <style name="def:character"               foreground="red"/>
  <style name="def:special-char"            foreground="red"/>

  <!-- Language specifics used by clang-parser in default config -->
  <style name="def:string"                  foreground="red"/>
  <style name="def:comment"                 foreground="gray"/>
  <style name="def:statement"               foreground="blue"/>
  <style name="def:type"                    foreground="blue"/>
  <style name="def:function"                foreground="dark-blue"/>
  <style name="def:identifier"              foreground="purple"/>
  <style name="def:preprocessor"            foreground="green"/>
  <style name="def:error"                   foreground="red"/>
  <style name="def:warning"                 foreground="orange"/>
  <style name="def:note"                    foreground="black" background="light-yellow"/>

  <style name="diff:added-line"             foreground="green"/>
  <style name="diff:removed-line"           foreground="red"/>
  <style name="diff:changed-line"           foreground="orange"/>
  <style name="diff:diff-file"              use-style="def:type"/>
  <style name="diff:location"               use-style="def:statement"/>
  <style name="diff:special-case"           use-style="def:constant"/>

</style-scheme>
)RAW";
}

const char *Config::juci_dark_style() {
  return R"RAW(<?xml version="1.0" encoding="UTF-8"?>

<style-scheme id="juci-dark" _name="juci" version="1.0">
  <author>juCi++ team</author>
  <_description>Dark juCi++ style</_description>

  <!-- Palette -->
  <color name="white"                       value="#D6D6D6"/>
  <color name="black"                       value="#202428"/>
  <color name="gray"                        value="#919191"/>
  <color name="red"                         value="#FF9999"/>
  <color name="yellow"                      value="#EEEE66"/>
  <color name="green"                       value="#AACC99"/>
  <color name="blue"                        value="#88AAFF"/>
  <color name="light-blue"                  value="#AABBEE"/>
  <color name="purple"                      value="#DD99DD"/>

  <style name="text"                        foreground="white" background="black"/>
  <style name="background-pattern"          background="#rgba(255,255,255,.04)"/>
  <style name="selection"                   background="#215D9C"/>

  <!-- Current Line Highlighting -->
  <style name="current-line"                background="#rgba(255,255,255,.05)"/>

  <!-- Bracket Matching -->
  <style name="bracket-match"               foreground="black" background="gray" bold="true"/>
  <style name="bracket-mismatch"            foreground="black" background="#FF0000" bold="true"/>

  <!-- Search Matching -->
  <style name="search-match"                foreground="#000000" background="#FFFF00"/>

  <!-- Language specifics -->
  <style name="def:builtin"                 foreground="blue"/>
  <style name="def:constant"                foreground="blue"/>
  <style name="def:boolean"                 foreground="red"/>
  <style name="def:decimal"                 foreground="red"/>
  <style name="def:base-n-integer"          foreground="red"/>
  <style name="def:floating-point"          foreground="red"/>
  <style name="def:complex"                 foreground="red"/>
  <style name="def:character"               foreground="red"/>
  <style name="def:special-char"            foreground="red"/>

  <!-- Language specifics used by clang-parser in default config -->
  <style name="def:string"                  foreground="red"/>
  <style name="def:comment"                 foreground="gray"/>
  <style name="def:statement"               foreground="blue"/>
  <style name="def:type"                    foreground="blue"/>
  <style name="def:function"                foreground="light-blue"/>
  <style name="def:identifier"              foreground="purple"/>
  <style name="def:preprocessor"            foreground="green"/>
  <style name="def:error"                   foreground="red"/>
  <style name="def:warning"                 foreground="yellow"/>
  <style name="def:note"                    foreground="#E6E6E6" background="#383F46"/>

  <style name="diff:added-line"             foreground="green"/>
  <style name="diff:removed-line"           foreground="red"/>
  <style name="diff:changed-line"           foreground="yellow"/>
  <style name="diff:diff-file"              use-style="def:type"/>
  <style name="diff:location"               use-style="def:statement"/>
  <style name="diff:special-case"           use-style="def:constant"/>

</style-scheme>
)RAW";
}

const char *Config::juci_dark_blue_style() {
  return R"RAW(<?xml version="1.0" encoding="UTF-8"?>

<style-scheme id="juci-dark-blue" _name="juci" version="1.0">
  <author>juCi++ team</author>
  <_description>Dark blue juCi++ style based on the Emacs deeper blue theme</_description>

  <!-- Palette -->
  <color name="white"                       value="#D6D6D6"/>
  <color name="dark-blue"                   value="#202233"/>
  <color name="gray"                        value="#919191"/>
  <color name="red"                         value="#FF7777"/>
  <color name="yellow"                      value="#FFE100"/>
  <color name="light-yellow"                value="#EAC595"/>
  <color name="blue"                        value="#00CCFF"/>
  <color name="green"                       value="#14ECA8"/>
  <color name="light-blue"                  value="#8BFAFF"/>
  <color name="light-green"                 value="#A0DB6B"/>

  <style name="text"                        foreground="white" background="dark-blue"/>
  <style name="background-pattern"          background="#rgba(255,255,255,.04)"/>
  <style name="selection"                   background="#215D9C"/>

  <!-- Current Line Highlighting -->
  <style name="current-line"                background="#rgba(255,255,255,.05)"/>

  <!-- Bracket Matching -->
  <style name="bracket-match"               foreground="dark-blue" background="gray" bold="true"/>
  <style name="bracket-mismatch"            foreground="dark-blue" background="#FF0000" bold="true"/>

  <!-- Search Matching -->
  <style name="search-match"                foreground="#000000" background="#FFFF00"/>

  <!-- Language specifics -->
  <style name="def:builtin"                 foreground="blue"/>
  <style name="def:constant"                foreground="blue"/>
  <style name="def:boolean"                 foreground="light-yellow"/>
  <style name="def:decimal"                 foreground="light-yellow"/>
  <style name="def:base-n-integer"          foreground="light-yellow"/>
  <style name="def:floating-point"          foreground="light-yellow"/>
  <style name="def:complex"                 foreground="light-yellow"/>
  <style name="def:character"               foreground="light-yellow"/>
  <style name="def:special-char"            foreground="light-yellow"/>

  <!-- Language specifics used by clang-parser in default config -->
  <style name="def:string"                  foreground="light-yellow"/>
  <style name="def:comment"                 foreground="gray"/>
  <style name="def:statement"               foreground="blue"/>
  <style name="def:type"                    foreground="blue"/>
  <style name="def:function"                foreground="light-blue"/>
  <style name="def:identifier"              foreground="light-green"/>
  <style name="def:preprocessor"            foreground="yellow"/>
  <style name="def:error"                   foreground="red"/>
  <style name="def:warning"                 foreground="yellow"/>
  <style name="def:note"                    foreground="#E6E6E6" background="#383C59"/>

  <style name="diff:added-line"             foreground="green"/>
  <style name="diff:removed-line"           foreground="red"/>
  <style name="diff:changed-line"           foreground="yellow"/>
  <style name="diff:diff-file"              use-style="def:type"/>
  <style name="diff:location"               use-style="def:statement"/>
  <style name="diff:special-case"           use-style="def:constant"/>

</style-scheme>
)RAW";
}
