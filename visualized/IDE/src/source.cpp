#include "source.hpp"
#include "config.hpp"
#include "ctags.hpp"
#include "directories.hpp"
#include "filesystem.hpp"
#include "git.hpp"
#include "info.hpp"
#include "json.hpp"
#include "menu.hpp"
#include "selection_dialog.hpp"
#include "terminal.hpp"
#include "utility.hpp"
#include <algorithm>
#include <boost/spirit/home/qi/char.hpp>
#include <boost/spirit/home/qi/operator.hpp>
#include <boost/spirit/home/qi/string.hpp>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <regex>
#include <set>

#ifdef _WIN32
#include <windows.h>
inline DWORD get_current_process_id() {
  return GetCurrentProcessId();
}
#else
#include <unistd.h>
inline pid_t get_current_process_id() {
  return getpid();
}
#endif

std::unique_ptr<TinyProcessLib::Process> Source::View::prettier_background_process = {};

Glib::RefPtr<Gsv::LanguageManager> Source::LanguageManager::get_default() {
  static auto instance = Gsv::LanguageManager::create();
  return instance;
}
Glib::RefPtr<Gsv::StyleSchemeManager> Source::StyleSchemeManager::get_default() {
  static auto instance = Gsv::StyleSchemeManager::create();
  static bool first = true;
  if(first) {
    instance->prepend_search_path((Config::get().home_juci_path / "styles").string());
    first = false;
  }
  return instance;
}

Glib::RefPtr<Gsv::Language> Source::guess_language(const boost::filesystem::path &file_path) {
  auto language_manager = LanguageManager::get_default();
  bool result_uncertain = false;
  auto filename = file_path.filename().string();
  auto content_type = Gio::content_type_guess(filename, nullptr, 0, result_uncertain);
  if(result_uncertain)
    content_type.clear();
  auto language = language_manager->guess_language(filename, content_type);
  if(!language) {
    auto extension = file_path.extension().string();
    if(filename == "CMakeLists.txt")
      language = language_manager->get_language("cmake");
    else if(filename == "meson.build")
      language = language_manager->get_language("meson");
    else if(filename == "Makefile")
      language = language_manager->get_language("makefile");
    else if(filename == "Jenkinsfile")
      language = language_manager->get_language("groovy");
    else if(extension == ".tcc")
      language = language_manager->get_language("cpphdr");
    else if(extension == ".ts" || extension == ".tsx" || extension == ".jsx" || extension == ".flow")
      language = language_manager->get_language("js");
    else if(extension == ".svelte")
      language = language_manager->get_language("html");
    else if(extension == ".vert" || // listed on https://github.com/KhronosGroup/glslang
            extension == ".frag" ||
            extension == ".tesc" ||
            extension == ".tese" ||
            extension == ".geom" ||
            extension == ".comp")
      language = language_manager->get_language("glsl");
    else if(!file_path.has_extension()) {
      for(auto &part : file_path) {
        if(part == "include") {
          language = language_manager->get_language("cpphdr");
          break;
        }
      }
    }
  }
  else if(language->get_id() == "cuda") {
    if(file_path.extension() == ".cuh")
      language = language_manager->get_language("cpphdr");
    else
      language = language_manager->get_language("cpp");
  }
  else if(language->get_id() == "opencl")
    language = language_manager->get_language("cpp");
  return language;
}

Source::FixIt::FixIt(std::string source_, std::string path_, std::pair<Offset, Offset> offsets_) : source(std::move(source_)), path(std::move(path_)), offsets(std::move(offsets_)) {
  if(this->source.size() == 0)
    type = Type::erase;
  else {
    if(this->offsets.first == this->offsets.second)
      type = Type::insert;
    else
      type = Type::replace;
  }
}

std::string Source::FixIt::string(BaseView &view) {
  bool in_current_view = path == view.file_path;

  auto from_pos = (!in_current_view ? boost::filesystem::path(path).filename().string() + ':' : "") + std::to_string(offsets.first.line + 1) + ':' + std::to_string(offsets.first.index + 1);
  std::string to_pos, text;
  if(type != Type::insert) {
    to_pos = std::to_string(offsets.second.line + 1) + ':' + std::to_string(offsets.second.index + 1);
    if(in_current_view) {
      text = view.get_buffer()->get_text(view.get_iter_at_line_index(offsets.first.line, offsets.first.index),
                                         view.get_iter_at_line_index(offsets.second.line, offsets.second.index));
    }
  }

  if(type == Type::insert)
    return "Insert " + source + " at " + from_pos;
  else if(type == Type::replace)
    return "Replace " + (!text.empty() ? text + " at " : "") + from_pos + " - " + to_pos + " with " + source;
  else
    return "Erase " + (!text.empty() ? text + " at " : "") + from_pos + " - " + to_pos;
}

//////////////
//// View ////
//////////////
std::set<Source::View *> Source::View::non_deleted_views;
std::set<Source::View *> Source::View::views;

Source::View::View(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language, bool is_generic_view) : BaseView(file_path, language), SpellCheckView(file_path, language), DiffView(file_path, language) {
  non_deleted_views.emplace(this);
  views.emplace(this);

  similar_symbol_tag = get_buffer()->create_tag();
  similar_symbol_tag->property_weight() = Pango::WEIGHT_ULTRAHEAVY;
  similar_symbol_tag->property_background_rgba() = Gdk::RGBA("rgba(255, 255, 255, 0.075)");
  clickable_tag = get_buffer()->create_tag();
  clickable_tag->property_underline() = Pango::Underline::UNDERLINE_SINGLE;
  clickable_tag->property_underline_set() = true;

  get_buffer()->create_tag("def:warning_underline");
  get_buffer()->create_tag("def:error_underline");

  auto mark_attr_debug_breakpoint = Gsv::MarkAttributes::create();
  Gdk::RGBA rgba;
  rgba.set_red(1.0);
  rgba.set_green(0.5);
  rgba.set_blue(0.5);
  rgba.set_alpha(0.3);
  mark_attr_debug_breakpoint->set_background(rgba);
  set_mark_attributes("debug_breakpoint", mark_attr_debug_breakpoint, 100);
  auto mark_attr_debug_stop = Gsv::MarkAttributes::create();
  rgba.set_red(0.5);
  rgba.set_green(0.5);
  rgba.set_blue(1.0);
  mark_attr_debug_stop->set_background(rgba);
  set_mark_attributes("debug_stop", mark_attr_debug_stop, 101);
  auto mark_attr_debug_breakpoint_and_stop = Gsv::MarkAttributes::create();
  rgba.set_red(0.75);
  rgba.set_green(0.5);
  rgba.set_blue(0.75);
  mark_attr_debug_breakpoint_and_stop->set_background(rgba);
  set_mark_attributes("debug_breakpoint_and_stop", mark_attr_debug_breakpoint_and_stop, 102);

  hide_tag = get_buffer()->create_tag();
  hide_tag->property_scale() = 0.25;

  if(is_c || is_cpp) {
    use_fixed_continuation_indenting = false;
    // TODO 2019: check if clang-format has improved...
    // boost::filesystem::path clang_format_file;
    // auto search_path=file_path.parent_path();
    // boost::system::error_code ec;
    // while(true) {
    //   clang_format_file=search_path/".clang-format";
    //   if(boost::filesystem::exists(clang_format_file, ec))
    //     break;
    //   clang_format_file=search_path/"_clang-format";
    //   if(boost::filesystem::exists(clang_format_file, ec))
    //     break;
    //   clang_format_file.clear();

    //   if(search_path==search_path.root_directory())
    //     break;
    //   search_path=search_path.parent_path();
    // }
    // if(!clang_format_file.empty()) {
    //   auto lines=filesystem::read_lines(clang_format_file);
    //   for(auto &line: lines) {
    //     std::cout << "1" << std::endl;
    //     if(!line.empty() && line.compare(0, 23, "ContinuationIndentWidth")==0) {
    //       std::cout << "2" << std::endl;
    //       use_continuation_indenting=true;
    //       break;
    //     }
    //   }
    // }
  }

  setup_signals();
  setup_format_style(is_generic_view);


  std::string comment_characters;
  if(is_bracket_language)
    comment_characters = "//";
  else {
    if(is_language({"cmake", "makefile", "python", "python3", "sh", "perl", "ruby", "r", "asm", "automake", "yaml", "docker", "julia"}))
      comment_characters = "#";
    else if(is_language({"latex", "matlab", "octave", "bibtex"}))
      comment_characters = "%";
    else if(language_id == "fortran")
      comment_characters = "!";
    else if(language_id == "pascal")
      comment_characters = "//";
    else if(language_id == "lua")
      comment_characters = "--";
  }
  if(!comment_characters.empty()) {
    toggle_comments = [this, comment_characters = std::move(comment_characters)] {
      std::vector<int> lines;
      Gtk::TextIter selection_start, selection_end;
      get_buffer()->get_selection_bounds(selection_start, selection_end);
      auto line_start = selection_start.get_line();
      auto line_end = selection_end.get_line();
      if(line_start != line_end && selection_end.starts_line())
        --line_end;
      bool lines_commented = true;
      bool extra_spaces = true;
      int min_indentation = std::numeric_limits<int>::max();
      for(auto line = line_start; line <= line_end; ++line) {
        auto iter = get_buffer()->get_iter_at_line(line);
        bool line_added = false;
        bool line_commented = false;
        bool extra_space = false;
        int indentation = 0;
        for(;;) {
          if(iter.ends_line())
            break;
          else if(*iter == ' ' || *iter == '\t') {
            ++indentation;
            iter.forward_char();
            continue;
          }
          else {
            lines.emplace_back(line);
            line_added = true;
            for(size_t c = 0; c < comment_characters.size(); ++c) {
              if(iter.ends_line()) {
                break;
              }
              else if(*iter == static_cast<unsigned int>(comment_characters[c])) {
                if(c < comment_characters.size() - 1) {
                  iter.forward_char();
                  continue;
                }
                else {
                  line_commented = true;
                  if(!iter.ends_line()) {
                    iter.forward_char();
                    if(*iter == ' ')
                      extra_space = true;
                  }
                  break;
                }
              }
              else
                break;
            }
            break;
          }
        }
        if(line_added) {
          lines_commented &= line_commented;
          extra_spaces &= extra_space;
          min_indentation = std::min(min_indentation, indentation);
        }
      }
      if(lines.size()) {
        auto comment_characters_and_space = comment_characters + ' ';
        get_buffer()->begin_user_action();
        for(auto &line : lines) {
          auto iter = get_buffer()->get_iter_at_line(line);
          if(min_indentation != std::numeric_limits<int>::max())
            iter.forward_chars(min_indentation);
          if(lines_commented) {
            auto end_iter = iter;
            end_iter.forward_chars(comment_characters.size() + static_cast<int>(extra_spaces));
            while(*iter == ' ' || *iter == '\t') {
              iter.forward_char();
              end_iter.forward_char();
            }
            get_buffer()->erase(iter, end_iter);
          }
          else
            get_buffer()->insert(iter, comment_characters_and_space);
        }
        get_buffer()->end_user_action();
      }
    };
  }

  get_methods = [this]() {
    std::vector<std::pair<Offset, std::string>> methods;
    boost::filesystem::path file_path;
    boost::system::error_code ec;
    bool use_tmp_file = false;
    bool is_cpp_standard_header = is_cpp && this->file_path.extension().empty();

    if(this->get_buffer()->get_modified() || is_cpp_standard_header) {
      use_tmp_file = true;
      file_path = boost::filesystem::temp_directory_path(ec);
      if(ec) {
        Terminal::get().print("\e[31mError\e[m: could not get temporary directory folder\n", true);
        return methods;
      }
      file_path /= "jucipp_get_methods" + std::to_string(get_current_process_id());
      boost::filesystem::create_directory(file_path, ec);
      if(ec) {
        Terminal::get().print("\e[31mError\e[m: could not create temporary folder\n", true);
        return methods;
      }
      file_path /= this->file_path.filename().string() + (is_cpp_standard_header ? ".hpp" : "");
      filesystem::write(file_path, this->get_buffer()->get_text().raw());
    }
    else
      file_path = this->file_path;

    Ctags ctags(file_path, true, true);
    if(use_tmp_file)
      boost::filesystem::remove_all(file_path.parent_path(), ec);
    if(!ctags) {
      Info::get().print("No methods found in current buffer");
      return methods;
    }

    std::string line;
    while(std::getline(ctags.output, line)) {
      auto location = ctags.get_location(line, true, is_cpp);
      std::transform(location.kind.begin(), location.kind.end(), location.kind.begin(),
                     [](char c) { return std::tolower(c); });
      std::vector<std::string> ignore_kinds = {"variable", "local", "constant", "global", "property", "member", "enum",
                                               "class", "struct", "namespace",
                                               "macro", "param", "header",
                                               "typedef", "using", "alias",
                                               "project", "option"};
      if(std::none_of(ignore_kinds.begin(), ignore_kinds.end(), [&location](const std::string &e) { return location.kind.find(e) != std::string::npos; }) &&
         location.source.find("<b>") != std::string::npos) {
        std::string scope = !location.scope.empty() ? Glib::Markup::escape_text(location.scope) : "";
        if(is_cpp && !scope.empty()) { // Remove namespace from location.source in C++ source files
          auto pos = location.source.find(scope + "::<b>");
          if(pos != std::string::npos)
            location.source.erase(pos, scope.size() + 2);
        }
        methods.emplace_back(Offset(location.line, location.index), (!scope.empty() ? scope + ":" : "") + std::to_string(location.line + 1) + ": " + location.source);
      }
    }
    std::sort(methods.begin(), methods.end(), [](const std::pair<Offset, std::string> &e1, const std::pair<Offset, std::string> &e2) {
      return e1.first < e2.first;
    });

    if(methods.empty())
      Info::get().print("No methods found in current buffer");
    return methods;
  };
}

Gsv::DrawSpacesFlags Source::View::parse_show_whitespace_characters(const std::string &text) {
  namespace qi = boost::spirit::qi;

  qi::symbols<char, Gsv::DrawSpacesFlags> options;
  options.add("space", Gsv::DRAW_SPACES_SPACE)("tab", Gsv::DRAW_SPACES_TAB)("newline", Gsv::DRAW_SPACES_NEWLINE)("nbsp", Gsv::DRAW_SPACES_NBSP)("leading", Gsv::DRAW_SPACES_LEADING)("text", Gsv::DRAW_SPACES_TEXT)("trailing", Gsv::DRAW_SPACES_TRAILING)("all", Gsv::DRAW_SPACES_ALL);

  std::set<Gsv::DrawSpacesFlags> out;

  // parse comma-separated list of options
  qi::phrase_parse(text.begin(), text.end(), options % ',', qi::space, out);

  return out.count(Gsv::DRAW_SPACES_ALL) > 0 ? Gsv::DRAW_SPACES_ALL : static_cast<Gsv::DrawSpacesFlags>(std::accumulate(out.begin(), out.end(), 0));
}

bool Source::View::save() {
  if(file_path.empty() || !get_buffer()->get_modified())
    return false;

  get_buffer()->begin_user_action();

  if(Config::get().source.remove_trailing_whitespace_characters_on_save)
    remove_trailing_whitespace_characters();
  if(Config::get().source.add_missing_newline_at_end_of_file_on_save)
    add_missing_newline_at_end_of_file();

  if(format_style && file_path.filename() != "package.json") {
    if(Config::get().source.format_style_on_save)
      format_style(true, true);
    else if(Config::get().source.format_style_on_save_if_style_file_found)
      format_style(false, true);
    hide_tooltips();
  }

  get_buffer()->end_user_action();

  try {
    auto io_channel = Glib::IOChannel::create_from_file(file_path.string(), "w");
    auto start_iter = get_buffer()->begin();
    auto end_iter = start_iter;
    while(start_iter) {
      end_iter.forward_chars(131072);
      io_channel->write(get_buffer()->get_text(start_iter, end_iter));
      start_iter = end_iter;
    }
  }
  catch(const Glib::Error &error) {
    Terminal::get().print("\e[31mError\e[m: could not save file " + filesystem::get_short_path(file_path).string() + ": " + error.what() + '\n', true);
    return false;
  }

  boost::system::error_code ec;
  last_write_time = boost::filesystem::last_write_time(file_path, ec);
  if(ec)
    last_write_time.reset();
  // Remonitor file in case it did not exist before
  monitor_file();
  get_buffer()->set_modified(false);
  Directories::get().on_save_file(file_path);
  return true;
}

void Source::View::configure() {
  SpellCheckView::configure();
  DiffView::configure();

  if(Config::get().source.style.size() > 0) {
    auto scheme = StyleSchemeManager::get_default()->get_scheme(Config::get().source.style);
    if(scheme)
      get_source_buffer()->set_style_scheme(scheme);
  }

  set_draw_spaces(parse_show_whitespace_characters(Config::get().source.show_whitespace_characters));
  { // Set Word Wrap
    namespace qi = boost::spirit::qi;
    std::set<std::string> word_wrap_language_ids;
    qi::phrase_parse(Config::get().source.word_wrap.begin(), Config::get().source.word_wrap.end(), (+(~qi::char_(','))) % ',', qi::space, word_wrap_language_ids);
    if(std::any_of(word_wrap_language_ids.begin(), word_wrap_language_ids.end(), [this](const std::string &word_wrap_language_id) {
         return word_wrap_language_id == language_id || word_wrap_language_id == "all";
       }))
      set_wrap_mode(Gtk::WrapMode::WRAP_WORD_CHAR);
    else
      set_wrap_mode(Gtk::WrapMode::WRAP_NONE);
  }
  property_highlight_current_line() = Config::get().source.highlight_current_line;
  line_renderer->set_visible(Config::get().source.show_line_numbers);

#if GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION >= 20)
  Gdk::Rectangle rectangle;
  get_iter_location(get_buffer()->begin(), rectangle);
  set_bottom_margin((rectangle.get_height() + get_pixels_above_lines() + get_pixels_below_lines()) * 10);
#endif

  if(Config::get().source.show_background_pattern)
    gtk_source_view_set_background_pattern(this->gobj(), GTK_SOURCE_BACKGROUND_PATTERN_TYPE_GRID);
  else
    gtk_source_view_set_background_pattern(this->gobj(), GTK_SOURCE_BACKGROUND_PATTERN_TYPE_NONE);
  if((property_show_right_margin() = Config::get().source.show_right_margin))
    property_right_margin_position() = Config::get().source.right_margin_position;

  // Create tags for diagnostic warnings and errors:
  auto scheme = get_source_buffer()->get_style_scheme();
  auto tag_table = get_buffer()->get_tag_table();
  auto style = scheme->get_style("def:warning");
  auto diagnostic_tag_underline = get_buffer()->get_tag_table()->lookup("def:warning_underline");
  if(style && (style->property_foreground_set() || style->property_background_set())) {
    Glib::ustring warning_property;
    if(style->property_foreground_set())
      warning_property = style->property_foreground().get_value();
    else if(style->property_background_set())
      warning_property = style->property_background().get_value();

    diagnostic_tag_underline->property_underline() = Pango::Underline::UNDERLINE_ERROR;
    auto tag_class = G_OBJECT_GET_CLASS(diagnostic_tag_underline->gobj()); // For older GTK+ 3 versions:
    auto param_spec = g_object_class_find_property(tag_class, "underline-rgba");
    if(param_spec)
      diagnostic_tag_underline->set_property("underline-rgba", Gdk::RGBA(warning_property));
  }
  style = scheme->get_style("def:error");
  diagnostic_tag_underline = get_buffer()->get_tag_table()->lookup("def:error_underline");
  if(style && (style->property_foreground_set() || style->property_background_set())) {
    Glib::ustring error_property;
    if(style->property_foreground_set())
      error_property = style->property_foreground().get_value();
    else if(style->property_background_set())
      error_property = style->property_background().get_value();

    diagnostic_tag_underline->property_underline() = Pango::Underline::UNDERLINE_ERROR;
    diagnostic_tag_underline->set_property("underline-rgba", Gdk::RGBA(error_property));
  }
  // TODO: clear tag_class and param_spec?

  style = scheme->get_style("selection");
  if(style && style->property_foreground_set())
    extra_cursor_selection->property_foreground() = style->property_foreground().get_value();
  else
    extra_cursor_selection->property_foreground_rgba() = get_style_context()->get_color(Gtk::StateFlags::STATE_FLAG_SELECTED);
  if(style && style->property_background_set())
    extra_cursor_selection->property_background() = style->property_background().get_value();
  else
    extra_cursor_selection->property_background_rgba() = get_style_context()->get_background_color(Gtk::StateFlags::STATE_FLAG_SELECTED);

  if(Config::get().menu.keys["source_show_completion"].empty()) {
    get_completion()->unblock_interactive();
    interactive_completion = true;
  }
  else {
    get_completion()->block_interactive();
    interactive_completion = false;
  }
}

void Source::View::setup_signals() {
  get_buffer()->signal_changed().connect([this]() {
    if(update_status_location)
      update_status_location(this);

    hide_tooltips();

    if(similar_symbol_tag_applied) {
      get_buffer()->remove_tag(similar_symbol_tag, get_buffer()->begin(), get_buffer()->end());
      similar_symbol_tag_applied = false;
    }
    if(clickable_tag_applied) {
      get_buffer()->remove_tag(clickable_tag, get_buffer()->begin(), get_buffer()->end());
      clickable_tag_applied = false;
    }

    previous_extended_selections.clear();
  });


  // Line numbers
  line_renderer = Gtk::manage(new Gsv::GutterRendererText());
  auto gutter = get_gutter(Gtk::TextWindowType::TEXT_WINDOW_LEFT);

  line_renderer->set_alignment_mode(Gsv::GutterRendererAlignmentMode::GUTTER_RENDERER_ALIGNMENT_MODE_FIRST);
  line_renderer->set_alignment(1.0, -1);
  line_renderer->set_padding(3, -1);
  gutter->insert(line_renderer, GTK_SOURCE_VIEW_GUTTER_POSITION_LINES);

  auto set_line_renderer_width = [this] {
    int width, height;
    line_renderer->measure(std::to_string(get_buffer()->get_line_count()), width, height);
    line_renderer->set_size(width);
  };
  set_line_renderer_width();
  get_buffer()->signal_changed().connect([set_line_renderer_width] {
    set_line_renderer_width();
  });
  signal_style_updated().connect([set_line_renderer_width] {
    set_line_renderer_width();
  });
  line_renderer->signal_query_data().connect([this](const Gtk::TextIter &start, const Gtk::TextIter &end, Gsv::GutterRendererState state) {
    if(!start.begins_tag(hide_tag) && !start.has_tag(hide_tag)) {
      if(start.get_line() == get_buffer()->get_insert()->get_iter().get_line())
        line_renderer->set_text(Gsv::Markup("<b>" + std::to_string(start.get_line() + 1) + "</b>"));
      else
        line_renderer->set_text(Gsv::Markup(std::to_string(start.get_line() + 1)));
    }
  });
  line_renderer->signal_query_activatable().connect([](const Gtk::TextIter &, const Gdk::Rectangle &, GdkEvent *) {
    return true;
  });
  line_renderer->signal_activate().connect([this](const Gtk::TextIter &iter, const Gdk::Rectangle &, GdkEvent *) {
    if(toggle_breakpoint)
      toggle_breakpoint(iter.get_line());
  });

  type_tooltips.on_motion = [this] {
    delayed_tooltips_connection.disconnect();
  };
  diagnostic_tooltips.on_motion = [this] {
    delayed_tooltips_connection.disconnect();
  };


  signal_motion_notify_event().connect([this](GdkEventMotion *event) {
    if(on_motion_last_x != event->x || on_motion_last_y != event->y) {
      delayed_tooltips_connection.disconnect();
      if((event->state & GDK_BUTTON1_MASK) == 0) {
        delayed_tooltips_connection = Glib::signal_timeout().connect(
            [this, x = event->x, y = event->y]() {
              type_tooltips.hide();
              diagnostic_tooltips.hide();
              Tooltips::init();
              Gdk::Rectangle rectangle(x, y, 1, 1);
              if(parsed) {
                show_type_tooltips(rectangle);
                show_diagnostic_tooltips(rectangle);
              }
              return false;
            },
            100);
      }

      if(clickable_tag_applied) {
        get_buffer()->remove_tag(clickable_tag, get_buffer()->begin(), get_buffer()->end());
        clickable_tag_applied = false;
      }
      if((event->state & primary_modifier_mask) && !(event->state & GDK_SHIFT_MASK) && !(event->state & GDK_BUTTON1_MASK)) {
        delayed_tag_clickable_connection.disconnect();
        delayed_tag_clickable_connection = Glib::signal_timeout().connect(
            [this, x = event->x, y = event->y]() {
              int buffer_x, buffer_y;
              window_to_buffer_coords(Gtk::TextWindowType::TEXT_WINDOW_TEXT, x, y, buffer_x, buffer_y);
              Gtk::TextIter iter;
              get_iter_at_location(iter, buffer_x, buffer_y);
              apply_clickable_tag(iter);
              clickable_tag_applied = true;
              return false;
            },
            100);
      }

      auto last_mouse_pos = std::make_pair<int, int>(on_motion_last_x, on_motion_last_y);
      auto mouse_pos = std::make_pair<int, int>(event->x, event->y);
      type_tooltips.hide(last_mouse_pos, mouse_pos);
      diagnostic_tooltips.hide(last_mouse_pos, mouse_pos);
    }
    on_motion_last_x = event->x;
    on_motion_last_y = event->y;
    return false;
  });

  get_buffer()->signal_mark_set().connect([this](const Gtk::TextIter &iterator, const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark) {
    auto mark_name = mark->get_name();
    if(mark_name == "selection_bound") {
      if(get_buffer()->get_has_selection())
        delayed_tooltips_connection.disconnect();

      if(update_status_location)
        update_status_location(this);

      if(!keep_previous_extended_selections)
        previous_extended_selections.clear();
    }
    else if(mark_name == "insert") {
      hide_tooltips();

      delayed_tooltips_connection.disconnect();
      delayed_tooltips_connection = Glib::signal_timeout().connect(
          [this]() {
            Tooltips::init();
            Gdk::Rectangle rectangle;
            get_iter_location(get_buffer()->get_insert()->get_iter(), rectangle);
            int location_window_x, location_window_y;
            buffer_to_window_coords(Gtk::TextWindowType::TEXT_WINDOW_TEXT, rectangle.get_x(), rectangle.get_y(), location_window_x, location_window_y);
            rectangle.set_x(location_window_x - 2);
            rectangle.set_y(location_window_y);
            rectangle.set_width(5);
            if(parsed) {
              show_type_tooltips(rectangle);
              show_diagnostic_tooltips(rectangle);
            }
            return false;
          },
          500);

      delayed_tag_similar_symbols_connection.disconnect();
      delayed_tag_similar_symbols_connection = Glib::signal_timeout().connect(
          [this] {
            apply_similar_symbol_tag();
            similar_symbol_tag_applied = true;
            return false;
          },
          100);

      if(SelectionDialog::get())
        SelectionDialog::get()->hide();
      if(CompletionDialog::get())
        CompletionDialog::get()->hide();

      if(update_status_location)
        update_status_location(this);

      if(!keep_previous_extended_selections)
        previous_extended_selections.clear();
    }
  });

  signal_key_release_event().connect([this](GdkEventKey *event) {
    if((event->state & primary_modifier_mask) && clickable_tag_applied) {
      get_buffer()->remove_tag(clickable_tag, get_buffer()->begin(), get_buffer()->end());
      clickable_tag_applied = false;
    }
    return false;
  });

  signal_scroll_event().connect([this](GdkEventScroll *event) {
    hide_tooltips();
    hide_dialogs();
    return false;
  });

  signal_focus_out_event().connect([this](GdkEventFocus *event) {
    hide_tooltips();
    if(clickable_tag_applied) {
      get_buffer()->remove_tag(clickable_tag, get_buffer()->begin(), get_buffer()->end());
      clickable_tag_applied = false;
    }
    return false;
  });

  signal_leave_notify_event().connect([this](GdkEventCrossing *) {
    delayed_tooltips_connection.disconnect();
    return false;
  });
}

void Source::View::setup_format_style(bool is_generic_view) {
  static auto prettier = filesystem::find_executable("prettier");
  auto prefer_prettier = is_language({"js", "json", "css", "html", "markdown", "yaml"});
  if(prettier.empty() && prefer_prettier && !filesystem::file_in_path(file_path, Config::get().home_juci_path)) {
    static bool shown = false;
    if(!shown) {
      Terminal::get().print("\e[33mWarning\e[m: could not find Prettier code formatter.\n");
      Terminal::get().print("To install Prettier, run the following command in a terminal: ");
#if defined(__APPLE__)
      Terminal::get().print("brew install prettier");
#elif defined(__linux)
      if(!filesystem::find_executable("pacman").empty())
        Terminal::get().print("sudo pacman -S prettier");
      else
        Terminal::get().print("npm i -g prettier");
#else
      Terminal::get().print("npm i -g prettier");
#endif
      Terminal::get().print("\n");
    }
    shown = true;
  }

  if(!prettier.empty() && prefer_prettier) {
    if(is_generic_view) {
      goto_next_diagnostic = [this] {
        place_cursor_at_next_diagnostic();
      };
      get_buffer()->signal_changed().connect([this] {
        clear_diagnostic_tooltips();
        status_diagnostics = std::make_tuple<size_t, size_t, size_t>(0, 0, 0);
        if(update_status_diagnostics)
          update_status_diagnostics(this);
      });
    }
    format_style = [this, is_generic_view](bool continue_without_style_file, bool ignore_selection) {
      if(!continue_without_style_file) {
        auto search_path = file_path.parent_path();
        while(true) {
          static std::vector<boost::filesystem::path> files = {".prettierrc", ".prettierrc.yaml", ".prettierrc.yml", ".prettierrc.json", ".prettierrc.toml", ".prettierrc.js", "prettier.config.js"};
          boost::system::error_code ec;
          bool found = false;
          for(auto &file : files) {
            if(boost::filesystem::exists(search_path / file, ec)) {
              found = true;
              break;
            }
          }
          if(found)
            break;
          auto package_json = search_path / "package.json";
          if(boost::filesystem::exists(package_json, ec)) {
            try {
              if(JSON(package_json).child_optional("prettier"))
                break;
            }
            catch(...) {
            }
          }

          if(search_path == search_path.root_directory())
            return;
          search_path = search_path.parent_path();
        }
      }

      size_t num_warnings = 0, num_errors = 0, num_fix_its = 0;
      if(is_generic_view)
        clear_diagnostic_tooltips();

      static auto get_prettier_library = [] {
        std::string library;
        TinyProcessLib::Process process(
            "npm root -g", "",
            [&library](const char *buffer, size_t length) {
              library += std::string(buffer, length);
            },
            [](const char *, size_t) {});
        if(process.get_exit_status() == 0) {
          while(!library.empty() && (library.back() == '\n' || library.back() == '\r'))
            library.pop_back();
          library += "/prettier";
          boost::system::error_code ec;
          if(boost::filesystem::is_directory(library, ec))
            return library;
          else {
            auto parent_path = prettier.parent_path();
            if(parent_path.filename() == "bin") {
              auto path = parent_path.parent_path() / "lib" / "prettier";
              if(boost::filesystem::is_directory(path, ec))
                return path.string();
            }
            // Try find prettier library installed with homebrew on MacOS
            boost::filesystem::path path = "/usr/local/opt/prettier/libexec/lib/node_modules/prettier";
            if(boost::filesystem::is_directory(path, ec))
              return path.string();
            path = "/opt/homebrew/opt/prettier/libexec/lib/node_modules/prettier";
            if(boost::filesystem::is_directory(path, ec))
              return path.string();
          }
        }
        return std::string();
      };
      static auto prettier_library = get_prettier_library();

      if(!prettier_library.empty()) {
        struct Error {
          std::string message;
          int line = -1, index = -1;
        };
        struct Result {
          std::string text;
          int cursor_offset;
        };
        static Mutex mutex;
        static boost::optional<Result> result GUARDED_BY(mutex);
        static boost::optional<Error> error GUARDED_BY(mutex);
        {
          LockGuard lock(mutex);
          result = {};
          error = {};
        }
        static std::stringstream stdout_buffer;
        static int curly_count = 0;
        static bool key_or_value = false;
        if(prettier_background_process) {
          int exit_status;
          if(prettier_background_process->try_get_exit_status(exit_status))
            prettier_background_process = {};
        }
        if(!prettier_background_process) {
          stdout_buffer = std::stringstream();
          curly_count = 0;
          key_or_value = false;
          prettier_background_process = std::make_unique<TinyProcessLib::Process>(
              "node -e \"const repl = require('repl');repl.start({prompt: '', ignoreUndefined: true, preview: false});\"",
              "",
              [](const char *bytes, size_t n) {
                for(size_t i = 0; i < n; ++i) {
                  if(!key_or_value) {
                    if(bytes[i] == '{')
                      ++curly_count;
                    else if(bytes[i] == '}')
                      --curly_count;
                    else if(bytes[i] == '"')
                      key_or_value = true;
                  }
                  else {
                    if(bytes[i] == '\\')
                      ++i;
                    else if(bytes[i] == '"')
                      key_or_value = false;
                  }
                }
                stdout_buffer.write(bytes, n);
                if(curly_count == 0) {
                  try {
                    JSON json(stdout_buffer);
                    LockGuard lock(mutex);
                    result = Result{json.string("formatted"), static_cast<int>(json.integer_or("cursorOffset", -1))};
                  }
                  catch(const std::exception &e) {
                    LockGuard lock(mutex);
                    error = Error{std::string(e.what()) + "\nOutput from prettier: " + stdout_buffer.str()};
                  }
                  stdout_buffer = std::stringstream();
                  key_or_value = false;
                }
              },
              [](const char *bytes, size_t n) {
                size_t i = 0;
                for(; i < n; ++i) {
                  if(bytes[i] == '\n')
                    break;
                }
                std::string first_line(bytes, i);
                std::string message;
                int line = -1, line_index = -1;

                if(starts_with(first_line, "ConfigError: "))
                  message = std::string(bytes + 13, n - 13);
                else if(starts_with(first_line, "ParseError: ")) {
                  const static std::regex regex(R"(^(.*) \(([0-9]*):([0-9]*)\)$)", std::regex::optimize);
                  std::smatch sm;
                  first_line.erase(0, 12);
                  if(std::regex_match(first_line, sm, regex)) {
                    message = sm[1].str();
                    try {
                      line = std::stoi(sm[2].str());
                      line_index = std::stoi(sm[3].str());
                    }
                    catch(...) {
                      line = -1;
                      line_index = -1;
                    }
                  }
                  else
                    message = std::string(bytes + 12, n - 12);
                }
                else
                  message = std::string(bytes, n);

                LockGuard lock(mutex);
                error = Error{std::move(message), line, line_index};
              },
              true, TinyProcessLib::Config{1048576});

          prettier_background_process->write("const prettier = require(\"" + escape(prettier_library, {'"'}) + "\");\n");
        }

        std::string options = "filepath: \"" + escape(file_path.string(), {'"'}) + "\"";
        if(!ignore_selection && get_buffer()->get_has_selection()) { // Cannot be used together with cursorOffset
          Gtk::TextIter start, end;
          get_buffer()->get_selection_bounds(start, end);
          options += ", rangeStart: " + std::to_string(start.get_offset()) + ", rangeEnd: " + std::to_string(end.get_offset());
        }
        else
          options += ", cursorOffset: " + std::to_string(get_buffer()->get_insert()->get_iter().get_offset());
        prettier_background_process->write("{prettier.clearConfigCache();let _ = prettier.resolveConfig(\"" + escape(file_path.string(), {'"'}) + "\").then(options => {try{let _ = process.stdout.write(JSON.stringify(prettier.formatWithCursor(Buffer.from('");
        prettier_background_process->write(to_hex_string(get_buffer()->get_text().raw()));
        prettier_background_process->write("', 'hex').toString(), {...options, " + options + "})));}catch(error){let _ = process.stderr.write('ParseError: ' + error.message);}}).catch(error => {let _ = process.stderr.write('ConfigError: ' + error.message);});}\n");

        int exit_status = -1;
        while(true) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          if(prettier_background_process->try_get_exit_status(exit_status))
            break;
          LockGuard lock(mutex);
          if(result || error)
            break;
        }
        {
          LockGuard lock(mutex);
          if(result) {
            replace_text(result->text);
            if(result->cursor_offset >= 0 && result->cursor_offset < get_buffer()->size()) {
              get_buffer()->place_cursor(get_buffer()->get_iter_at_offset(result->cursor_offset));
              hide_tooltips();
            }
          }
          else if(error) {
            if(error->line != -1 && error->index != -1) {
              if(is_generic_view) {
                auto start = get_iter_at_line_offset(error->line - 1, error->index - 1);
                ++num_errors;
                while(start.ends_line() && start.backward_char()) {
                }
                auto end = start;
                end.forward_char();
                if(start == end)
                  start.backward_char();

                add_diagnostic_tooltip(start, end, true, [error_message = std::move(error->message)](Tooltip &tooltip) {
                  tooltip.insert_with_links_tagged(error_message);
                });
              }
            }
            else
              Terminal::get().print("\e[31mError (prettier)\e[m: " + error->message + '\n', true);
          }
          else if(exit_status >= 0)
            Terminal::get().print("\e[31mError (prettier)\e[m: process exited with exit status " + std::to_string(exit_status) + '\n', true);
        }
      }
      else {
        auto command = prettier.string();
        command += " --stdin-filepath " + filesystem::escape_argument(this->file_path.string());

        if(!ignore_selection && get_buffer()->get_has_selection()) { // Cannot be used together with --cursor-offset
          Gtk::TextIter start, end;
          get_buffer()->get_selection_bounds(start, end);
          command += " --range-start " + std::to_string(start.get_offset());
          command += " --range-end " + std::to_string(end.get_offset());
        }
        else
          command += " --cursor-offset " + std::to_string(get_buffer()->get_insert()->get_iter().get_offset());

        std::stringstream stdin_stream(get_buffer()->get_text().raw()), stdout_stream, stderr_stream;
        auto exit_status = Terminal::get().process(stdin_stream, stdout_stream, command, this->file_path.parent_path(), &stderr_stream);
        if(exit_status == 0) {
          replace_text(stdout_stream.str());
          std::string line;
          std::getline(stderr_stream, line);
          if(!line.empty() && line != "NaN") {
            try {
              auto offset = std::stoi(line);
              if(offset < get_buffer()->size()) {
                get_buffer()->place_cursor(get_buffer()->get_iter_at_offset(offset));
                hide_tooltips();
              }
            }
            catch(...) {
            }
          }
        }
        else {
          const static std::regex regex(R"(^\[.*error.*\] [^:]*: (.*) \(([0-9]*):([0-9]*)\)$)", std::regex::optimize);
          std::string line;
          std::getline(stderr_stream, line);
          std::smatch sm;
          if(std::regex_match(line, sm, regex)) {
            if(is_generic_view) {
              try {
                auto start = get_iter_at_line_offset(std::stoi(sm[2].str()) - 1, std::stoi(sm[3].str()) - 1);
                ++num_errors;
                while(start.ends_line() && start.backward_char()) {
                }
                auto end = start;
                end.forward_char();
                if(start == end)
                  start.backward_char();

                add_diagnostic_tooltip(start, end, true, [error_message = sm[1].str()](Tooltip &tooltip) {
                  tooltip.insert_with_links_tagged(error_message);
                });
              }
              catch(...) {
              }
            }
          }
          else
            Terminal::get().print("\e[31mError (prettier)\e[m: " + stderr_stream.str(), true);
        }
      }

      if(is_generic_view) {
        status_diagnostics = std::make_tuple(num_warnings, num_errors, num_fix_its);
        if(update_status_diagnostics)
          update_status_diagnostics(this);
      }
    };
  }
  else if(is_bracket_language) {
    format_style = [this](bool continue_without_style_file, bool ignore_selection) {
      static auto clang_format_command = filesystem::get_executable("clang-format").string();

      auto command = clang_format_command + " -output-replacements-xml -assume-filename=" + filesystem::escape_argument(this->file_path.string());

      if(!ignore_selection && get_buffer()->get_has_selection()) {
        Gtk::TextIter start, end;
        get_buffer()->get_selection_bounds(start, end);
        command += " -lines=" + std::to_string(start.get_line() + 1) + ':' + std::to_string(end.get_line() + 1);
      }

      bool use_style_file = false;
      auto style_file_search_path = this->file_path.parent_path();
      boost::system::error_code ec;
      while(true) {
        if(boost::filesystem::exists(style_file_search_path / ".clang-format", ec) || boost::filesystem::exists(style_file_search_path / "_clang-format", ec)) {
          use_style_file = true;
          break;
        }
        if(style_file_search_path == style_file_search_path.root_directory())
          break;
        style_file_search_path = style_file_search_path.parent_path();
      }

      if(use_style_file)
        command += " -style=file";
      else {
        if(!continue_without_style_file)
          return;
        unsigned indent_width;
        std::string tab_style;
        if(tab_char == '\t') {
          indent_width = tab_size * 8;
          tab_style = "UseTab: Always";
        }
        else {
          indent_width = tab_size;
          tab_style = "UseTab: Never";
        }
        command += " -style=\"{IndentWidth: " + std::to_string(indent_width);
        command += ", " + tab_style;
        command += ", " + std::string("AccessModifierOffset: -") + std::to_string(indent_width);
        if(Config::get().source.clang_format_style != "")
          command += ", " + Config::get().source.clang_format_style;
        command += "}\"";
      }

      std::stringstream stdin_stream(get_buffer()->get_text()), stdout_stream;

      auto exit_status = Terminal::get().process(stdin_stream, stdout_stream, command, this->file_path.parent_path());
      if(exit_status == 0) {
        // The following code is complex due to clang-format returning offsets in byte offsets instead of char offsets

        // Create bytes_in_lines cache to significantly speed up the processing of finding iterators from byte offsets
        std::vector<size_t> bytes_in_lines;
        auto line_count = get_buffer()->get_line_count();
        for(int line_nr = 0; line_nr < line_count; ++line_nr) {
          auto iter = get_buffer()->get_iter_at_line(line_nr);
          bytes_in_lines.emplace_back(iter.get_bytes_in_line());
        }

        get_buffer()->begin_user_action();
        try {
          boost::property_tree::ptree pt;
          boost::property_tree::xml_parser::read_xml(stdout_stream, pt);
          auto replacements_pt = pt.get_child("replacements", boost::property_tree::ptree());
          for(auto it = replacements_pt.rbegin(); it != replacements_pt.rend(); ++it) {
            if(it->first == "replacement") {
              auto offset = it->second.get<size_t>("<xmlattr>.offset");
              auto length = it->second.get<size_t>("<xmlattr>.length");
              auto replacement_str = it->second.get<std::string>("");

              size_t bytes = 0;
              for(size_t c = 0; c < bytes_in_lines.size(); ++c) {
                auto previous_bytes = bytes;
                bytes += bytes_in_lines[c];
                if(offset < bytes || (c == bytes_in_lines.size() - 1 && offset == bytes)) {
                  std::pair<size_t, size_t> line_index(c, offset - previous_bytes);
                  auto start = get_buffer()->get_iter_at_line_index(line_index.first, line_index.second);

                  // Use left gravity insert to avoid moving cursor from end of line
                  bool left_gravity_insert = false;
                  if(get_buffer()->get_insert()->get_iter() == start) {
                    auto iter = start;
                    do {
                      if(*iter != ' ' && *iter != '\t') {
                        left_gravity_insert = iter.ends_line();
                        break;
                      }
                    } while(iter.forward_char());
                  }

                  if(length > 0) {
                    auto offset_end = offset + length;
                    size_t bytes = 0;
                    for(size_t c = 0; c < bytes_in_lines.size(); ++c) {
                      auto previous_bytes = bytes;
                      bytes += bytes_in_lines[c];
                      if(offset_end < bytes || (c == bytes_in_lines.size() - 1 && offset_end == bytes)) {
                        auto end = get_buffer()->get_iter_at_line_index(c, offset_end - previous_bytes);
                        get_buffer()->erase(start, end);
                        start = get_buffer()->get_iter_at_line_index(line_index.first, line_index.second);
                        break;
                      }
                    }
                  }
                  if(left_gravity_insert) {
                    Mark mark(start);
                    get_buffer()->insert(start, replacement_str);
                    get_buffer()->place_cursor(mark->get_iter());
                  }
                  else
                    get_buffer()->insert(start, replacement_str);
                  break;
                }
              }
            }
          }
        }
        catch(const std::exception &e) {
          Terminal::get().print(std::string("\e[31mError\e[m: error parsing clang-format output: ") + e.what() + '\n', true);
        }
        get_buffer()->end_user_action();
      }
    };
  }
  else if(language_id == "python") {
    static auto yapf = filesystem::find_executable("yapf");
    if(!yapf.empty()) {
      format_style = [this](bool continue_without_style_file, bool ignore_selection) {
        std::string command = "yapf";

        if(!ignore_selection && get_buffer()->get_has_selection()) {
          Gtk::TextIter start, end;
          get_buffer()->get_selection_bounds(start, end);
          command += " -l " + std::to_string(start.get_line() + 1) + '-' + std::to_string(end.get_line() + 1);
        }

        if(!continue_without_style_file) {
          auto search_path = file_path.parent_path();
          while(true) {
            boost::system::error_code ec;
            if(boost::filesystem::exists(search_path / ".python-format", ec) || boost::filesystem::exists(search_path / ".style.yapf", ec))
              break;
            if(search_path == search_path.root_directory())
              return;
            search_path = search_path.parent_path();
          }
        }

        std::stringstream stdin_stream(get_buffer()->get_text()), stdout_stream;

        auto exit_status = Terminal::get().process(stdin_stream, stdout_stream, command, this->file_path.parent_path());
        if(exit_status == 0)
          replace_text(stdout_stream.str());
      };
    }
  }
  else if(language_id == "latex") {
    static auto yapf = filesystem::find_executable("latexindent");
    if(!yapf.empty()) {
      format_style = [this](bool continue_without_style_file, bool ignore_selection) {
        std::string command = "latexindent -m";

        if(!ignore_selection && get_buffer()->get_has_selection()) {
          Gtk::TextIter start, end;
          get_buffer()->get_selection_bounds(start, end);
          command += " -n " + std::to_string(start.get_line() + 1) + '-' + std::to_string(end.get_line() + 1);
        }

        auto search_path = file_path.parent_path();
        while(true) {
          static std::vector<boost::filesystem::path> files = {"localSettings.yaml", ".localSettings.yaml", "indentconfig.yaml", ".indentconfig.yaml"};
          boost::system::error_code ec;
          bool found = false;
          for(auto &file : files) {
            if(boost::filesystem::exists(search_path / file, ec)) {
              command += " -l=" + file.string();
              found = true;
              break;
            }
          }
          if(found)
            break;
          if(search_path == search_path.root_directory()) {
            if(!continue_without_style_file)
              return;
            else
              break;
          }
          search_path = search_path.parent_path();
        }

        command += " -";

        std::stringstream stdin_stream(get_buffer()->get_text()), stdout_stream;

        auto exit_status = Terminal::get().process(stdin_stream, stdout_stream, command, this->file_path.parent_path());
        if(exit_status == 0)
          replace_text(stdout_stream.str());
      };
    }
  }
}

Source::View::~View() {
  delayed_tooltips_connection.disconnect();
  delayed_tag_similar_symbols_connection.disconnect();
  delayed_tag_clickable_connection.disconnect();

  non_deleted_views.erase(this);
  views.erase(this);
}

void Source::View::hide_tooltips() {
  delayed_tooltips_connection.disconnect();
  type_tooltips.hide();
  diagnostic_tooltips.hide();
}

void Source::View::hide_dialogs() {
  SpellCheckView::hide_dialogs();
  if(SelectionDialog::get())
    SelectionDialog::get()->hide();
  if(CompletionDialog::get())
    CompletionDialog::get()->hide();
}

void Source::View::scroll_to_cursor_delayed(bool center, bool show_tooltips) {
  if(!show_tooltips)
    hide_tooltips();
  Glib::signal_idle().connect([this, center] {
    if(views.find(this) != views.end()) {
      if(center)
        scroll_to(get_buffer()->get_insert(), 0.0, 1.0, 0.5);
      else
        scroll_to(get_buffer()->get_insert());
    }
    return false;
  });
}

void Source::View::extend_selection() {
  // Have tried to generalize this function as much as possible due to the complexity of this task,
  // but some further workarounds for edge cases might be needed

  // It is impossible to identify <> used for templates by syntax alone, but
  // this function works in most cases.
  auto is_template_arguments = [this](Gtk::TextIter start, Gtk::TextIter end) {
    if(*start != '<' || *end != '>' || start.get_line() != end.get_line())
      return false;
    auto prev = start;
    if(!prev.backward_char())
      return false;
    if(!is_token_char(*prev))
      return false;
    auto next = end;
    next.forward_char();
    if(*next != '(' && *next != ' ')
      return false;
    return true;
  };

  // Extends expression from 'here' in for instance: test->here(...), test.test(here) or here.test(test)
  auto extend_expression = [&](Gtk::TextIter &start, Gtk::TextIter &end) {
    auto start_stored = start;
    auto end_stored = end;
    bool extend_token_forward = true, extend_token_backward = true;

    auto iter = start;
    auto prev = iter;
    if(prev.backward_char() && ((*prev == '(' && *end == ')') || (*prev == '[' && *end == ']') || (*prev == '<' && *end == '>') || (*prev == '{' && *end == '}'))) {
      if(*prev == '<' && !is_template_arguments(prev, end))
        return false;
      iter = start = prev;
      end.forward_char();
      extend_token_forward = false;
    }
    else if(is_token_char(*iter)) {
      auto token = get_token_iters(iter);
      if(start != token.first || end != token.second)
        return false;
      extend_token_forward = false;
      extend_token_backward = false;
    }
    else
      return false;

    // Extend expression forward passed for instance member function
    {
      auto iter = end;

      bool extend_token = extend_token_forward;
      while(forward_to_code(iter)) {
        if(extend_token && is_token_char(*iter)) {
          auto token = get_token_iters(iter);
          iter = end = token.second;
          extend_token = false;
          continue;
        }

        if(!extend_token && *iter == '(' && iter.forward_char() && find_close_symbol_forward(iter, iter, '(', ')')) {
          iter.forward_char();
          end = iter;
          extend_token = false;
          continue;
        }
        if(!extend_token && *iter == '[' && iter.forward_char() && find_close_symbol_forward(iter, iter, '[', ']')) {
          iter.forward_char();
          end = iter;
          extend_token = false;
          continue;
        }
        auto prev = iter;
        if(!extend_token && *iter == '<' && iter.forward_char() && find_close_symbol_forward(iter, iter, '<', '>') && is_template_arguments(prev, iter)) { // Only extend for instance std::max<int>(1, 2)
          iter.forward_char();
          end = iter;
          extend_token = false;
          continue;
        }

        if(!extend_token && *iter == '.') {
          iter.forward_char();
          extend_token = true;
          continue;
        }
        auto next = iter;
        if(!extend_token && next.forward_char() && ((*iter == ':' && *next == ':') || (*iter == '-' && *next == '>'))) {
          iter = next;
          iter.forward_char();
          extend_token = true;
          continue;
        }

        break;
      }

      // Extend through {}
      auto prev = iter = end;
      prev.backward_char();
      if(*prev != '}' && forward_to_code(iter) && *iter == '{' && iter.forward_char() && find_close_symbol_forward(iter, iter, '{', '}')) {
        iter.forward_char();
        end = iter;
      }
    }

    // Extend backward
    iter = start;
    bool extend_token = extend_token_backward;
    while(true) {
      if(!iter.backward_char() || !backward_to_code(iter))
        break;

      if(extend_token && is_token_char(*iter)) {
        auto token = get_token_iters(iter);
        start = iter = token.first;
        extend_token = false;
        continue;
      }

      if(extend_token && *iter == ')' && iter.backward_char() && find_open_symbol_backward(iter, iter, '(', ')')) {
        start = iter;
        extend_token = true;
        continue;
      }
      if(extend_token && *iter == ']' && iter.backward_char() && find_open_symbol_backward(iter, iter, '[', ']')) {
        start = iter;
        extend_token = true;
        continue;
      }
      auto angle_end = iter;
      if(extend_token && *iter == '>' && iter.backward_char() && find_open_symbol_backward(iter, iter, '<', '>') && is_template_arguments(iter, angle_end)) { // Only extend for instance std::max<int>(1, 2)
        start = iter;
        continue;
      }

      if(*iter == '.') {
        extend_token = true;
        continue;
      }
      if(angle_end.backward_char() && ((*angle_end == ':' && *iter == ':') || (*angle_end == '-' && *iter == '>'))) {
        iter = angle_end;
        extend_token = true;
        continue;
      }

      break;
    }

    if(start != start_stored || end != end_stored)
      return true;
    return false;
  };

  Gtk::TextIter start, end;
  get_buffer()->get_selection_bounds(start, end);

  // If entire buffer is selected, do nothing
  if(start == get_buffer()->begin() && end == get_buffer()->end())
    return;

  auto start_stored = start;
  auto end_stored = end;

  previous_extended_selections.emplace_back(start, end);
  keep_previous_extended_selections = true;
  ScopeGuard guard{[this] {
    keep_previous_extended_selections = false;
  }};

  // Select token
  if(!get_buffer()->get_has_selection()) {
    auto iter = get_buffer()->get_insert()->get_iter();
    if(is_token_char(*iter)) {
      auto token = get_token_iters(iter);
      get_buffer()->select_range(token.first, token.second);
      return;
    }
  }
  else { // Complete token selection
    Gtk::TextIter start, end;
    get_buffer()->get_selection_bounds(start, end);
    auto start_token = get_token_iters(start);
    auto end_token = get_token_iters(end);
    if(start_token.first < start || end_token.second > end) {
      get_buffer()->select_range(start_token.first, end_token.second);
      return;
    }
  }

  // Select string or comment block
  auto before_start = start;
  if(!is_code_iter(start) && !(before_start.backward_char() && is_code_iter(before_start) && is_code_iter(end))) {
    bool no_code_iter = true;
    for(auto iter = start; iter.forward_char() && iter < end;) {
      if(is_code_iter(iter)) {
        no_code_iter = false;
        break;
      }
    }
    if(no_code_iter) {
      if(backward_to_code(start)) {
        while(start.forward_char() && (*start == ' ' || *start == '\t' || start.ends_line())) {
        }
      }
      if(forward_to_code(end)) {
        while(end.backward_char() && (*end == ' ' || *end == '\t' || end.ends_line())) {
        }
        end.forward_char();
      }
      if(start != start_stored || end != end_stored) {
        get_buffer()->select_range(start, end);
        return;
      }
      start = start_stored;
      end = end_stored;
    }
  }

  // Select expression from token
  if(get_buffer()->get_has_selection() && is_token_char(*start) && start.get_line() == end.get_line() && extend_expression(start, end)) {
    get_buffer()->select_range(start, end);
    return;
  }

  before_start = start;
  auto before_end = end;
  bool ignore_comma = false;
  auto start_sentence_iter = get_buffer()->end();
  auto end_sentence_iter = get_buffer()->end();
  if(is_code_iter(start) && is_code_iter(end) && before_start.backward_char() && before_end.backward_char()) {
    if((*before_start == '(' && *end == ')') ||
       (*before_start == '[' && *end == ']') ||
       (*before_start == '<' && *end == '>') ||
       (*before_start == '{' && *end == '}')) {
      // Select expression from selected brackets
      if(!(*before_start == '<' && *end == '>' && is_js) &&
         extend_expression(start, end)) {
        get_buffer()->select_range(start, end);
        return;
      }
      start = before_start;
      end.forward_char();
    }
    else if((*before_start == ',' && *end == ',') ||
            (*before_start == ',' && *end == ')') ||
            (*before_start == ',' && *end == ']') ||
            (*before_start == ',' && *end == '>') ||
            (*before_start == ',' && *end == '}') ||
            (*before_start == '(' && *end == ',') ||
            (*before_start == '[' && *end == ',') ||
            (*before_start == '<' && *end == ',') ||
            (*before_start == '{' && *end == ','))
      ignore_comma = true;
    else if(start != end && (*before_end == ';' || *before_end == '}')) {
      auto iter = end;
      if(*before_end == '}' && forward_to_code(iter) && *iter == ';')
        end_sentence_iter = iter;
      else
        end_sentence_iter = before_end;
    }
  }

  int para_count = 0;
  int square_count = 0;
  int angle_count = 0;
  int curly_count = 0;
  auto start_comma_iter = get_buffer()->end();
  auto start_angle_iter = get_buffer()->end();
  auto start_angle_reversed_iter = get_buffer()->end();
  while(start.backward_char()) {
    if(*start == '(' && is_code_iter(start))
      para_count++;
    else if(*start == ')' && is_code_iter(start))
      para_count--;
    else if(*start == '[' && is_code_iter(start))
      square_count++;
    else if(*start == ']' && is_code_iter(start))
      square_count--;
    else if(*start == '<' && is_code_iter(start)) {
      if(!start_angle_iter && para_count == 0 && square_count == 0 && angle_count == 0 && curly_count == 0)
        start_angle_iter = start;
      angle_count++;
    }
    else if(*start == '>' && is_code_iter(start)) {
      auto prev = start;
      if(!(prev.backward_char() && (*prev == '=' || *prev == '-'))) {
        if(!start_angle_reversed_iter && para_count == 0 && square_count == 0 && angle_count == 0 && curly_count == 0)
          start_angle_reversed_iter = start;
        angle_count--;
      }
    }
    else if(*start == '{' && is_code_iter(start)) {
      if(!start_sentence_iter &&
         para_count == 0 && square_count == 0 && curly_count == 0) {
        start_sentence_iter = start;
      }
      curly_count++;
    }
    else if(*start == '}' && is_code_iter(start)) {
      if(!start_sentence_iter &&
         para_count == 0 && square_count == 0 && curly_count == 0) {
        auto next = start;
        if(next.forward_char() && forward_to_code(next) && *next != ';')
          start_sentence_iter = start;
      }
      curly_count--;
    }
    else if(!ignore_comma && !start_comma_iter &&
            para_count == 0 && square_count == 0 && curly_count == 0 &&
            *start == ',' && is_code_iter(start))
      start_comma_iter = start;
    else if(!start_sentence_iter &&
            para_count == 0 && square_count == 0 && curly_count == 0 &&
            *start == ';' && is_code_iter(start))
      start_sentence_iter = start;
    if(*start == ';' && is_code_iter(start)) {
      ignore_comma = true;
      start_comma_iter = get_buffer()->end();
    }
    if(para_count > 0 || square_count > 0 || curly_count > 0)
      break;
  }

  para_count = 0;
  square_count = 0;
  angle_count = 0;
  curly_count = 0;
  auto end_comma_iter = get_buffer()->end();
  auto end_angle_iter = get_buffer()->end();
  auto end_angle_reversed_iter = get_buffer()->end();
  do {
    if(*end == '(' && is_code_iter(end))
      para_count++;
    else if(*end == ')' && is_code_iter(end))
      para_count--;
    else if(*end == '[' && is_code_iter(end))
      square_count++;
    else if(*end == ']' && is_code_iter(end))
      square_count--;
    else if(*end == '<' && is_code_iter(end)) {
      if(!end_angle_reversed_iter && para_count == 0 && square_count == 0 && angle_count == 0 && curly_count == 0)
        end_angle_reversed_iter = end;
      angle_count++;
    }
    else if(*end == '>' && is_code_iter(end)) {
      auto prev = end;
      if(!(prev.backward_char() && (*prev == '=' || *prev == '-'))) {
        if(!end_angle_iter && para_count == 0 && square_count == 0 && angle_count == 0 && curly_count == 0)
          end_angle_iter = end;
        angle_count--;
      }
    }
    else if(*end == '{' && is_code_iter(end))
      curly_count++;
    else if(*end == '}' && is_code_iter(end)) {
      curly_count--;
      if(!end_sentence_iter &&
         para_count == 0 && square_count == 0 && curly_count == 0) {
        auto next = end_sentence_iter = end;
        if(next.forward_char() && forward_to_code(next) && *next == ';')
          end_sentence_iter = next;
        else if(is_js && *next == '>')
          end_sentence_iter = get_buffer()->end();
      }
    }
    else if(!ignore_comma && !end_comma_iter &&
            para_count == 0 && square_count == 0 && curly_count == 0 &&
            *end == ',' && is_code_iter(end))
      end_comma_iter = end;
    else if(!end_sentence_iter &&
            para_count == 0 && square_count == 0 && curly_count == 0 &&
            *end == ';' && is_code_iter(end))
      end_sentence_iter = end;
    if(*end == ';' && is_code_iter(end)) {
      ignore_comma = true;
      start_comma_iter = get_buffer()->end();
      end_comma_iter = get_buffer()->end();
    }
    if(para_count < 0 || square_count < 0 || curly_count < 0)
      break;
  } while(end.forward_char());

  // Extend HTML/JSX
  if(is_js) {
    static std::vector<std::string> void_elements = {"area", "base", "br", "col", "command", "embed", "hr", "img", "input", "keygen", "link", "meta", "param", "source", "track", "wbr"};
    auto get_element = [this](Gtk::TextIter iter) {
      auto start = iter;
      auto end = iter;
      while(iter.backward_char() && (is_token_char(*iter) || *iter == '.'))
        start = iter;
      while((is_token_char(*end) || *end == '.') && end.forward_char()) {
      }
      return get_buffer()->get_text(start, end);
    };

    Gtk::TextIter start_backward_search = get_buffer()->end(), start_forward_search = get_buffer()->end();
    auto start_stored_prev = start_stored;
    if(start_angle_iter && end_angle_iter) { // If inside angle brackets in for instance <selection here>
      auto next = start_angle_iter;
      next.forward_char();
      auto prev = end_angle_iter;
      prev.backward_char();
      auto element = get_element(next);
      if(*next == '/') {
        start_backward_search = start_forward_search = start_angle_iter;
        start_backward_search.backward_char();
      }
      else if(*prev == '/' || std::any_of(void_elements.begin(), void_elements.end(), [&element](const std::string &e) { return e == element; })) {
        end_angle_iter.forward_char();
        get_buffer()->select_range(start_angle_iter, end_angle_iter);
        return;
      }
      else {
        start_backward_search = start_forward_search = end_angle_iter;
        start_forward_search.forward_char();
      }
    }
    else if(start_stored_prev.backward_char() && *start_stored_prev == '<' && *end_stored == '>') { // Matches for instance <div>, where div is selected
      auto element = get_element(start_stored);
      if(std::any_of(void_elements.begin(), void_elements.end(), [&element](const std::string &e) { return e == element; })) {
        auto next = end_stored;
        next.forward_char();
        get_buffer()->select_range(start_stored_prev, next);
        return;
      }
      start_backward_search = start_forward_search = end_stored;
      start_forward_search.forward_char();
    }
    else if(start_angle_reversed_iter && end_angle_reversed_iter) { // If not inside angle brackets, for instance <>selection here</>
      start_backward_search = start_angle_reversed_iter;
      start_forward_search = end_angle_reversed_iter;
    }

    if(start_backward_search && start_forward_search) {
      Gtk::TextIter start_children, end_children;
      auto iter = start_backward_search;
      int depth = 0;
      // Search backward for opening element
      while(find_open_symbol_backward(iter, iter, '>', '<') && iter.backward_char()) { // Backward to > (end of opening element)
        start_children = iter;
        start_children.forward_chars(2);
        auto prev = iter;
        prev.backward_char();
        bool no_child_element = *iter == '/' && *prev != '<'; // Excludes </> as it is always closing element of <>
        if(find_open_symbol_backward(iter, iter, '<', '>')) { // Backward to <
          if(!no_child_element) {
            iter.forward_char();
            if(*iter == '/') {
              if(!iter.backward_chars(2))
                break;
              --depth;
              continue;
            }
            auto element = get_element(iter);
            if(std::any_of(void_elements.begin(), void_elements.end(), [&element](const std::string &e) { return e == element; })) {
              if(!iter.backward_chars(2))
                break;
              continue;
            }
            else if(depth == 0) {
              iter.backward_char();
              auto start_selection = iter;
              iter = start_forward_search;
              // Search forward for closing element
              int depth = 0;
              while(find_close_symbol_forward(iter, iter, '>', '<') && iter.forward_char()) { // Forward to < (start of closing element)
                end_children = iter;
                end_children.backward_char();
                if(*iter == '/') {
                  if(iter.forward_char() && find_close_symbol_forward(iter, iter, '<', '>') && iter.forward_char()) { // Forward to >
                    if(depth == 0) {
                      if(start_children <= start_stored && end_children >= end_stored && (start_children != start_stored || end_children != end_stored)) // Select children
                        get_buffer()->select_range(start_children, end_children);
                      else
                        get_buffer()->select_range(start_selection, iter);
                      return;
                    }
                    else
                      --depth;
                  }
                  else
                    break;
                }
                else {
                  auto element = get_element(iter);
                  if(std::any_of(void_elements.begin(), void_elements.end(), [&element](const std::string &e) { return e == element; })) {
                    if(!(find_close_symbol_forward(iter, iter, '<', '>') && iter.forward_char())) // Forward to >
                      break;
                  }
                  else if(find_close_symbol_forward(iter, iter, '<', '>') && iter.backward_char()) { // Forward to >
                    if(*iter == '/')
                      iter.forward_chars(2);
                    else {
                      ++depth;
                      iter.forward_chars(2);
                    }
                  }
                  else
                    break;
                }
              }
              break;
            }
            else if(!iter.backward_chars(2))
              break;
            ++depth;
          }
          else if(!iter.backward_char())
            break;
        }
        else
          break;
      }
    }
  }

  // Test for <> used for template arguments
  if(start_angle_iter && end_angle_iter && is_template_arguments(start_angle_iter, end_angle_iter)) {
    start_angle_iter.forward_char();
    get_buffer()->select_range(start_angle_iter, end_angle_iter);
    return;
  }

  // Test for matching brackets and try select regions within brackets separated by ','
  bool comma_used = false;
  bool select_matching_brackets = false;
  if((*start == '(' && *end == ')') ||
     (*start == '[' && *end == ']') ||
     (*start == '<' && *end == '>') ||
     (*start == '{' && *end == '}')) {
    if(start_comma_iter && start < start_comma_iter) {
      start = start_comma_iter;
      comma_used = true;
    }
    if(end_comma_iter && end > end_comma_iter) {
      end = end_comma_iter;
      comma_used = true;
    }
    select_matching_brackets = true;
  }

  // Attempt to select a sentence, for instance: int a = 2;
  if(!is_bracket_language) { // If for instance cmake, meson or python
    if(!select_matching_brackets) {
      bool select_end_block = is_language({"cmake", "meson", "julia", "xml"});

      auto get_tabs = [this](Gtk::TextIter iter) -> boost::optional<int> {
        iter = get_buffer()->get_iter_at_line(iter.get_line());
        int tabs = 0;
        while(!iter.ends_line() && (*iter == ' ' || *iter == '\t')) {
          tabs++;
          if(!iter.forward_char())
            break;
        }
        if(iter.ends_line())
          return {};
        return tabs;
      };

      start = start_stored;
      end = end_stored;

      // Select following line with code (for instance when inside comment)
      // Forward start to non-empty line
      forward_to_code(start);
      start = get_buffer()->get_iter_at_line(start.get_line());
      while(!start.is_end() && (*start == ' ' || *start == '\t') && start.forward_char()) {
      }
      // Forward end of line
      if(start > end)
        end = start;
      end = get_iter_at_line_end(end.get_line());
      while(end.backward_char() && (*end == ' ' || *end == '\t' || end.ends_line())) {
      }
      end.forward_char();
      if(end == end_stored) // Cancel line selection if the line is already selected
        start = start_stored;

      if(start != start_stored || end != end_stored) {
        get_buffer()->select_range(start, end);
        return;
      }

      // Select current line
      // Backward to line start
      start = get_buffer()->get_iter_at_line(start.get_line());
      auto start_tabs = get_tabs(start);
      while((*start == ' ' || *start == '\t' || start.ends_line()) && start.forward_char()) {
      }
      // Forward to line end
      end = get_iter_at_line_end(end.get_line());
      bool include_children = false;
      if(start_tabs) {
        while(end.forward_char()) {
          auto tabs = get_tabs(end);
          if(tabs) {
            if(tabs > start_tabs)
              include_children = true;
            else if(tabs == start_tabs) {
              if(include_children && select_end_block)
                end = get_iter_at_line_end(end.get_line());
              break;
            }
            else
              break;
          }
          end = get_iter_at_line_end(end.get_line());
        }
      }
      // Backward end to non-empty line
      while(end.backward_char() && (*end == ' ' || *end == '\t' || end.ends_line())) {
      }
      end.forward_char();

      if(start != start_stored || end != end_stored) {
        // Forward to closing symbol if open symbol is found between start and end
        auto iter = start;
        para_count = 0;
        square_count = 0;
        curly_count = 0;
        do {
          if(*iter == '(' && is_code_iter(iter))
            para_count++;
          else if(*iter == ')' && is_code_iter(iter))
            para_count--;
          else if(*iter == '[' && is_code_iter(iter))
            square_count++;
          else if(*iter == ']' && is_code_iter(iter))
            square_count--;
          else if(*iter == '{' && is_code_iter(iter))
            curly_count++;
          else if(*iter == '}' && is_code_iter(iter))
            curly_count--;
          if(para_count < 0 || square_count < 0 || curly_count < 0)
            break;
        } while(iter.forward_char() && iter < end);
        if(iter == end && (para_count > 0 || square_count > 0 || curly_count > 0)) {
          do {
            if(*iter == '(' && is_code_iter(iter))
              para_count++;
            else if(*iter == ')' && is_code_iter(iter))
              para_count--;
            else if(*iter == '[' && is_code_iter(iter))
              square_count++;
            else if(*iter == ']' && is_code_iter(iter))
              square_count--;
            else if(*iter == '{' && is_code_iter(iter))
              curly_count++;
            else if(*iter == '}' && is_code_iter(iter))
              curly_count--;
            if(para_count == 0 && square_count == 0 && curly_count == 0)
              break;
          } while(iter.forward_char());
          if(iter) {
            end = iter;
            end.forward_char();
          }
        }

        get_buffer()->select_range(start, end);
        return;
      }

      // Select block that cursor is within
      // Backward start block start
      if(start_tabs > 0) {
        start = get_buffer()->get_iter_at_line(start.get_line());
        while(start.backward_char()) {
          auto tabs = get_tabs(start);
          if(tabs && tabs < start_tabs)
            break;
          start = get_buffer()->get_iter_at_line(start.get_line());
        }
        while((*start == ' ' || *start == '\t' || start.ends_line()) && start.forward_char()) {
        }
        // Forward to block end
        end = get_iter_at_line_end(end.get_line());
        while(end.forward_char()) {
          auto tabs = get_tabs(end);
          if(tabs && tabs < start_tabs)
            break;
          end = get_iter_at_line_end(end.get_line());
        }
        // Backward end to non-empty line
        while(end.backward_char() && (*end == ' ' || *end == '\t' || end.ends_line())) {
        }
        end.forward_char();

        if(start != start_stored || end != end_stored) {
          get_buffer()->select_range(start, end);
          return;
        }

        // Select expression surrounding block
        // Backward to expression starting block
        if(start.backward_char()) {
          backward_to_code(start);
          if(start_tabs > get_tabs(start)) {
            start = get_buffer()->get_iter_at_line(start.get_line());
            while((*start == ' ' || *start == '\t' || start.ends_line()) && start.forward_char()) {
            }
          }
          else
            start = start_stored;
        }
        // Forward to expression ending block
        if(select_end_block) {
          forward_to_code(end);
          if(start_tabs > get_tabs(end)) {
            end = get_iter_at_line_end(end.get_line());
            while(end.backward_char() && (*end == ' ' || *end == '\t' || end.ends_line())) {
            }
            end.forward_char();
          }
          else
            end = end_stored;
        }

        if(start != start_stored || end != end_stored) {
          get_buffer()->select_range(start, end);
          return;
        }
      }

      // Select no_spellcheck_tag block if markdown
      if(language_id == "markdown" && no_spellcheck_tag && start.has_tag(no_spellcheck_tag) && end.has_tag(no_spellcheck_tag)) {
        if(!start.starts_tag(no_spellcheck_tag))
          start.backward_to_tag_toggle(no_spellcheck_tag);
        if(!end.ends_tag(no_spellcheck_tag))
          end.forward_to_tag_toggle(no_spellcheck_tag);
        auto prev = start;
        while(*start == '`' && start.forward_char()) {
        }
        if(start.get_offset() - prev.get_offset() > 1) {
          start.forward_to_line_end();
          start.forward_char();
        }
        while(end.backward_char() && *end == '`') {
        }
        if(!end.ends_line())
          end.forward_char();

        if(start != start_stored || end != end_stored) {
          get_buffer()->select_range(start, end);
          return;
        }
      }

      get_buffer()->select_range(get_buffer()->begin(), get_buffer()->end());
      return;
    }
  }
  else if(!comma_used && end_sentence_iter && end > end_sentence_iter) {
    if(!start_sentence_iter)
      start_sentence_iter = start;
    else
      start_sentence_iter.forward_char();

    // Forward to code iter (move passed macros)
    while(forward_to_code(start_sentence_iter) && *start_sentence_iter == '#' && start_sentence_iter.forward_to_line_end()) {
      auto prev = start_sentence_iter;
      if(prev.backward_char() && *prev == '\\' && start_sentence_iter.forward_char()) {
        while(start_sentence_iter.forward_to_line_end()) {
          prev = start_sentence_iter;
          if(prev.backward_char() && *prev == '\\' && start_sentence_iter.forward_char())
            continue;
          break;
        }
      }
    }

    // Select sentence
    end_sentence_iter.forward_char();
    if((start_sentence_iter != start_stored || end_sentence_iter != end_stored) &&
       ((*start == '{' && *end == '}') || (start.is_start() && end.is_end()))) {
      get_buffer()->select_range(start_sentence_iter, end_sentence_iter);
      return;
    }
  }

  if(select_matching_brackets)
    start.forward_char();

  if(start != start_stored || end != end_stored) {
    get_buffer()->select_range(start, end);
    return;
  }

  // In case of no change due to inbalanced brackets
  previous_extended_selections.pop_back();
  if(!start.backward_char() && !end.forward_char())
    return;
  get_buffer()->select_range(start, end);
  extend_selection();
}

void Source::View::shrink_selection() {
  if(previous_extended_selections.empty()) {
    Info::get().print("No previous extended selections found");
    return;
  }
  auto selection = previous_extended_selections.back();
  keep_previous_extended_selections = true;
  get_buffer()->select_range(selection.first, selection.second);
  hide_tooltips();
  keep_previous_extended_selections = false;
  previous_extended_selections.pop_back();
}

void Source::View::show_or_hide() {
  Gtk::TextIter start, end;
  get_buffer()->get_selection_bounds(start, end);

  if(start == end && !(start.starts_line() && start.ends_line())) { // Select code block instead if no current selection
    start = get_buffer()->get_iter_at_line(start.get_line());
    auto tabs_end = get_tabs_end_iter(start);
    auto start_tabs = tabs_end.get_line_offset() - start.get_line_offset();

    if(!end.ends_line())
      end.forward_to_line_end();

    auto last_empty = get_buffer()->end();
    auto last_tabs_end = get_buffer()->end();
    while(true) {
      if(end.ends_line()) {
        auto line_start = get_buffer()->get_iter_at_line(end.get_line());
        auto tabs_end = get_tabs_end_iter(line_start);
        if(end.starts_line() || tabs_end.ends_line()) { // Empty line
          if(!last_empty)
            last_empty = end;
        }
        else {
          auto tabs = tabs_end.get_line_offset() - line_start.get_line_offset();
          if((is_c || is_cpp) && tabs == 0 && *line_start == '#') { // C/C++ defines can be at the first line
            if(end.get_line() == start.get_line())                  // Do not try to find define blocks since these rarely are indented
              break;
          }
          else if(tabs < start_tabs) {
            end = get_buffer()->get_iter_at_line(end.get_line());
            break;
          }
          else if(tabs == start_tabs) {
            // Check for block continuation keywords
            std::string text = get_buffer()->get_text(tabs_end, end);
            if(end.get_line() != start.get_line()) {
              if(text.empty()) {
                end = get_buffer()->get_iter_at_line(end.get_line());
                break;
              }
              static std::vector<std::string> starts_with_symbols = {"}", ")", "]", ">", "</"};
              static std::vector<std::string> exact_tokens = {"else", "end", "endif", "elseif", "elif", "catch", "case", "default", "private", "public", "protected"};
              if(text == "{") { // C/C++ sometimes starts a block with a standalone {
                if(!is_token_char(*last_tabs_end)) {
                  end = get_buffer()->get_iter_at_line(end.get_line());
                  break;
                }
                else { // Check for ; at the end of last line
                  auto iter = tabs_end;
                  while(iter.backward_char() && iter > last_tabs_end && (*iter == ' ' || *iter == '\t' || iter.ends_line() || !is_code_iter(iter))) {
                  }
                  if(*iter == ';') {
                    end = get_buffer()->get_iter_at_line(end.get_line());
                    break;
                  }
                }
              }
              else if(std::none_of(starts_with_symbols.begin(), starts_with_symbols.end(), [&text](const std::string &symbol) {
                        return starts_with(text, symbol);
                      }) &&
                      std::none_of(exact_tokens.begin(), exact_tokens.end(), [this, &text](const std::string &token) {
                        return starts_with(text, token) && (text.size() <= token.size() || !is_token_char(text[token.size()]));
                      })) {
                end = get_buffer()->get_iter_at_line(end.get_line());
                break;
              }
            }
            last_tabs_end = tabs_end;
          }
          last_empty = get_buffer()->end();
        }
      }
      if(end.is_end())
        break;
      end.forward_char();
    }
    if(last_empty)
      end = get_buffer()->get_iter_at_line(last_empty.get_line());
  }
  if(start == end)
    end.forward_char(); // Select empty line

  if(!start.starts_line())
    start = get_buffer()->get_iter_at_line(start.get_line());
  if(!end.ends_line() && !end.starts_line())
    end.forward_to_line_end();

  if((start.begins_tag(hide_tag) || start.has_tag(hide_tag)) && (end.ends_tag(hide_tag) || end.has_tag(hide_tag))) {
    get_buffer()->remove_tag(hide_tag, start, end);
    return;
  }
  auto iter = start;
  if(iter.forward_to_tag_toggle(hide_tag) && iter < end) {
    get_buffer()->remove_tag(hide_tag, start, end);
    return;
  }
  get_buffer()->apply_tag(hide_tag, start, end);
}

void Source::View::add_diagnostic_tooltip(const Gtk::TextIter &start, const Gtk::TextIter &end, bool error, std::function<void(Tooltip &)> &&set_buffer) {
  diagnostic_offsets.emplace(start.get_offset());

  std::string severity_tag_name = error ? "def:error" : "def:warning";

  diagnostic_tooltips.emplace_back(this, start, end, [error, severity_tag_name, set_buffer = std::move(set_buffer)](Tooltip &tooltip) {
    tooltip.buffer->insert_with_tag(tooltip.buffer->get_insert()->get_iter(), error ? "Error" : "Warning", severity_tag_name);
    tooltip.buffer->insert(tooltip.buffer->get_insert()->get_iter(), ":\n");
    set_buffer(tooltip);
  });

  get_buffer()->apply_tag_by_name(severity_tag_name + "_underline", start, end);

  auto iter = get_buffer()->get_insert()->get_iter();
  if(iter.ends_line()) {
    auto next_iter = iter;
    if(next_iter.forward_char())
      get_buffer()->remove_tag_by_name(severity_tag_name + "_underline", iter, next_iter);
  }
}

void Source::View::clear_diagnostic_tooltips() {
  diagnostic_offsets.clear();
  diagnostic_tooltips.clear();
  get_buffer()->remove_tag_by_name("def:warning_underline", get_buffer()->begin(), get_buffer()->end());
  get_buffer()->remove_tag_by_name("def:error_underline", get_buffer()->begin(), get_buffer()->end());
}

void Source::View::place_cursor_at_next_diagnostic() {
  auto insert_offset = get_buffer()->get_insert()->get_iter().get_offset();
  for(auto offset : diagnostic_offsets) {
    if(offset > insert_offset) {
      get_buffer()->place_cursor(get_buffer()->get_iter_at_offset(offset));
      scroll_to(get_buffer()->get_insert(), 0.0, 1.0, 0.5);
      return;
    }
  }
  if(diagnostic_offsets.size() == 0)
    Info::get().print("No diagnostics found in current buffer");
  else {
    auto iter = get_buffer()->get_iter_at_offset(*diagnostic_offsets.begin());
    get_buffer()->place_cursor(iter);
    scroll_to(get_buffer()->get_insert(), 0.0, 1.0, 0.5);
  }
}

bool Source::View::backward_to_code(Gtk::TextIter &iter) {
  while((*iter == ' ' || *iter == '\t' || iter.ends_line() || !is_code_iter(iter)) && iter.backward_char()) {
  }
  return !iter.is_start() || is_code_iter(iter);
}

bool Source::View::forward_to_code(Gtk::TextIter &iter) {
  while((*iter == ' ' || *iter == '\t' || iter.ends_line() || !is_code_iter(iter)) && iter.forward_char()) {
  }
  return !iter.is_end();
}

bool Source::View::backward_to_code_or_line_start(Gtk::TextIter &iter) {
  while(!iter.starts_line() && (*iter == ' ' || *iter == '\t' || iter.ends_line() || !is_code_iter(iter)) && iter.backward_char()) {
  }
  return !iter.is_start() || is_code_iter(iter);
}

bool Source::View::forward_to_code_or_line_end(Gtk::TextIter &iter) {
  while(!iter.ends_line() && (*iter == ' ' || *iter == '\t' || !is_code_iter(iter)) && iter.forward_char()) {
  }
  return !iter.is_end();
}

Gtk::TextIter Source::View::get_start_of_expression(Gtk::TextIter iter) {
  backward_to_code_or_line_start(iter);

  if(iter.starts_line())
    return iter;

  bool has_semicolon = false;
  bool has_open_curly = false;

  if(is_bracket_language) {
    if(*iter == ';' && is_code_iter(iter))
      has_semicolon = true;
    else if(*iter == '{' && is_code_iter(iter)) {
      iter.backward_char();
      has_open_curly = true;
    }
  }

  int para_count = 0;
  int square_count = 0;
  long curly_count = 0;

  do {
    if(*iter == '(' && is_code_iter(iter))
      para_count++;
    else if(*iter == ')' && is_code_iter(iter))
      para_count--;
    else if(*iter == '[' && is_code_iter(iter))
      square_count++;
    else if(*iter == ']' && is_code_iter(iter))
      square_count--;
    else if(*iter == '{' && is_code_iter(iter))
      curly_count++;
    else if(*iter == '}' && is_code_iter(iter)) {
      curly_count--;
      if(iter.starts_line())
        break;
    }

    if(para_count > 0 || square_count > 0 || curly_count > 0)
      break;

    if(iter.starts_line() && para_count == 0 && square_count == 0) {
      if(!is_bracket_language)
        return iter;
      // Handle << at the beginning of the sentence if iter initially started with ;
      if(has_semicolon) {
        auto test_iter = get_tabs_end_iter(iter);
        if(!test_iter.starts_line() && *test_iter == '<' && is_code_iter(test_iter) &&
           test_iter.forward_char() && *test_iter == '<')
          continue;
      }
      // Handle for instance: test\n  .test();
      if(has_semicolon && use_fixed_continuation_indenting) {
        auto test_iter = get_tabs_end_iter(iter);
        if(!test_iter.starts_line() && *test_iter == '.' && is_code_iter(test_iter))
          continue;
      }
      // Handle : at the beginning of the sentence if iter initially started with {
      if(has_open_curly) {
        auto test_iter = get_tabs_end_iter(iter);
        if(!test_iter.starts_line() && *test_iter == ':' && is_code_iter(test_iter))
          continue;
      }

      // Handle ',', ':', or operators that can be used between two lines, on previous line

      // Return if previous line is empty
      auto previous_iter = iter;
      previous_iter.backward_char();
      backward_to_code_or_line_start(previous_iter);
      if(previous_iter.starts_line())
        return iter;
      // Handle for instance: Test::Test():\n    test(2) {
      if(has_open_curly && *previous_iter == ':') {
        previous_iter.backward_char();
        backward_to_code_or_line_start(previous_iter);
        if(*previous_iter == ')') {
          auto token = get_token(get_tabs_end_iter(previous_iter));
          if(token != "case")
            continue;
        }
        return iter;
      }
      // Handle for instance: int a =\n    b;
      if(*previous_iter == '=' || *previous_iter == '+' || *previous_iter == '-' || *previous_iter == '*' || *previous_iter == '/' ||
         *previous_iter == '%' || *previous_iter == '<' || *previous_iter == '>' || *previous_iter == '&' || *previous_iter == '|') {
        if(has_semicolon)
          continue;
        return iter;
      }
      if(*previous_iter != ',')
        return iter;
      else {
        // Return if , is followed by }, for instance: {\n  1,\n  2,\n}
        auto next_iter = iter;
        if(forward_to_code_or_line_end(next_iter) && *next_iter == '}')
          return iter;
      }
    }
  } while(iter.backward_char());

  return iter;
}

bool Source::View::find_close_symbol_forward(Gtk::TextIter iter, Gtk::TextIter &found_iter, unsigned int positive_char, unsigned int negative_char) {
  long count = 0;
  if(positive_char == '{' && negative_char == '}') {
    do {
      if(*iter == positive_char && is_code_iter(iter))
        count++;
      else if(*iter == negative_char && is_code_iter(iter)) {
        if(count == 0) {
          found_iter = iter;
          return true;
        }
        count--;
      }
    } while(iter.forward_char());
    return false;
  }
  else {
    long curly_count = 0;
    do {
      if(curly_count == 0 && *iter == positive_char && is_code_iter(iter)) {
        if((is_c || is_cpp) && positive_char == '>') {
          auto prev = iter;
          if(prev.backward_char() && *prev == '-')
            continue;
        }
        else if(is_js && positive_char == '>') {
          auto prev = iter;
          if(prev.backward_char() && *prev == '=')
            continue;
        }
        count++;
      }
      else if(curly_count == 0 && *iter == negative_char && is_code_iter(iter)) {
        if((is_c || is_cpp) && negative_char == '>') {
          auto prev = iter;
          if(prev.backward_char() && *prev == '-')
            continue;
        }
        else if(is_js && negative_char == '>') {
          auto prev = iter;
          if(prev.backward_char() && *prev == '=')
            continue;
        }
        if(count == 0) {
          found_iter = iter;
          return true;
        }
        count--;
      }
      else if(*iter == '{' && is_code_iter(iter))
        curly_count++;
      else if(*iter == '}' && is_code_iter(iter)) {
        if(curly_count == 0)
          return false;
        curly_count--;
      }
    } while(iter.forward_char());
    return false;
  }
}

bool Source::View::find_open_symbol_backward(Gtk::TextIter iter, Gtk::TextIter &found_iter, unsigned int positive_char, unsigned int negative_char) {
  long count = 0;
  if(positive_char == '{' && negative_char == '}') {
    do {
      if(*iter == positive_char && is_code_iter(iter)) {
        if(count == 0) {
          found_iter = iter;
          return true;
        }
        count++;
      }
      else if(*iter == negative_char && is_code_iter(iter))
        count--;
    } while(iter.backward_char());
    return false;
  }
  else {
    long curly_count = 0;
    do {
      if(curly_count == 0 && *iter == positive_char && is_code_iter(iter)) {
        if((is_c || is_cpp) && positive_char == '>') {
          auto prev = iter;
          if(prev.backward_char() && *prev == '-')
            continue;
        }
        else if(is_js && positive_char == '>') {
          auto prev = iter;
          if(prev.backward_char() && *prev == '=')
            continue;
        }
        if(count == 0) {
          found_iter = iter;
          return true;
        }
        count++;
      }
      else if(curly_count == 0 && *iter == negative_char && is_code_iter(iter)) {
        if((is_c || is_cpp) && negative_char == '>') {
          auto prev = iter;
          if(prev.backward_char() && *prev == '-')
            continue;
        }
        else if(is_js && negative_char == '>') {
          auto prev = iter;
          if(prev.backward_char() && *prev == '=')
            continue;
        }
        count--;
      }
      else if(*iter == '{' && is_code_iter(iter)) {
        if(curly_count == 0)
          return false;
        curly_count++;
      }
      else if(*iter == '}' && is_code_iter(iter))
        curly_count--;
    } while(iter.backward_char());
    return false;
  }
}

long Source::View::symbol_count(Gtk::TextIter iter, unsigned int positive_char, unsigned int negative_char) {
  auto iter_stored = iter;
  long symbol_count = 0;

  if(positive_char == '{' && negative_char == '}') {
    // If checking top-level curly brackets, check whole buffer
    auto previous_iter = iter;
    if(iter.starts_line() || (previous_iter.backward_char() && previous_iter.starts_line() && *previous_iter == '{')) {
      auto iter = get_buffer()->begin();
      do {
        if(*iter == '{' && is_code_iter(iter))
          ++symbol_count;
        else if(*iter == '}' && is_code_iter(iter))
          --symbol_count;
      } while(iter.forward_char());
      return symbol_count;
    }
    // Can stop when text is found at top-level indentation
    else {
      do {
        if(*iter == '{' && is_code_iter(iter))
          ++symbol_count;
        else if(*iter == '}' && is_code_iter(iter))
          --symbol_count;
        if(iter.starts_line() && !iter.ends_line() && *iter != '#' && *iter != ' ' && *iter != '\t')
          break;
      } while(iter.backward_char());

      iter = iter_stored;
      if(!iter.forward_char())
        return symbol_count;

      do {
        if(*iter == '{' && is_code_iter(iter))
          ++symbol_count;
        else if(*iter == '}' && is_code_iter(iter))
          --symbol_count;
        if(iter.starts_line() && !iter.ends_line() && *iter != '#' && *iter != ' ' && *iter != '\t') {
          if(*iter == 'p') {
            auto token = get_token(iter);
            if(token == "public" || token == "protected" || token == "private")
              continue;
          }
          break;
        }
      } while(iter.forward_char());
      return symbol_count;
    }
  }

  long curly_count = 0;

  do {
    if(*iter == positive_char && is_code_iter(iter))
      symbol_count++;
    else if(*iter == negative_char && is_code_iter(iter))
      symbol_count--;
    else if(*iter == '{' && is_code_iter(iter)) {
      if(curly_count == 0)
        break;
      curly_count++;
    }
    else if(*iter == '}' && is_code_iter(iter))
      curly_count--;
  } while(iter.backward_char());

  iter = iter_stored;
  if(!iter.forward_char())
    return symbol_count;

  curly_count = 0;
  do {
    if(*iter == positive_char && is_code_iter(iter))
      symbol_count++;
    else if(*iter == negative_char && is_code_iter(iter))
      symbol_count--;
    else if(*iter == '{' && is_code_iter(iter))
      curly_count++;
    else if(*iter == '}' && is_code_iter(iter)) {
      if(curly_count == 0)
        break;
      curly_count--;
    }
  } while(iter.forward_char());

  return symbol_count;
}

bool Source::View::is_templated_function(Gtk::TextIter iter, Gtk::TextIter &parenthesis_end_iter) {
  auto iter_stored = iter;
  long bracket_count = 0;
  long curly_count = 0;

  if(!(iter.backward_char() && *iter == '>' && *iter_stored == '('))
    return false;

  do {
    if(*iter == '<' && is_code_iter(iter))
      bracket_count++;
    else if(*iter == '>' && is_code_iter(iter))
      bracket_count--;
    else if(*iter == '{' && is_code_iter(iter))
      curly_count++;
    else if(*iter == '}' && is_code_iter(iter))
      curly_count--;

    if(bracket_count == 0)
      break;

    if(curly_count > 0)
      break;
  } while(iter.backward_char());

  if(bracket_count != 0)
    return false;

  iter = iter_stored;
  bracket_count = 0;
  curly_count = 0;
  do {
    if(*iter == '(' && is_code_iter(iter))
      bracket_count++;
    else if(*iter == ')' && is_code_iter(iter))
      bracket_count--;
    else if(*iter == '{' && is_code_iter(iter))
      curly_count++;
    else if(*iter == '}' && is_code_iter(iter))
      curly_count--;

    if(bracket_count == 0) {
      parenthesis_end_iter = iter;
      return true;
    }

    if(curly_count < 0)
      return false;
  } while(iter.forward_char());

  return false;
}

bool Source::View::is_possible_argument() {
  auto iter = get_buffer()->get_insert()->get_iter();
  if(iter.backward_char() && (!interactive_completion || last_keyval == '(' || last_keyval == ',' || last_keyval == ' ' ||
                              last_keyval == GDK_KEY_Return || last_keyval == GDK_KEY_KP_Enter)) {
    if(backward_to_code(iter) && (*iter == '(' || *iter == ','))
      return true;
  }
  return false;
}

bool Source::View::on_key_press_event(GdkEventKey *event) {
  enable_multiple_cursors = true;
  ScopeGuard guard{[this] {
    enable_multiple_cursors = false;
  }};

  if(SelectionDialog::get() && SelectionDialog::get()->is_visible()) {
    if(SelectionDialog::get()->on_key_press(event))
      return true;
  }
  if(CompletionDialog::get() && CompletionDialog::get()->is_visible()) {
    if(CompletionDialog::get()->on_key_press(event))
      return true;
  }

  if(last_keyval < GDK_KEY_Shift_L || last_keyval > GDK_KEY_Hyper_R)
    previous_non_modifier_keyval = last_keyval;
  last_keyval = event->keyval;

  if((event->keyval == GDK_KEY_Tab || event->keyval == GDK_KEY_ISO_Left_Tab) && (event->state & GDK_SHIFT_MASK) == 0 && select_snippet_parameter())
    return true;
  if(on_key_press_event_extra_cursors(event))
    return true;

  {
    LockGuard lock(snippets_mutex);
    if(snippets) {
      guint keyval_without_state;
      gdk_keymap_translate_keyboard_state(gdk_keymap_get_default(), event->hardware_keycode, (GdkModifierType)0, 0, &keyval_without_state, nullptr, nullptr, nullptr);
      for(auto &snippet : *snippets) {
        if(snippet.key == keyval_without_state && (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK | GDK_META_MASK)) == snippet.modifier) {
          insert_snippet(get_buffer()->get_insert()->get_iter(), snippet.body);
          return true;
        }
      }
    }
  }

  get_buffer()->begin_user_action();

  // Shift+enter: go to end of line and enter
  if((event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) && (event->state & GDK_SHIFT_MASK) > 0) {
    auto iter = get_buffer()->get_insert()->get_iter();
    if(!iter.ends_line()) {
      iter.forward_to_line_end();
      get_buffer()->place_cursor(iter);
    }
  }

  if(Config::get().source.smart_brackets && on_key_press_event_smart_brackets(event)) {
    get_buffer()->end_user_action();
    return true;
  }
  if(Config::get().source.smart_inserts && on_key_press_event_smart_inserts(event)) {
    get_buffer()->end_user_action();
    return true;
  }

  if(is_bracket_language && on_key_press_event_bracket_language(event)) {
    get_buffer()->end_user_action();
    return true;
  }
  else if(on_key_press_event_basic(event)) {
    get_buffer()->end_user_action();
    return true;
  }
  else {
    get_buffer()->end_user_action();
    return BaseView::on_key_press_event(event);
  }
}

// Basic indentation
bool Source::View::on_key_press_event_basic(GdkEventKey *event) {
  auto iter = get_buffer()->get_insert()->get_iter();

  // Indent as in current or next line
  if((event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) && !get_buffer()->get_has_selection() && !iter.starts_line()) {
    remove_whitespace_characters(iter);

    iter = get_buffer()->get_insert()->get_iter();
    auto condition_iter = iter;
    condition_iter.backward_char();
    backward_to_code_or_line_start(condition_iter);
    auto start_iter = get_start_of_expression(condition_iter);
    auto tabs_end_iter = get_tabs_end_iter(start_iter);
    auto tabs = get_line_before(tabs_end_iter);

    // Python indenting after :
    if(*condition_iter == ':' && language_id == "python") {
      get_buffer()->insert_at_cursor('\n' + tabs + tab);
      scroll_to(get_buffer()->get_insert());
      return true;
    }

    // Indent as in current or next line
    int line_nr = iter.get_line();
    if(iter.ends_line() && (line_nr + 1) < get_buffer()->get_line_count()) {
      auto next_tabs_end_iter = get_tabs_end_iter(line_nr + 1);
      if(next_tabs_end_iter.get_line_offset() > tabs_end_iter.get_line_offset()) {
        get_buffer()->insert_at_cursor('\n' + get_line_before(next_tabs_end_iter));
        scroll_to(get_buffer()->get_insert());
        return true;
      }
    }
    get_buffer()->insert_at_cursor('\n' + tabs);
    scroll_to(get_buffer()->get_insert());
    return true;
  }
  // Indent as in next or previous line
  else if(event->keyval == GDK_KEY_Tab && (event->state & GDK_SHIFT_MASK) == 0) {
    // Special case if insert is at beginning of empty line:
    if(iter.starts_line() && iter.ends_line() && !get_buffer()->get_has_selection()) {
      auto prev_line_iter = iter;
      while(prev_line_iter.starts_line() && prev_line_iter.backward_char()) {
      }
      auto start_iter = get_start_of_expression(prev_line_iter);
      auto prev_line_tabs_end_iter = get_tabs_end_iter(start_iter);

      auto next_line_iter = iter;
      while(next_line_iter.starts_line() && next_line_iter.forward_char()) {
      }
      auto next_line_tabs_end_iter = get_tabs_end_iter(next_line_iter);

      Gtk::TextIter tabs_end_iter;
      if(next_line_tabs_end_iter.get_line_offset() > prev_line_tabs_end_iter.get_line_offset())
        tabs_end_iter = next_line_tabs_end_iter;
      else
        tabs_end_iter = prev_line_tabs_end_iter;
      auto tabs = get_line_before(tabs_end_iter);

      get_buffer()->insert_at_cursor(tabs.size() >= tab_size ? tabs : tab);
      return true;
    }

    if(!Config::get().source.tab_indents_line && !get_buffer()->get_has_selection()) {
      get_buffer()->insert_at_cursor(tab);
      return true;
    }

    // Indent right when clicking tab, no matter where in the line the cursor is. Also works on selected text.
    Gtk::TextIter selection_start, selection_end;
    get_buffer()->get_selection_bounds(selection_start, selection_end);
    Mark selection_end_mark(selection_end);
    int line_start = selection_start.get_line();
    int line_end = selection_end.get_line();
    for(int line = line_start; line <= line_end; line++) {
      auto line_it = get_buffer()->get_iter_at_line(line);
      if(!get_buffer()->get_has_selection() || line_it != selection_end_mark->get_iter())
        get_buffer()->insert(line_it, tab);
    }
    return true;
  }
  // Indent left when clicking shift-tab, no matter where in the line the cursor is. Also works on selected text.
  else if((event->keyval == GDK_KEY_ISO_Left_Tab || event->keyval == GDK_KEY_Tab) && (event->state & GDK_SHIFT_MASK) > 0) {
    Gtk::TextIter selection_start, selection_end;
    get_buffer()->get_selection_bounds(selection_start, selection_end);
    int line_start = selection_start.get_line();
    int line_end = selection_end.get_line();

    unsigned indent_left_steps = tab_size;
    std::vector<bool> ignore_line;
    for(int line_nr = line_start; line_nr <= line_end; line_nr++) {
      auto line_it = get_buffer()->get_iter_at_line(line_nr);
      if(!get_buffer()->get_has_selection() || line_it != selection_end) {
        auto tabs_end_iter = get_tabs_end_iter(line_nr);
        if(tabs_end_iter.starts_line() && tabs_end_iter.ends_line())
          ignore_line.push_back(true);
        else {
          auto line_tabs = get_line_before(tabs_end_iter);

          if(line_tabs.size() > 0) {
            indent_left_steps = std::min(indent_left_steps, static_cast<unsigned>(line_tabs.size()));
            ignore_line.push_back(false);
          }
          else
            return true;
        }
      }
    }

    for(int line_nr = line_start; line_nr <= line_end; line_nr++) {
      Gtk::TextIter line_it = get_buffer()->get_iter_at_line(line_nr);
      Gtk::TextIter line_plus_it = line_it;
      if(!get_buffer()->get_has_selection() || line_it != selection_end) {
        line_plus_it.forward_chars(indent_left_steps);
        if(!ignore_line.at(line_nr - line_start))
          get_buffer()->erase(line_it, line_plus_it);
      }
    }
    return true;
  }
  // "Smart" backspace key
  else if(event->keyval == GDK_KEY_BackSpace && !get_buffer()->get_has_selection()) {
    auto line = get_line_before();
    bool do_smart_backspace = true;
    for(auto &chr : line) {
      if(chr != ' ' && chr != '\t') {
        do_smart_backspace = false;
        break;
      }
    }
    if(iter.get_line() == 0) // Special case since there are no previous line
      do_smart_backspace = false;
    if(do_smart_backspace) {
      auto previous_line_end_iter = iter;
      if(previous_line_end_iter.backward_chars(line.size() + 1)) {
        if(!previous_line_end_iter.ends_line()) // For CR+LF
          previous_line_end_iter.backward_char();
        if(previous_line_end_iter.starts_line()) // When previous line is empty, keep tabs in current line
          get_buffer()->erase(previous_line_end_iter, get_buffer()->get_iter_at_line(iter.get_line()));
        else
          get_buffer()->erase(previous_line_end_iter, iter);
        return true;
      }
    }
  }
  // "Smart" delete key
  else if(event->keyval == GDK_KEY_Delete && !get_buffer()->get_has_selection()) {
    auto insert_iter = iter;
    bool do_smart_delete = true;
    do {
      if(*iter != ' ' && *iter != '\t' && !iter.ends_line()) {
        do_smart_delete = false;
        break;
      }
      if(iter.ends_line()) {
        if(!iter.forward_char())
          do_smart_delete = false;
        break;
      }
    } while(iter.forward_char());
    if(do_smart_delete) {
      if(!insert_iter.starts_line()) {
        while((*iter == ' ' || *iter == '\t') && iter.forward_char()) {
        }
      }
      get_buffer()->erase(insert_iter, iter);
      return true;
    }
  }

  // Workaround for TextView::on_key_press_event bug sometimes causing segmentation faults
  // TODO: figure out the bug and create pull request to gtk
  // Have only experienced this on OS X
  // Note: valgrind reports issues on TextView::on_key_press_event as well
  auto unicode = gdk_keyval_to_unicode(event->keyval);
  if((event->state & (GDK_CONTROL_MASK | GDK_META_MASK)) == 0 && unicode >= 32 && unicode != 127 &&
     (previous_non_modifier_keyval < GDK_KEY_dead_grave || previous_non_modifier_keyval > GDK_KEY_dead_greek)) {
    if(get_buffer()->get_has_selection()) {
      Gtk::TextIter selection_start, selection_end;
      get_buffer()->get_selection_bounds(selection_start, selection_end);
      get_buffer()->erase(selection_start, selection_end);
    }
    get_buffer()->insert_at_cursor(Glib::ustring(1, unicode));
    scroll_to(get_buffer()->get_insert());

    // Trick to make the cursor visible right after insertion:
    set_cursor_visible(false);
    set_cursor_visible();

    return true;
  }

  return false;
}

// Bracket language indentation
bool Source::View::on_key_press_event_bracket_language(GdkEventKey *event) {
  const static std::regex no_bracket_statement_regex("^[ \t]*(if( +constexpr)?|for|while) *\\(.*[^;}{] *$|"
                                                     "^[ \t]*[}]? *else if( +constexpr)? *\\(.*[^;}{] *$|"
                                                     "^[ \t]*[}]? *else *$",
                                                     std::regex::extended | std::regex::optimize);

  auto iter = get_buffer()->get_insert()->get_iter();

  if(get_buffer()->get_has_selection())
    return false;

  if(!is_code_iter(iter)) {
    // Add * at start of line in comment blocks
    if(event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) {
      if(!iter.starts_line() && (!string_tag || (!iter.has_tag(string_tag) && !iter.ends_tag(string_tag)))) {
        remove_whitespace_characters(iter);
        iter = get_buffer()->get_insert()->get_iter();

        auto start_iter = get_tabs_end_iter(iter.get_line());
        if(!is_code_iter(start_iter)) {
          auto end_iter = start_iter;
          end_iter.forward_chars(2);
          auto start_of_sentence = get_buffer()->get_text(start_iter, end_iter);
          if(!start_of_sentence.empty() && (start_of_sentence == "/*" || start_of_sentence[0] == '*')) {
            auto tabs = get_line_before(start_iter);
            auto insert_str = '\n' + tabs;
            if(start_of_sentence[0] == '/')
              insert_str += ' ';
            insert_str += "* ";

            get_buffer()->insert_at_cursor(insert_str);
            return true;
          }
        }
      }
      else if(!comment_tag || !iter.ends_tag(comment_tag))
        return false;
    }
    else
      return false;
  }

  // Indent depending on if/else/etc and brackets
  if((event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) && !iter.starts_line()) {
    remove_whitespace_characters(iter);
    iter = get_buffer()->get_insert()->get_iter();

    auto previous_iter = iter;
    previous_iter.backward_char();
    // Remove matching bracket highlights that get extended when inserting text in between the brackets
    ScopeGuard guard;
    if((*previous_iter == '{' && *iter == '}') || (*previous_iter == '(' && *iter == ')') ||
       (*previous_iter == '[' && *iter == ']') || (*previous_iter == '<' && *iter == '>')) {
      get_source_buffer()->set_highlight_matching_brackets(false);
      guard.on_exit = [this] {
        get_source_buffer()->set_highlight_matching_brackets(true);
      };
    }

    auto condition_iter = previous_iter;
    backward_to_code_or_line_start(condition_iter);
    auto start_iter = get_start_of_expression(condition_iter);
    auto tabs_end_iter = get_tabs_end_iter(start_iter);
    auto tabs = get_line_before(tabs_end_iter);

    /*
     * Change tabs after ending comment block with an extra space (as in this case)
     */
    if(tabs.size() % tab_size == 1 && !start_iter.ends_line() && !is_code_iter(start_iter)) {
      auto end_of_line_iter = start_iter;
      end_of_line_iter.forward_to_line_end();
      if(starts_with(get_buffer()->get_text(tabs_end_iter, end_of_line_iter).raw(), "*/")) {
        tabs.pop_back();
        get_buffer()->insert_at_cursor('\n' + tabs);
        scroll_to(get_buffer()->get_insert());
        return true;
      }
    }

    // Indent enter inside HTML or JSX elements, for instance:
    // <div>CURSOR</div>
    // to:
    // <div>
    //   CURSOR
    // </div>
    // and
    // <div>CURSORtest
    // to
    // <div>
    //   CURSORtest
    if(*condition_iter == '>' && is_js) {
      auto prev = condition_iter;
      Gtk::TextIter open_element_iter;
      if(prev.backward_char() && backward_to_code(prev) && *prev != '/' &&
         find_open_symbol_backward(prev, open_element_iter, '<', '>') &&
         open_element_iter.forward_char() && forward_to_code(open_element_iter) && *open_element_iter != '/') {
        auto get_element = [this](Gtk::TextIter iter) {
          auto start = iter;
          auto end = iter;
          while(iter.backward_char() && (is_token_char(*iter) || *iter == '.'))
            start = iter;
          while((is_token_char(*end) || *end == '.') && end.forward_char()) {
          }
          return get_buffer()->get_text(start, end);
        };
        auto open_element_token = get_element(open_element_iter);
        static std::vector<std::string> void_elements = {"area", "base", "br", "col", "command", "embed", "hr", "img", "input", "keygen", "link", "meta", "param", "source", "track", "wbr"};
        if(std::none_of(void_elements.begin(), void_elements.end(), [&open_element_token](const std::string &e) { return e == open_element_token; })) {
          auto close_element_iter = iter;
          // If cursor is placed between open and close tag
          if(*close_element_iter == '<' && close_element_iter.forward_char() && forward_to_code(close_element_iter) && *close_element_iter == '/' &&
             close_element_iter.forward_char() && forward_to_code(close_element_iter) &&
             open_element_token == get_element(close_element_iter)) {
            get_buffer()->insert_at_cursor('\n' + tabs + tab + '\n' + tabs);
            auto insert_it = get_buffer()->get_insert()->get_iter();
            if(insert_it.backward_chars(tabs.size() + 1)) {
              scroll_to(get_buffer()->get_insert());
              get_buffer()->place_cursor(insert_it);
            }
            return true;
          }
          close_element_iter = iter;
          // Add closing tag if missing
          if(close_element_iter.ends_line()) {
            forward_to_code(close_element_iter);
            auto close_element_tabs_size = static_cast<size_t>(get_tabs_end_iter(close_element_iter).get_line_offset());
            if(!close_element_iter || close_element_tabs_size < tabs.size() ||
               (close_element_tabs_size == tabs.size() && (*close_element_iter == '{' ||
                                                           (*close_element_iter == '<' && close_element_iter.forward_char() && forward_to_code(close_element_iter) &&
                                                            !(*close_element_iter == '/' && close_element_iter.forward_char() && forward_to_code(close_element_iter) && get_element(close_element_iter) == open_element_token))))) {
              get_buffer()->insert_at_cursor('\n' + tabs + tab + '\n' + tabs + "</" + open_element_token + '>');
              auto insert_it = get_buffer()->get_insert()->get_iter();
              if(insert_it.backward_chars(tabs.size() + 1 + open_element_token.size() + 3)) {
                scroll_to(get_buffer()->get_insert());
                get_buffer()->place_cursor(insert_it);
              }
              return true;
            }
          }
          // If line ends with closing element, move content after open to new line, and closing tag on the next line thereafter
          close_element_iter = iter;
          if(!close_element_iter.ends_line())
            close_element_iter.forward_to_line_end();
          if(close_element_iter.backward_char() && *close_element_iter == '>' && close_element_iter.backward_char() &&
             find_open_symbol_backward(close_element_iter, close_element_iter, '<', '>')) {
            auto start_close_element_iter = close_element_iter;
            auto content_size = start_close_element_iter.get_offset() - iter.get_offset();
            if(close_element_iter.forward_char() && forward_to_code(close_element_iter) &&
               *close_element_iter == '/' && close_element_iter.forward_char() && forward_to_code(close_element_iter) && get_element(close_element_iter) == open_element_token) {
              get_buffer()->insert_at_cursor('\n' + tabs + tab);
              auto close_element_start_iter = get_buffer()->get_insert()->get_iter();
              close_element_start_iter.forward_chars(content_size);
              get_buffer()->insert(close_element_start_iter, '\n' + tabs);
              scroll_to(get_buffer()->get_insert());
              return true;
            }
          }
          get_buffer()->insert_at_cursor('\n' + tabs + tab);
          scroll_to(get_buffer()->get_insert());
          return true;
        }
      }
    }

    // Special indentation of {, [ and ( for for instance JavaScript, JSON, Rust
    if(use_fixed_continuation_indenting) {
      unsigned int open_symbol = 0, close_symbol = 0;
      if(*condition_iter == '[') {
        open_symbol = '[';
        close_symbol = ']';
      }
      else if(*condition_iter == '(') {
        open_symbol = '(';
        close_symbol = ')';
      }
      else if(*condition_iter == '{') {
        open_symbol = '{';
        close_symbol = '}';
      }
      if(open_symbol != 0 && is_code_iter(condition_iter)) {
        Gtk::TextIter found_iter;
        // Check if an ), ] or } is needed
        bool has_right_bracket = false;
        if(find_close_symbol_forward(iter, found_iter, open_symbol, close_symbol)) {
          auto found_tabs_end_iter = get_tabs_end_iter(found_iter);
          if(found_tabs_end_iter.get_line_offset() == tabs_end_iter.get_line_offset())
            has_right_bracket = true;
        }

        if(*iter == close_symbol) {
          get_buffer()->insert_at_cursor('\n' + tabs + tab + '\n' + tabs);
          auto insert_it = get_buffer()->get_insert()->get_iter();
          if(insert_it.backward_chars(tabs.size() + 1)) {
            scroll_to(get_buffer()->get_insert());
            get_buffer()->place_cursor(insert_it);
          }
          return true;
        }
        else if(!has_right_bracket) {
          // If line does not end with: (,[, or {, move contents after the left bracket to next line inside brackets
          if(!iter.ends_line() && *iter != ')' && *iter != ']' && *iter != '}') {
            get_buffer()->insert_at_cursor('\n' + tabs + tab);
            auto iter = get_buffer()->get_insert()->get_iter();
            Mark mark(iter);
            iter.forward_to_line_end();
            get_buffer()->insert(iter, '\n' + tabs + static_cast<char>(close_symbol));
            scroll_to(get_buffer()->get_insert());
            get_buffer()->place_cursor(mark->get_iter());
            return true;
          }
          else {
            // Insert new lines with bracket end
            get_buffer()->insert_at_cursor('\n' + tabs + tab + '\n' + tabs + static_cast<char>(close_symbol));
            auto insert_it = get_buffer()->get_insert()->get_iter();
            if(insert_it.backward_chars(tabs.size() + 2)) {
              scroll_to(get_buffer()->get_insert());
              get_buffer()->place_cursor(insert_it);
            }
            return true;
          }
        }
        else {
          get_buffer()->insert_at_cursor('\n' + tabs + tab);
          scroll_to(get_buffer()->get_insert());
          return true;
        }
      }

      // JavaScript: simplified indentations inside brackets, after for example:
      // [\n  1, 2, 3,\n
      // return (\n
      // ReactDOM.render(\n  <div>\n
      Gtk::TextIter found_iter;
      auto after_start_iter = start_iter;
      after_start_iter.forward_char();
      if((*start_iter == '[' && (!find_close_symbol_forward(after_start_iter, found_iter, '[', ']') || found_iter > iter)) ||
         (*start_iter == '(' && (!find_close_symbol_forward(after_start_iter, found_iter, '(', ')') || found_iter > iter)) ||
         (*start_iter == '{' && (!find_close_symbol_forward(after_start_iter, found_iter, '{', '}') || found_iter > iter))) {
        get_buffer()->insert_at_cursor('\n' + tabs + tab);
        scroll_to(get_buffer()->get_insert());
        return true;
      }
    }
    else {
      if(*condition_iter == '{' && is_code_iter(condition_iter)) {
        Gtk::TextIter close_iter;
        // Check if an '}' is needed
        bool has_right_curly_bracket = false;
        if(find_close_symbol_forward(iter, close_iter, '{', '}')) {
          auto found_tabs_end_iter = get_tabs_end_iter(close_iter);
          if(found_tabs_end_iter.get_line_offset() == tabs_end_iter.get_line_offset()) {
            has_right_curly_bracket = true;
            // Special case for functions and classes with no indentation after: namespace {
            if(is_cpp && tabs_end_iter.starts_line()) {
              auto iter = condition_iter;
              Gtk::TextIter open_iter;
              if(iter.backward_char() && find_open_symbol_backward(iter, open_iter, '{', '}')) {
                if(open_iter.starts_line()) // in case of: namespace test\n{
                  open_iter.backward_char();
                auto iter = get_buffer()->get_iter_at_line(open_iter.get_line());
                if(get_token(iter) == "namespace")
                  has_right_curly_bracket = close_iter.forward_char() && find_close_symbol_forward(close_iter, close_iter, '{', '}');
              }
            }
          }
          /**
           * Handle pressing enter after '{' in special expressions like:
           * enum class A { a,
           *                b }
           */
          else if(close_iter.get_line() > condition_iter.get_line() && close_iter.get_line_offset() > condition_iter.get_line_offset()) {
            get_buffer()->insert_at_cursor('\n' + get_line_before(get_tabs_end_iter(condition_iter.get_line() + 1)));
            scroll_to(get_buffer()->get_insert());
            return true;
          }
          /**
           * Handle pressing enter after '{' in special expressions like:
           * test(2,
           *      []() {
           *        func();
           *      });
           */
          else if(close_iter.get_line() > condition_iter.get_line() && close_iter.get_line_offset() == get_tabs_end_iter(condition_iter).get_line_offset()) {
            get_buffer()->insert_at_cursor('\n' + get_line_before(get_tabs_end_iter(condition_iter)) + tab);
            scroll_to(get_buffer()->get_insert());
            return true;
          }
        }

        // Check if one should add semicolon after '}'
        bool add_semicolon = false;
        if(is_c || is_cpp) {
          // add semicolon after class or struct?
          auto token = get_token(tabs_end_iter);
          if(token == "class" || token == "struct")
            add_semicolon = true;
          // Add semicolon after lambda unless it's an argument
          else if(*start_iter != '(' && *start_iter != '{' && *start_iter != '[') {
            auto it = condition_iter;
            long para_count = 0;
            long square_count = 0;
            bool square_outside_para_found = false;
            while(it.backward_char()) {
              if(*it == ']' && is_code_iter(it)) {
                --square_count;
                if(para_count == 0)
                  square_outside_para_found = true;
              }
              else if(*it == '[' && is_code_iter(it))
                ++square_count;
              else if(*it == ')' && is_code_iter(it))
                --para_count;
              else if(*it == '(' && is_code_iter(it))
                ++para_count;

              if(square_outside_para_found && square_count == 0 && para_count == 0) {
                // Look for equal sign
                while(it.backward_char() && (*it == ' ' || *it == '\t' || it.ends_line())) {
                }
                if(*it == '=')
                  add_semicolon = true;
                break;
              }
              if(it == start_iter)
                break;
              if(!square_outside_para_found && square_count == 0 && para_count == 0) {
                if(is_token_char(*it) ||
                   *it == '-' || *it == ' ' || *it == '\t' || *it == '<' || *it == '>' || *it == '(' || *it == ':' ||
                   *it == '*' || *it == '&' || *it == '/' || it.ends_line() || !is_code_iter(it)) {
                  continue;
                }
                else
                  break;
              }
            }
          }
        }

        if(*iter == '}') {
          get_buffer()->insert_at_cursor('\n' + tabs + tab + '\n' + tabs);
          if(add_semicolon) {
            // Check if semicolon exists
            auto next_iter = get_buffer()->get_insert()->get_iter();
            next_iter.forward_char();
            if(*next_iter != ';')
              get_buffer()->insert(next_iter, ";");
          }
          auto insert_it = get_buffer()->get_insert()->get_iter();
          if(insert_it.backward_chars(tabs.size() + 1)) {
            scroll_to(get_buffer()->get_insert());
            get_buffer()->place_cursor(insert_it);
          }
          return true;
        }
        else if(!has_right_curly_bracket) {
          // If line does not end with: {, move contents after { to next line inside brackets
          if(!iter.ends_line() && *iter != ')' && *iter != ']') {
            get_buffer()->insert_at_cursor('\n' + tabs + tab);
            auto iter = get_buffer()->get_insert()->get_iter();
            Mark mark(iter);
            iter.forward_to_line_end();
            get_buffer()->insert(iter, '\n' + tabs + '}');
            scroll_to(get_buffer()->get_insert());
            get_buffer()->place_cursor(mark->get_iter());
            return true;
          }
          else {
            get_buffer()->insert_at_cursor('\n' + tabs + tab + '\n' + tabs + (add_semicolon ? "};" : "}"));
            auto insert_it = get_buffer()->get_insert()->get_iter();
            if(insert_it.backward_chars(tabs.size() + (add_semicolon ? 3 : 2))) {
              scroll_to(get_buffer()->get_insert());
              get_buffer()->place_cursor(insert_it);
            }
            return true;
          }
        }
        else {
          get_buffer()->insert_at_cursor('\n' + tabs + tab);
          scroll_to(get_buffer()->get_insert());
          return true;
        }
      }

      // Indent multiline expressions
      if(*start_iter == '(' || *start_iter == '[') {
        auto iter = get_tabs_end_iter(start_iter);
        auto tabs = get_line_before(iter);
        while(iter <= start_iter) {
          tabs += ' ';
          iter.forward_char();
        }
        get_buffer()->insert_at_cursor('\n' + tabs);
        scroll_to(get_buffer()->get_insert());
        return true;
      }
    }

    auto after_condition_iter = condition_iter;
    after_condition_iter.forward_char();
    std::string sentence = get_buffer()->get_text(start_iter, after_condition_iter);
    std::smatch sm;
    // Indenting after for instance: if(...)\n
    if(std::regex_match(sentence, sm, no_bracket_statement_regex)) {
      get_buffer()->insert_at_cursor('\n' + tabs + tab);
      scroll_to(get_buffer()->get_insert());
      return true;
    }
    // Indenting after for instance: if(...)\n...;\n
    else if(*condition_iter == ';' && condition_iter.get_line() > 0 && is_code_iter(condition_iter)) {
      auto previous_end_iter = start_iter;
      while(previous_end_iter.backward_char() && !previous_end_iter.ends_line()) {
      }
      backward_to_code_or_line_start(previous_end_iter);
      auto previous_start_iter = get_tabs_end_iter(get_buffer()->get_iter_at_line(get_start_of_expression(previous_end_iter).get_line()));
      auto previous_tabs = get_line_before(previous_start_iter);
      if(!previous_end_iter.ends_line())
        previous_end_iter.forward_char();
      std::string previous_sentence = get_buffer()->get_text(previous_start_iter, previous_end_iter);
      std::smatch sm2;
      if(std::regex_match(previous_sentence, sm2, no_bracket_statement_regex)) {
        get_buffer()->insert_at_cursor('\n' + previous_tabs);
        scroll_to(get_buffer()->get_insert());
        return true;
      }
    }
    // Indenting after ':'
    else if(*condition_iter == ':' && is_code_iter(condition_iter)) {
      bool perform_indent = true;
      auto iter = condition_iter;
      if(!iter.starts_line())
        iter.backward_char();
      backward_to_code_or_line_start(iter);
      if(*iter == ')') {
        auto token = get_token(get_tabs_end_iter(get_buffer()->get_iter_at_line(iter.get_line())));
        if(token != "case") // Do not move left for instance: void Test::Test():
          perform_indent = false;
      }

      if(perform_indent) {
        Gtk::TextIter found_curly_iter;
        if(find_open_symbol_backward(iter, found_curly_iter, '{', '}')) {
          auto tabs_end_iter = get_tabs_end_iter(get_buffer()->get_iter_at_line(found_curly_iter.get_line()));
          auto tabs_start_of_sentence = get_line_before(tabs_end_iter);
          if(tabs.size() == (tabs_start_of_sentence.size() + tab_size)) {
            auto start_line_iter = get_buffer()->get_iter_at_line(iter.get_line());
            auto start_line_plus_tab_size = start_line_iter;
            for(size_t c = 0; c < tab_size; c++)
              start_line_plus_tab_size.forward_char();
            get_buffer()->erase(start_line_iter, start_line_plus_tab_size);
            get_buffer()->insert_at_cursor('\n' + tabs);
            scroll_to(get_buffer()->get_insert());
            return true;
          }
          else {
            get_buffer()->insert_at_cursor('\n' + tabs + tab);
            scroll_to(get_buffer()->get_insert());
            return true;
          }
        }
      }
    }

    // Indent as in current or next line
    int line_nr = iter.get_line();
    if(iter.ends_line() && (line_nr + 1) < get_buffer()->get_line_count()) {
      auto next_tabs_end_iter = get_tabs_end_iter(line_nr + 1);
      if(next_tabs_end_iter.get_line_offset() > tabs_end_iter.get_line_offset()) {
        get_buffer()->insert_at_cursor('\n' + get_line_before(next_tabs_end_iter));
        scroll_to(get_buffer()->get_insert());
        return true;
      }
    }
    get_buffer()->insert_at_cursor('\n' + tabs);
    scroll_to(get_buffer()->get_insert());
    return true;
  }
  // Indent left when writing }, ) or ] on a new line
  else if(event->keyval == GDK_KEY_braceright ||
          (use_fixed_continuation_indenting && (event->keyval == GDK_KEY_bracketright || event->keyval == GDK_KEY_parenright))) {
    std::string bracket;
    if(event->keyval == GDK_KEY_braceright)
      bracket = "}";
    if(event->keyval == GDK_KEY_bracketright)
      bracket = "]";
    else if(event->keyval == GDK_KEY_parenright)
      bracket = ")";
    std::string line = get_line_before();
    if(line.size() >= tab_size && iter.ends_line()) {
      bool indent_left = true;
      for(auto c : line) {
        if(c != tab_char) {
          indent_left = false;
          break;
        }
      }
      if(indent_left) {
        auto line_it = get_buffer()->get_iter_at_line(iter.get_line());
        auto line_plus_it = line_it;
        line_plus_it.forward_chars(tab_size);
        get_buffer()->erase(line_it, line_plus_it);
        get_buffer()->insert_at_cursor(bracket);
        return true;
      }
    }
  }
  // Indent left when writing { on a new line after for instance if(...)\n...
  else if(event->keyval == GDK_KEY_braceleft) {
    auto tabs_end_iter = get_tabs_end_iter();
    auto tabs = get_line_before(tabs_end_iter);
    size_t line_nr = iter.get_line();
    if(line_nr > 0 && tabs.size() >= tab_size && iter == tabs_end_iter) {
      auto previous_end_iter = iter;
      while(previous_end_iter.backward_char() && !previous_end_iter.ends_line()) {
      }
      auto condition_iter = previous_end_iter;
      backward_to_code_or_line_start(condition_iter);
      auto previous_start_iter = get_tabs_end_iter(get_buffer()->get_iter_at_line(get_start_of_expression(condition_iter).get_line()));
      auto previous_tabs = get_line_before(previous_start_iter);
      auto after_condition_iter = condition_iter;
      after_condition_iter.forward_char();
      if((tabs.size() - tab_size) == previous_tabs.size()) {
        std::string previous_sentence = get_buffer()->get_text(previous_start_iter, after_condition_iter);
        std::smatch sm;
        if(std::regex_match(previous_sentence, sm, no_bracket_statement_regex)) {
          auto start_iter = iter;
          start_iter.backward_chars(tab_size);
          get_buffer()->erase(start_iter, iter);
          get_buffer()->insert_at_cursor("{");
          scroll_to(get_buffer()->get_insert());
          return true;
        }
      }
    }
  }
  // Mark parameters of templated functions after pressing tab and after writing template argument
  else if(event->keyval == GDK_KEY_Tab && (event->state & GDK_SHIFT_MASK) == 0) {
    if(*iter == '>') {
      iter.forward_char();
      Gtk::TextIter parenthesis_end_iter;
      if(*iter == '(' && is_templated_function(iter, parenthesis_end_iter)) {
        iter.forward_char();
        get_buffer()->select_range(iter, parenthesis_end_iter);
        scroll_to(get_buffer()->get_insert());
        return true;
      }
    }
    // Special case if insert is at beginning of empty line:
    else if(iter.starts_line() && iter.ends_line() && !get_buffer()->get_has_selection()) {
      // Indenting after for instance: if(...)\n...;\n
      auto condition_iter = iter;
      while(condition_iter.starts_line() && condition_iter.backward_char()) {
      }
      backward_to_code_or_line_start(condition_iter);
      if(*condition_iter == ';' && condition_iter.get_line() > 0 && is_code_iter(condition_iter)) {
        auto start_iter = get_start_of_expression(condition_iter);
        auto previous_end_iter = start_iter;
        while(previous_end_iter.backward_char() && !previous_end_iter.ends_line()) {
        }
        backward_to_code_or_line_start(previous_end_iter);
        auto previous_start_iter = get_tabs_end_iter(get_buffer()->get_iter_at_line(get_start_of_expression(previous_end_iter).get_line()));
        auto previous_tabs = get_line_before(previous_start_iter);
        if(!previous_end_iter.ends_line())
          previous_end_iter.forward_char();
        std::string previous_sentence = get_buffer()->get_text(previous_start_iter, previous_end_iter);
        std::smatch sm2;
        if(std::regex_match(previous_sentence, sm2, no_bracket_statement_regex)) {
          get_buffer()->insert_at_cursor(previous_tabs);
          scroll_to(get_buffer()->get_insert());
          return true;
        }
      }
    }
  }

  return false;
}

bool Source::View::on_key_press_event_smart_brackets(GdkEventKey *event) {
  if(get_buffer()->get_has_selection())
    return false;

  auto iter = get_buffer()->get_insert()->get_iter();
  auto previous_iter = iter;
  previous_iter.backward_char();
  if(is_code_iter(iter)) {
    // Move after ')' if closed expression
    if(event->keyval == GDK_KEY_parenright) {
      if(*iter == ')' && symbol_count(iter, '(', ')') <= 0) {
        iter.forward_char();
        get_buffer()->place_cursor(iter);
        scroll_to(get_buffer()->get_insert());
        return true;
      }
    }
    // Move after '>' if >( and closed expression
    else if(event->keyval == GDK_KEY_greater) {
      if(*iter == '>') {
        iter.forward_char();
        Gtk::TextIter parenthesis_end_iter;
        if(*iter == '(' && is_templated_function(iter, parenthesis_end_iter)) {
          get_buffer()->place_cursor(iter);
          scroll_to(get_buffer()->get_insert());
          return true;
        }
      }
    }
    // Move after '(' if >( and select text inside parentheses
    else if(event->keyval == GDK_KEY_parenleft) {
      auto previous_iter = iter;
      previous_iter.backward_char();
      if(*previous_iter == '>') {
        Gtk::TextIter parenthesis_end_iter;
        if(*iter == '(' && is_templated_function(iter, parenthesis_end_iter)) {
          iter.forward_char();
          get_buffer()->select_range(iter, parenthesis_end_iter);
          scroll_to(iter);
          return true;
        }
      }
    }
  }

  return false;
}

bool Source::View::on_key_press_event_smart_inserts(GdkEventKey *event) {
  keep_snippet_marks = true;
  ScopeGuard guard{[this] {
    keep_snippet_marks = false;
  }};

  if(get_buffer()->get_has_selection()) {
    if(is_bracket_language) {
      // Remove /**/ around selection
      if(event->keyval == GDK_KEY_slash) {
        Gtk::TextIter start, end;
        get_buffer()->get_selection_bounds(start, end);
        auto before_start = start;
        auto after_end = end;
        if(before_start.backward_char() && *before_start == '*' && before_start.backward_char() && *before_start == '/' &&
           *after_end == '*' && after_end.forward_char() && *after_end == '/') {
          Mark start_mark(start);
          Mark end_mark(end);
          get_buffer()->erase(before_start, start);
          after_end = end_mark->get_iter();
          after_end.forward_chars(2);
          get_buffer()->erase(end_mark->get_iter(), after_end);

          get_buffer()->select_range(start_mark->get_iter(), end_mark->get_iter());
          return true;
        }
      }
    }

    Glib::ustring left, right;
    // Insert () around selection
    if(event->keyval == GDK_KEY_parenleft) {
      left = '(';
      right = ')';
    }
    // Insert [] around selection
    else if(event->keyval == GDK_KEY_bracketleft) {
      left = '[';
      right = ']';
    }
    // Insert {} around selection
    else if(event->keyval == GDK_KEY_braceleft) {
      left = '{';
      right = '}';
    }
    // Insert <> around selection
    else if(event->keyval == GDK_KEY_less) {
      left = '<';
      right = '>';
    }
    // Insert '' around selection
    else if(event->keyval == GDK_KEY_apostrophe) {
      left = '\'';
      right = '\'';
    }
    // Insert "" around selection
    else if(event->keyval == GDK_KEY_quotedbl) {
      left = '"';
      right = '"';
    }
    // Insert /**/ around selection
    else if(is_bracket_language && event->keyval == GDK_KEY_slash) {
      left = "/*";
      right = "*/";
    }
    // Insert $$ around selection
    else if(event->keyval == GDK_KEY_dollar && language_id == "latex") {
      left = '$';
      right = '$';
    }
    else if(language_id == "markdown" ||
            !is_code_iter(get_buffer()->get_insert()->get_iter()) || !is_code_iter(get_buffer()->get_selection_bound()->get_iter())) {
      // Insert `` around selection
      if(event->keyval == GDK_KEY_dead_grave) {
        left = '`';
        right = '`';
      }
      // Insert ** around selection
      else if(event->keyval == GDK_KEY_asterisk) {
        left = '*';
        right = '*';
      }
      // Insert __ around selection
      else if(event->keyval == GDK_KEY_underscore) {
        left = '_';
        right = '_';
      }
      // Insert ~~ around selection
      else if(event->keyval == GDK_KEY_dead_tilde) {
        left = '~';
        right = '~';
      }
    }
    if(!left.empty() && !right.empty()) {
      Gtk::TextIter start, end;
      get_buffer()->get_selection_bounds(start, end);
      Mark start_mark(start);
      Mark end_mark(end);
      get_buffer()->insert(start, left);
      get_buffer()->insert(end_mark->get_iter(), right);

      auto start_mark_next_iter = start_mark->get_iter();
      start_mark_next_iter.forward_chars(left.size());
      get_buffer()->select_range(start_mark_next_iter, end_mark->get_iter());
      return true;
    }
    return false;
  }

  auto iter = get_buffer()->get_insert()->get_iter();
  auto previous_iter = iter;
  previous_iter.backward_char();
  auto next_iter = iter;
  next_iter.forward_char();

  auto allow_insertion = [](const Gtk::TextIter &iter) {
    if(iter.ends_line() || *iter == ' ' || *iter == '\t' || *iter == ';' || *iter == ',' ||
       *iter == ')' || *iter == ']' || *iter == '}' || *iter == '<' || *iter == '>' || *iter == '/')
      return true;
    return false;
  };

  if(is_code_iter(iter)) {
    // Insert ()
    if(event->keyval == GDK_KEY_parenleft && allow_insertion(iter)) {
      if(symbol_count(iter, '(', ')') >= 0) {
        get_buffer()->insert_at_cursor(")");
        auto iter = get_buffer()->get_insert()->get_iter();
        iter.backward_char();
        get_buffer()->place_cursor(iter);
        scroll_to(get_buffer()->get_insert());
        return false;
      }
    }
    // Insert []
    else if(event->keyval == GDK_KEY_bracketleft && allow_insertion(iter)) {
      if(symbol_count(iter, '[', ']') >= 0) {
        get_buffer()->insert_at_cursor("]");
        auto iter = get_buffer()->get_insert()->get_iter();
        iter.backward_char();
        get_buffer()->place_cursor(iter);
        scroll_to(get_buffer()->get_insert());
        return false;
      }
    }
    // Move right on ] in []
    else if(event->keyval == GDK_KEY_bracketright) {
      if(*iter == ']' && symbol_count(iter, '[', ']') <= 0) {
        iter.forward_char();
        get_buffer()->place_cursor(iter);
        scroll_to(get_buffer()->get_insert());
        return true;
      }
    }
    // Insert {}
    else if(event->keyval == GDK_KEY_braceleft && allow_insertion(iter)) {
      auto start_iter = get_start_of_expression(iter);
      // Do not add } if { is at end of line and next line has a higher indentation
      auto test_iter = iter;
      while(!test_iter.ends_line() && (*test_iter == ' ' || *test_iter == '\t' || !is_code_iter(test_iter)) && test_iter.forward_char()) {
      }
      if(test_iter.ends_line()) {
        if(iter.get_line() + 1 < get_buffer()->get_line_count() && *start_iter != '(' && *start_iter != '[' && *start_iter != '{') {
          auto tabs_end_iter = (get_tabs_end_iter(get_buffer()->get_iter_at_line(start_iter.get_line())));
          auto next_line_iter = get_buffer()->get_iter_at_line(iter.get_line() + 1);
          auto next_line_tabs_end_iter = (get_tabs_end_iter(get_buffer()->get_iter_at_line(next_line_iter.get_line())));
          if(next_line_tabs_end_iter.get_line_offset() > tabs_end_iter.get_line_offset())
            return false;
        }
      }

      Gtk::TextIter close_iter;
      bool has_right_curly_bracket = false;
      auto tabs_end_iter = get_tabs_end_iter(start_iter);
      if(find_close_symbol_forward(iter, close_iter, '{', '}')) {
        auto found_tabs_end_iter = get_tabs_end_iter(close_iter);
        if(found_tabs_end_iter.get_line_offset() == tabs_end_iter.get_line_offset()) {
          has_right_curly_bracket = true;
          // Special case for functions and classes with no indentation after: namespace {:
          if(is_cpp && tabs_end_iter.starts_line()) {
            Gtk::TextIter open_iter;
            if(find_open_symbol_backward(iter, open_iter, '{', '}')) {
              if(open_iter.starts_line()) // in case of: namespace test\n{
                open_iter.backward_char();
              auto iter = get_buffer()->get_iter_at_line(open_iter.get_line());
              if(get_token(iter) == "namespace")
                has_right_curly_bracket = close_iter.forward_char() && find_close_symbol_forward(close_iter, close_iter, '{', '}');
            }
          }
          // Inside for example {}:
          else if(found_tabs_end_iter.get_line() == tabs_end_iter.get_line())
            has_right_curly_bracket = symbol_count(iter, '{', '}') < 0;
        }
      }
      if(!has_right_curly_bracket) {
        get_buffer()->insert_at_cursor("}");
        auto iter = get_buffer()->get_insert()->get_iter();
        iter.backward_char();
        get_buffer()->place_cursor(iter);
        scroll_to(get_buffer()->get_insert());
      }
      return false;
    }
    // Move right on } in {}
    else if(event->keyval == GDK_KEY_braceright) {
      if(*iter == '}' && symbol_count(iter, '{', '}') <= 0) {
        iter.forward_char();
        get_buffer()->place_cursor(iter);
        scroll_to(get_buffer()->get_insert());
        return true;
      }
    }
    // Insert ''
    else if(event->keyval == GDK_KEY_apostrophe && allow_insertion(iter) && symbol_count(iter, '\'') % 2 == 0) {
      get_buffer()->insert_at_cursor("''");
      auto iter = get_buffer()->get_insert()->get_iter();
      iter.backward_char();
      get_buffer()->place_cursor(iter);
      scroll_to(get_buffer()->get_insert());
      return true;
    }
    // Insert ""
    else if(event->keyval == GDK_KEY_quotedbl && allow_insertion(iter) && symbol_count(iter, '"') % 2 == 0) {
      get_buffer()->insert_at_cursor("\"\"");
      auto iter = get_buffer()->get_insert()->get_iter();
      iter.backward_char();
      get_buffer()->place_cursor(iter);
      scroll_to(get_buffer()->get_insert());
      return true;
    }
    // Move right on last ' in '', or last " in ""
    else if(((event->keyval == GDK_KEY_apostrophe && *iter == '\'') || (event->keyval == GDK_KEY_quotedbl && *iter == '\"')) && is_spellcheck_iter(iter)) {
      get_buffer()->place_cursor(next_iter);
      scroll_to(get_buffer()->get_insert());
      return true;
    }
    // Insert ; at the end of line, if iter is at the last )
    else if(event->keyval == GDK_KEY_semicolon) {
      if(*iter == ')' && symbol_count(iter, '(', ')') <= 0 && next_iter.ends_line()) {
        auto start_iter = get_start_of_expression(previous_iter);
        if(*start_iter == '(') {
          start_iter.backward_char();
          if(*start_iter == ' ')
            start_iter.backward_char();
          auto token = get_token(start_iter);
          if(token != "for" && token != "if" && token != "while" && token != "else") {
            iter.forward_char();
            get_buffer()->place_cursor(iter);
            get_buffer()->insert_at_cursor(";");
            scroll_to(get_buffer()->get_insert());
            return true;
          }
        }
      }
    }
    // Delete ()
    else if(event->keyval == GDK_KEY_BackSpace) {
      if(*previous_iter == '(' && *iter == ')' && symbol_count(iter, '(', ')') <= 0) {
        auto next_iter = iter;
        next_iter.forward_char();
        get_buffer()->erase(previous_iter, next_iter);
        scroll_to(get_buffer()->get_insert());
        return true;
      }
      // Delete []
      else if(*previous_iter == '[' && *iter == ']' && symbol_count(iter, '[', ']') <= 0) {
        auto next_iter = iter;
        next_iter.forward_char();
        get_buffer()->erase(previous_iter, next_iter);
        scroll_to(get_buffer()->get_insert());
        return true;
      }
      // Delete {}
      else if(*previous_iter == '{' && *iter == '}' && symbol_count(iter, '{', '}') <= 0) {
        auto next_iter = iter;
        next_iter.forward_char();
        get_buffer()->erase(previous_iter, next_iter);
        scroll_to(get_buffer()->get_insert());
        return true;
      }
      // Delete '' or ""
      else if(event->keyval == GDK_KEY_BackSpace) {
        if((*previous_iter == '\'' && *iter == '\'') || (*previous_iter == '"' && *iter == '"')) {
          get_buffer()->erase(previous_iter, next_iter);
          scroll_to(get_buffer()->get_insert());
          return true;
        }
      }
    }
  }

  return false;
}

bool Source::View::on_button_press_event(GdkEventButton *event) {
  // Select range when double clicking
  if(event->type == GDK_2BUTTON_PRESS) {
    Gtk::TextIter start, end;
    get_buffer()->get_selection_bounds(start, end);
    auto iter = start;
    while((*iter >= 48 && *iter <= 57) || (*iter >= 65 && *iter <= 90) || (*iter >= 97 && *iter <= 122) || *iter == 95) {
      start = iter;
      if(!iter.backward_char())
        break;
    }
    while((*end >= 48 && *end <= 57) || (*end >= 65 && *end <= 90) || (*end >= 97 && *end <= 122) || *end == 95) {
      if(!end.forward_char())
        break;
    }
    get_buffer()->select_range(start, end);
    return true;
  }

  // Go to implementation or declaration
  if((event->type == GDK_BUTTON_PRESS) && (event->button == 1)) {
    if(event->state & primary_modifier_mask) {
      int x, y;
      window_to_buffer_coords(Gtk::TextWindowType::TEXT_WINDOW_TEXT, event->x, event->y, x, y);
      Gtk::TextIter iter;
      get_iter_at_location(iter, x, y);
      if(iter)
        get_buffer()->place_cursor(iter);

      Menu::get().actions["source_goto_declaration_or_implementation"]->activate();
      return true;
    }
  }

  // Open right click menu
  if((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
    hide_tooltips();
    if(!get_buffer()->get_has_selection()) {
      int x, y;
      window_to_buffer_coords(Gtk::TextWindowType::TEXT_WINDOW_TEXT, event->x, event->y, x, y);
      Gtk::TextIter iter;
      get_iter_at_location(iter, x, y);
      if(iter)
        get_buffer()->place_cursor(iter);
      Menu::get().right_click_line_menu->popup(event->button, event->time);
    }
    else {
      Menu::get().right_click_selected_menu->popup(event->button, event->time);
    }
    return true;
  }
  return Gsv::View::on_button_press_event(event);
}
