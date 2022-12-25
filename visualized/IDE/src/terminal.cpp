#include "terminal.hpp"
#include "config.hpp"
#include "filesystem.hpp"
#include "info.hpp"
#include "notebook.hpp"
#include "project.hpp"
#include "utility.hpp"
#include <future>
#include <iostream>
#include <regex>
#include <thread>

Terminal::Terminal() : Source::CommonView() {
  get_style_context()->add_class("juci_terminal");

  set_editable(false);

  bold_tag = get_buffer()->create_tag();
  bold_tag->property_weight() = Pango::WEIGHT_ULTRAHEAVY;

  red_tag = get_buffer()->create_tag();
  green_tag = get_buffer()->create_tag();
  yellow_tag = get_buffer()->create_tag();
  blue_tag = get_buffer()->create_tag();
  magenta_tag = get_buffer()->create_tag();
  cyan_tag = get_buffer()->create_tag();
  gray_tag = get_buffer()->create_tag();

  link_tag = get_buffer()->create_tag();
  link_tag->property_underline() = Pango::Underline::UNDERLINE_SINGLE;

  invisible_tag = get_buffer()->create_tag();
  invisible_tag->property_invisible() = true;

  link_mouse_cursor = Gdk::Cursor::create(Gdk::CursorType::HAND1);
  default_mouse_cursor = Gdk::Cursor::create(Gdk::CursorType::XTERM);

  class DetectPossibleLink {
    bool delimiter_found = false, dot_found = false;

  public:
    bool operator()(char chr) {
      if(chr == '\n') {
        auto all_found = delimiter_found && dot_found;
        delimiter_found = dot_found = false;
        return all_found;
      }
      else if(chr == '/' || chr == '\\')
        delimiter_found = true;
      else if(chr == '.')
        dot_found = true;
      return false;
    }
  };
  class ParseAnsiEscapeSequence {
    enum class State {
      none = 1,
      escaped,
      parameter_bytes,
      intermediate_bytes
    };
    State state = State::none;
    std::string parameters;
    size_t length = 0;

  public:
    struct Sequence {
      std::string arguments;
      size_t length;
      char command;
    };

    boost::optional<Sequence> operator()(char chr) {
      if(chr == '\e') {
        state = State::escaped;
        parameters = {};
        length = 1;
      }
      else if(state != State::none) {
        ++length;
        if(chr == '[') {
          if(state == State::escaped)
            state = State::parameter_bytes;
          else
            state = State::none;
        }
        else if(chr >= 0x30 && chr <= 0x3f) {
          if(state == State::parameter_bytes)
            parameters += chr;
          else
            state = State::none;
        }
        else if(chr >= 0x20 && chr <= 0x2f) {
          if(state == State::parameter_bytes)
            state = State::intermediate_bytes;
          else if(state != State::intermediate_bytes)
            state = State::none;
        }
        else if(chr >= 0x40 && chr <= 0x7e) {
          if(state == State::parameter_bytes || state == State::intermediate_bytes) {
            state = State::none;
            return Sequence{std::move(parameters), length, chr};
          }
          else
            state = State::none;
        }
        else
          state = State::none;
      }
      return {};
    }
  };
  get_buffer()->signal_insert().connect([this, detect_possible_link = DetectPossibleLink(), parse_ansi_escape_sequence = ParseAnsiEscapeSequence(), last_color = -1, last_color_sequence_mark = std::shared_ptr<Source::Mark>()](const Gtk::TextIter &iter, const Glib::ustring &text_, int /*bytes*/) mutable {
    boost::optional<Gtk::TextIter> start_of_text;
    int line_nr_offset = 0;
    auto get_line_nr = [&] {
      if(!start_of_text) {
        start_of_text = iter;
        start_of_text->backward_chars(text_.size());
      }
      return start_of_text->get_line() + line_nr_offset;
    };
    const auto &text = text_.raw();
    for(size_t i = 0; i < text.size(); ++i) {
      if(detect_possible_link(text[i])) {
        auto start = get_buffer()->get_iter_at_line(get_line_nr());
        auto end = start;
        if(!end.ends_line())
          end.forward_to_line_end();
        if(auto link = find_link(get_buffer()->get_text(start, end, false).raw())) { // Apply link tags
          auto link_start = start;
          if(link_start.has_tag(invisible_tag))
            link_start.forward_visible_cursor_position();
          auto link_end = link_start;
          link_start.forward_visible_cursor_positions(link->start_pos);
          link_end.forward_visible_cursor_positions(link->end_pos);
          get_buffer()->apply_tag(link_tag, link_start, link_end);
        }
      }
      if(auto sequence = parse_ansi_escape_sequence(text[i])) {
        auto end = iter;
        end.backward_chars(utf8_character_count(text, i + 1));
        auto start = end;
        start.backward_chars(sequence->length);
        get_buffer()->apply_tag(invisible_tag, start, end);
        if(sequence->command == 'm') {
          int color = -1;
          if(sequence->arguments.empty())
            color = 0;
          else {
            size_t pos = 0;
            size_t start_pos = pos;
            while(true) {
              pos = sequence->arguments.find(";", pos);
              try {
                auto code = std::stoi(sequence->arguments.substr(start_pos, pos != std::string::npos ? pos - start_pos : pos));
                if(code == 39)
                  color = 0;
                else if(code == 38) {
                  color = 0;
                  break; // Do not read next arguments
                }
                else if(code == 48 || code == 58)
                  break; // Do not read next arguments
                else if(code == 0 || code == 2 || code == 22 || (code >= 30 && code <= 37))
                  color = code;
              }
              catch(...) {
              }
              if(pos == std::string::npos)
                break;
              pos += 1;
              start_pos = pos;
            }
          }
          if(last_color >= 0) {
            if(last_color == 31)
              get_buffer()->apply_tag(red_tag, (*last_color_sequence_mark)->get_iter(), start);
            else if(last_color == 32)
              get_buffer()->apply_tag(green_tag, (*last_color_sequence_mark)->get_iter(), start);
            else if(last_color == 33)
              get_buffer()->apply_tag(yellow_tag, (*last_color_sequence_mark)->get_iter(), start);
            else if(last_color == 34)
              get_buffer()->apply_tag(blue_tag, (*last_color_sequence_mark)->get_iter(), start);
            else if(last_color == 35)
              get_buffer()->apply_tag(magenta_tag, (*last_color_sequence_mark)->get_iter(), start);
            else if(last_color == 36)
              get_buffer()->apply_tag(cyan_tag, (*last_color_sequence_mark)->get_iter(), start);
            else if(last_color == 37 || last_color == 2)
              get_buffer()->apply_tag(gray_tag, (*last_color_sequence_mark)->get_iter(), start);
          }

          if(color >= 0) {
            last_color = color;
            last_color_sequence_mark = std::make_shared<Source::Mark>(end);
          }
        }
      }
      if(text[i] == '\n')
        ++line_nr_offset;
    }
  });
}

int Terminal::process(const std::string &command, const boost::filesystem::path &path, bool use_pipes) {
  std::unique_ptr<TinyProcessLib::Process> process;
  if(use_pipes)
    process = std::make_unique<TinyProcessLib::Process>(
        command, path.string(),
        [this](const char *bytes, size_t n) {
          async_print(std::string(bytes, n));
        },
        [this](const char *bytes, size_t n) {
          async_print(std::string(bytes, n), true);
        });
  else
    process = std::make_unique<TinyProcessLib::Process>(command, path.string());

  if(process->get_id() <= 0) {
    async_print("\e[31mError\e[m: failed to run command: " + command + "\n", true);
    return -1;
  }

  return process->get_exit_status();
}

int Terminal::process(std::istream &stdin_stream, std::ostream &stdout_stream, const std::string &command, const boost::filesystem::path &path, std::ostream *stderr_stream) {
  TinyProcessLib::Process process(
      command, path.string(),
      [&stdout_stream](const char *bytes, size_t n) {
        Glib::ustring umessage(std::string(bytes, n));
        Glib::ustring::iterator iter;
        while(!umessage.validate(iter)) {
          auto next_char_iter = iter;
          next_char_iter++;
          umessage.replace(iter, next_char_iter, "?");
        }
        stdout_stream.write(umessage.data(), n);
      },
      [this, stderr_stream](const char *bytes, size_t n) {
        if(stderr_stream)
          stderr_stream->write(bytes, n);
        else
          async_print(std::string(bytes, n), true);
      },
      true);

  if(process.get_id() <= 0) {
    async_print("\e[31mError\e[m: failed to run command: " + command + "\n", true);
    return -1;
  }

  char buffer[131072];
  for(;;) {
    stdin_stream.readsome(buffer, 131072);
    auto read_n = stdin_stream.gcount();
    if(read_n == 0)
      break;
    if(!process.write(buffer, read_n)) {
      break;
    }
  }
  process.close_stdin();

  return process.get_exit_status();
}

std::shared_ptr<TinyProcessLib::Process> Terminal::async_process(const std::string &command, const boost::filesystem::path &path, std::function<void(int exit_status)> callback, bool quiet) {
  if(scroll_to_bottom)
    scroll_to_bottom();
  stdin_buffer.clear();

#if !defined(__APPLE__) && !defined(_WIN32)
  static auto stdbuf = filesystem::find_executable("stdbuf").string();

  std::string cd_path_and_command;
  if(!path.empty()) {
    auto path_escaped = path.string();
    size_t pos = 0;
    // Based on https://www.reddit.com/r/cpp/comments/3vpjqg/a_new_platform_independent_process_library_for_c11/cxsxyb7
    while((pos = path_escaped.find('\'', pos)) != std::string::npos) {
      path_escaped.replace(pos, 1, "'\\''");
      pos += 4;
    }
    cd_path_and_command = "cd '" + path_escaped + "' && " + command; // To avoid resolving symbolic links
  }
  else
    cd_path_and_command = command;
#endif

  auto process = std::make_shared<TinyProcessLib::Process>(
#if defined(__APPLE__)
      "STDBUF1=L " + command,
      path.string(),
#elif defined(_WIN32)
      command,
      path.string(),
#else
      !stdbuf.empty() ? std::vector<std::string>{stdbuf, "-oL", "/bin/sh", "-c", cd_path_and_command} : std::vector<std::string>{"/bin/sh", "-c", cd_path_and_command},
      "",
#endif
      [this, quiet](const char *bytes, size_t n) {
        if(!quiet) {
          // Print stdout message sequentially to avoid the GUI becoming unresponsive
          std::promise<void> message_printed;
          dispatcher.post([message = std::string(bytes, n), &message_printed]() mutable {
            Terminal::get().print(std::move(message));
            message_printed.set_value();
          });
          message_printed.get_future().get();
        }
      },
      [this, quiet](const char *bytes, size_t n) {
        if(!quiet) {
          // Print stderr message sequentially to avoid the GUI becoming unresponsive
          std::promise<void> message_printed;
          dispatcher.post([message = std::string(bytes, n), &message_printed]() mutable {
            Terminal::get().print(std::move(message), true);
            message_printed.set_value();
          });
          message_printed.get_future().get();
        }
      },
      true);

  auto pid = process->get_id();
  if(pid <= 0) {
    async_print("\e[31mError\e[m: failed to run command: " + command + "\n", true);
    if(callback)
      callback(-1);
    return process;
  }
  else {
    LockGuard lock(processes_mutex);
    processes.emplace_back(process);
  }

  std::thread([this, process, pid, callback = std::move(callback)]() mutable {
    auto exit_status = process->get_exit_status();
    {
      LockGuard lock(processes_mutex);
      for(auto it = processes.begin(); it != processes.end(); it++) {
        if((*it)->get_id() == pid) {
          processes.erase(it);
          break;
        }
      }
    }
    if(callback) {
      dispatcher.post([callback = std::move(callback), exit_status] {
        callback(exit_status);
      });
    }
  }).detach();

  return process;
}

void Terminal::kill_last_async_process(bool force) {
  LockGuard lock(processes_mutex);
  if(processes.empty())
    Info::get().print("No running processes");
  else
    processes.back()->kill(force);
}

void Terminal::kill_async_processes(bool force) {
  LockGuard lock(processes_mutex);
  for(auto &process : processes)
    process->kill(force);
}

bool Terminal::on_motion_notify_event(GdkEventMotion *event) {
  Gtk::TextIter iter;
  int location_x, location_y;
  window_to_buffer_coords(Gtk::TextWindowType::TEXT_WINDOW_TEXT, event->x, event->y, location_x, location_y);
  get_iter_at_location(iter, location_x, location_y);
  if(iter.has_tag(link_tag))
    get_window(Gtk::TextWindowType::TEXT_WINDOW_TEXT)->set_cursor(link_mouse_cursor);
  else
    get_window(Gtk::TextWindowType::TEXT_WINDOW_TEXT)->set_cursor(default_mouse_cursor);

  return Source::CommonView::on_motion_notify_event(event);
}

boost::optional<Terminal::Link> Terminal::find_link(const std::string &line) {
  if(line.size() >= 1000) // Due to https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86164
    return {};

  const static std::regex link_regex("^([A-Z]:)?([^:]+):([0-9]+):([0-9]+): .*$|"                                      // C/C++ compile warning/error/rename usages
                                     "^In file included from ([A-Z]:)?([^:]+):([0-9]+)[:,]$|"                         // C/C++ extra compile warning/error info
                                     "^                 from ([A-Z]:)?([^:]+):([0-9]+)[:,]$|"                         // C/C++ extra compile warning/error info (gcc)
                                     "^ +--> ([A-Z]:)?([^:]+):([0-9]+):([0-9]+)$|"                                    // Rust
                                     "^Assertion failed: .*file ([A-Z]:)?([^:]+), line ([0-9]+)\\.$|"                 // clang assert()
                                     "^[^:]*: ([A-Z]:)?([^:]+):([0-9]+): .* Assertion .* failed\\.$|"                 // gcc assert()
                                     "^ERROR:([A-Z]:)?([^:]+):([0-9]+):.*$|"                                          // g_assert (glib.h)
                                     "^([A-Z]:)?([\\\\/][^:]+):([0-9]+)$|"                                            // Node.js
                                     "^ +at .*?\\(([A-Z]:)?([^:]+):([0-9]+):([0-9]+)\\).*$|"                          // Node.js stack trace
                                     "^ +at ([A-Z]:)?([^:]+):([0-9]+):([0-9]+).*$|"                                   // Node.js stack trace
                                     "^  File \"([A-Z]:)?([^\"]+)\", line ([0-9]+), in .*$|"                          // Python
                                     "^.*?([A-Z]:)?([a-zA-Z0-9._\\\\/~][a-zA-Z0-9._\\-\\\\/]*):([0-9]+):([0-9]+).*$", // Posix path:line:column
                                     std::regex::optimize);
  std::smatch sm;
  if(std::regex_match(line, sm, link_regex)) {
    for(size_t sub = 1; sub < link_regex.mark_count();) {
      size_t subs = (sub == 1 ||
                     sub == 1 + 4 + 3 + 3 ||
                     sub == 1 + 4 + 3 + 3 + 4 + 3 + 3 + 3 + 3 ||
                     sub == 1 + 4 + 3 + 3 + 4 + 3 + 3 + 3 + 3 + 4 ||
                     sub == 1 + 4 + 3 + 3 + 4 + 3 + 3 + 3 + 3 + 4 + 4 + 3)
                        ? 4
                        : 3;
      if(sm.length(sub + 1)) {
        auto start_pos = static_cast<int>(sm.position(sub + 1) - sm.length(sub));
        auto end_pos = static_cast<int>(sm.position(sub + subs - 1) + sm.length(sub + subs - 1));
        int start_pos_utf8 = utf8_character_count(line, 0, start_pos);
        int end_pos_utf8 = start_pos_utf8 + utf8_character_count(line, start_pos, end_pos - start_pos);
        std::string path;
        if(sm.length(sub))
          path = sm[sub].str();
        path += sm[sub + 1].str();
        try {
          auto line_number = std::stoi(sm[sub + 2].str());
          auto line_offset = std::stoi(subs == 4 ? sm[sub + 3].str() : "1");
          return Link{start_pos_utf8, end_pos_utf8, path, line_number, line_offset};
        }
        catch(...) {
          return {};
        }
      }
      sub += subs;
    }
  }
  const static std::regex uri_regex("^.*(https?://[\\w\\-.~:/?#%\\[\\]@!$&'()*+,;=]+[\\w\\-~/#@$*+;=]).*$", std::regex::optimize);
  if(std::regex_match(line, sm, uri_regex)) {
    auto start_pos = static_cast<int>(sm.position(1));
    auto end_pos = static_cast<int>(start_pos + sm.length(1));
    int start_pos_utf8 = utf8_character_count(line, 0, start_pos);
    int end_pos_utf8 = start_pos_utf8 + utf8_character_count(line, start_pos, end_pos - start_pos);
    return Link{start_pos_utf8, end_pos_utf8, sm[1].str(), 0, 0};
  }
  return {};
}

void Terminal::print(std::string message, bool bold) {
  if(message.empty())
    return;

  if(auto parent = get_parent()) {
    if(!parent->is_visible())
      parent->show();
  }

  Glib::ustring umessage = std::move(message);

  Glib::ustring::iterator iter;
  while(!umessage.validate(iter)) {
    auto next_char_iter = iter;
    next_char_iter++;
    umessage.replace(iter, next_char_iter, "?");
  }

  if(bold)
    get_buffer()->insert_with_tag(get_buffer()->end(), umessage, bold_tag);
  else
    get_buffer()->insert(get_buffer()->end(), umessage);

  auto excess_lines = get_buffer()->get_line_count() - Config::get().terminal.history_size;
  if(excess_lines > 0)
    get_buffer()->erase(get_buffer()->begin(), get_buffer()->get_iter_at_line(excess_lines));
}

void Terminal::async_print(std::string message, bool bold) {
  dispatcher.post([message = std::move(message), bold]() mutable {
    Terminal::get().print(std::move(message), bold);
  });
}

void Terminal::configure() {
  link_tag->property_foreground_rgba() = get_style_context()->get_color(Gtk::StateFlags::STATE_FLAG_LINK);

  auto normal_color = get_style_context()->get_color(Gtk::StateFlags::STATE_FLAG_NORMAL);
  auto light_theme = (normal_color.get_red() + normal_color.get_green() + normal_color.get_blue()) / 3 < 0.5;

  Gdk::RGBA rgba;
  rgba.set_rgba(1.0, 0.0, 0.0);
  double factor = light_theme ? 0.5 : 0.35;
  rgba.set_red(normal_color.get_red() + factor * (rgba.get_red() - normal_color.get_red()));
  rgba.set_green(normal_color.get_green() + factor * (rgba.get_green() - normal_color.get_green()));
  rgba.set_blue(normal_color.get_blue() + factor * (rgba.get_blue() - normal_color.get_blue()));
  red_tag->property_foreground_rgba() = rgba;

  rgba.set_rgba(0.0, 1.0, 0.0);
  factor = 0.4;
  rgba.set_red(normal_color.get_red() + factor * (rgba.get_red() - normal_color.get_red()));
  rgba.set_green(normal_color.get_green() + factor * (rgba.get_green() - normal_color.get_green()));
  rgba.set_blue(normal_color.get_blue() + factor * (rgba.get_blue() - normal_color.get_blue()));
  green_tag->property_foreground_rgba() = rgba;

  rgba.set_rgba(1.0, 1.0, 0.2);
  factor = 0.5;
  rgba.set_red(normal_color.get_red() + factor * (rgba.get_red() - normal_color.get_red()));
  rgba.set_green(normal_color.get_green() + factor * (rgba.get_green() - normal_color.get_green()));
  rgba.set_blue(normal_color.get_blue() + factor * (rgba.get_blue() - normal_color.get_blue()));
  yellow_tag->property_foreground_rgba() = rgba;

  rgba.set_rgba(0.0, 0.0, 1.0);
  factor = light_theme ? 0.8 : 0.2;
  rgba.set_red(normal_color.get_red() + factor * (rgba.get_red() - normal_color.get_red()));
  rgba.set_green(normal_color.get_green() + factor * (rgba.get_green() - normal_color.get_green()));
  rgba.set_blue(normal_color.get_blue() + factor * (rgba.get_blue() - normal_color.get_blue()));
  blue_tag->property_foreground_rgba() = rgba;

  rgba.set_rgba(1.0, 0.0, 1.0);
  factor = light_theme ? 0.45 : 0.25;
  rgba.set_red(normal_color.get_red() + factor * (rgba.get_red() - normal_color.get_red()));
  rgba.set_green(normal_color.get_green() + factor * (rgba.get_green() - normal_color.get_green()));
  rgba.set_blue(normal_color.get_blue() + factor * (rgba.get_blue() - normal_color.get_blue()));
  magenta_tag->property_foreground_rgba() = rgba;

  rgba.set_rgba(0.0, 1.0, 1.0);
  factor = light_theme ? 0.35 : 0.35;
  rgba.set_red(normal_color.get_red() + factor * (rgba.get_red() - normal_color.get_red()));
  rgba.set_green(normal_color.get_green() + factor * (rgba.get_green() - normal_color.get_green()));
  rgba.set_blue(normal_color.get_blue() + factor * (rgba.get_blue() - normal_color.get_blue()));
  cyan_tag->property_foreground_rgba() = rgba;

  rgba.set_rgba(0.5, 0.5, 0.5);
  factor = light_theme ? 0.6 : 0.4;
  rgba.set_red(normal_color.get_red() + factor * (rgba.get_red() - normal_color.get_red()));
  rgba.set_green(normal_color.get_green() + factor * (rgba.get_green() - normal_color.get_green()));
  rgba.set_blue(normal_color.get_blue() + factor * (rgba.get_blue() - normal_color.get_blue()));
  gray_tag->property_foreground_rgba() = rgba;

  // Set search match style:
  get_buffer()->get_tag_table()->foreach([](const Glib::RefPtr<Gtk::TextTag> &tag) {
    if(tag->property_background_set()) {
      auto scheme = Source::StyleSchemeManager::get_default()->get_scheme(Config::get().source.style);
      if(scheme) {
        auto style = scheme->get_style("search-match");
        if(style) {
          if(style->property_background_set())
            tag->property_background() = style->property_background();
          if(style->property_foreground_set())
            tag->property_foreground() = style->property_foreground();
        }
      }
    }
  });
}

void Terminal::clear() {
  get_buffer()->set_text("");
}

bool Terminal::on_button_press_event(GdkEventButton *button_event) {
  // open clicked link in terminal
  if(button_event->type == GDK_BUTTON_PRESS && button_event->button == GDK_BUTTON_PRIMARY) {
    Gtk::TextIter iter;
    int location_x, location_y;
    window_to_buffer_coords(Gtk::TextWindowType::TEXT_WINDOW_TEXT, button_event->x, button_event->y, location_x, location_y);
    get_iter_at_location(iter, location_x, location_y);
    if(iter.has_tag(link_tag)) {
      auto start = get_buffer()->get_iter_at_line(iter.get_line());
      auto end = start;
      if(!end.ends_line())
        end.forward_to_line_end();
      if(auto link = find_link(get_buffer()->get_text(start, end, false).raw())) {
        if(starts_with(link->path, "http://") || starts_with(link->path, "https://")) {
          Notebook::get().open_uri(link->path);
          return true;
        }

        auto path = filesystem::get_long_path(link->path);

        if(path.is_relative()) {
          auto project = Project::current;
          if(!project)
            project = Project::create();
          if(project) {
            boost::system::error_code ec;
            if(boost::filesystem::exists(project->build->get_default_path() / path, ec))
              path = project->build->get_default_path() / path;
            else if(boost::filesystem::exists(project->build->get_debug_path() / path, ec))
              path = project->build->get_debug_path() / path;
            else if(boost::filesystem::exists(project->build->project_path / path, ec))
              path = project->build->project_path / path;
            else
              return Gtk::TextView::on_button_press_event(button_event);
          }
          else
            return Gtk::TextView::on_button_press_event(button_event);
        }
        boost::system::error_code ec;
        if(boost::filesystem::is_regular_file(path, ec)) {
          if(Notebook::get().open(path)) {
            auto view = Notebook::get().get_current_view();
            view->place_cursor_at_line_index(link->line - 1, link->line_index - 1);
            view->scroll_to_cursor_delayed(true, true);
            return true;
          }
        }
      }
    }
  }
  return Gtk::TextView::on_button_press_event(button_event);
}

bool Terminal::on_key_press_event(GdkEventKey *event) {
  if(event->keyval == GDK_KEY_Home || event->keyval == GDK_KEY_End ||
     event->keyval == GDK_KEY_Page_Up || event->keyval == GDK_KEY_Page_Down ||
     event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_Down ||
     event->keyval == GDK_KEY_Left || event->keyval == GDK_KEY_Right)
    return Source::CommonView::on_key_press_event(event);

  LockGuard lock(processes_mutex);
  bool debug_is_running = false;
#ifdef JUCI_ENABLE_DEBUG
  debug_is_running = Project::current ? Project::current->debug_is_running() : false;
#endif
  if(processes.size() > 0 || debug_is_running) {
    auto unicode = gdk_keyval_to_unicode(event->keyval);
    if(unicode >= 32 && unicode != 126 && unicode != 0) {
      if(scroll_to_bottom)
        scroll_to_bottom();
      get_buffer()->place_cursor(get_buffer()->end());
      stdin_buffer += unicode;
      get_buffer()->insert_at_cursor(Glib::ustring() + unicode);
    }
    else if(event->keyval == GDK_KEY_BackSpace) {
      if(scroll_to_bottom)
        scroll_to_bottom();
      get_buffer()->place_cursor(get_buffer()->end());
      if(stdin_buffer.size() > 0 && get_buffer()->get_char_count() > 0) {
        auto iter = get_buffer()->end();
        iter.backward_char();
        stdin_buffer.erase(stdin_buffer.size() - 1);
        get_buffer()->erase(iter, get_buffer()->end());
      }
    }
    else if(event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) {
      if(scroll_to_bottom)
        scroll_to_bottom();
      get_buffer()->place_cursor(get_buffer()->end());
      stdin_buffer += '\n';
      get_buffer()->insert_at_cursor("\n");
      if(debug_is_running) {
#ifdef JUCI_ENABLE_DEBUG
        Project::current->debug_write(stdin_buffer.raw());
#endif
      }
      else
        processes.back()->write(stdin_buffer.raw());
      stdin_buffer.clear();
    }
  }
  return true;
}

void Terminal::paste() {
  std::string text = Gtk::Clipboard::get()->wait_for_text();
  if(text.empty())
    return;

  // Replace carriage returns (which leads to crash) with newlines
  for(size_t c = 0; c < text.size(); c++) {
    if(text[c] == '\r') {
      if((c + 1) < text.size() && text[c + 1] == '\n')
        text.replace(c, 2, "\n");
      else
        text.replace(c, 1, "\n");
    }
  }

  std::string after_last_newline_str;
  auto last_newline = text.rfind('\n');

  LockGuard lock(processes_mutex);
  bool debug_is_running = false;
#ifdef JUCI_ENABLE_DEBUG
  debug_is_running = Project::current ? Project::current->debug_is_running() : false;
#endif
  if(processes.size() > 0 || debug_is_running) {
    if(scroll_to_bottom)
      scroll_to_bottom();
    get_buffer()->place_cursor(get_buffer()->end());
    get_buffer()->insert_at_cursor(text);
    if(last_newline != std::string::npos) {
      if(debug_is_running) {
#ifdef JUCI_ENABLE_DEBUG
        Project::current->debug_write(stdin_buffer.raw() + text.substr(0, last_newline + 1));
#endif
      }
      else
        processes.back()->write(stdin_buffer.raw() + text.substr(0, last_newline + 1));
      stdin_buffer = text.substr(last_newline + 1);
    }
    else
      stdin_buffer += text;
  }
}
