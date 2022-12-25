#include "notebook.hpp"
#include "config.hpp"
#include "dialog.hpp"
#include "directories.hpp"
#include "filesystem.hpp"
#include "project.hpp"
#include "selection_dialog.hpp"
#include "source_clang.hpp"
#include "source_generic.hpp"
#include "source_language_protocol.hpp"
#include "utility.hpp"
#include <fstream>
#include <gtksourceview-3.0/gtksourceview/gtksourcemap.h>
#include <regex>


Notebook::TabLabel::TabLabel(const std::function<void()> &on_close) {
  set_can_focus(false);

  auto button = Gtk::manage(new Gtk::Button());
  auto hbox = Gtk::manage(new Gtk::Box());

  hbox->set_can_focus(false);
  label.set_can_focus(false);
  button->set_image_from_icon_name("window-close-symbolic", Gtk::ICON_SIZE_MENU);
  button->set_can_focus(false);
  button->set_relief(Gtk::ReliefStyle::RELIEF_NONE);

  hbox->pack_start(label, Gtk::PACK_SHRINK);
  hbox->pack_end(*button, Gtk::PACK_SHRINK);
  add(*hbox);

  button->signal_clicked().connect(on_close);
  signal_button_press_event().connect([on_close](GdkEventButton *event) {
    if(event->button == GDK_BUTTON_MIDDLE) {
      on_close();
      return true;
    }
    return false;
  });

  show_all();
}

Notebook::Notebook() : Gtk::Paned(), notebooks(2) {
  for(auto &notebook : notebooks) {
    notebook.get_style_context()->add_class("juci_notebook");
    notebook.set_scrollable();
    notebook.set_group_name("source_notebooks");
    notebook.signal_switch_page().connect([this](Gtk::Widget *widget, guint) {
      auto hbox = dynamic_cast<Gtk::Box *>(widget);
      for(size_t c = 0; c < hboxes.size(); ++c) {
        if(hboxes[c].get() == hbox) {
          focus_view(source_views[c]);
          set_current_view(source_views[c]);
          break;
        }
      }
      last_index.reset();
    });
    notebook.signal_page_added().connect([this](Gtk::Widget *widget, guint) {
      auto hbox = dynamic_cast<Gtk::Box *>(widget);
      for(size_t c = 0; c < hboxes.size(); ++c) {
        if(hboxes[c].get() == hbox) {
          focus_view(source_views[c]);
          set_current_view(source_views[c]);
          break;
        }
      }
    });
  }
  pack1(notebooks[0], true, true);

  if(filesystem::find_executable("rustup").empty()) {
    // PATH might not be set (for instance after installation)
#ifdef _WIN32
    boost::filesystem::path cargo_bin;
    if(auto userprofile = std::getenv("USERPROFILE")) // rustup uses this environment variable on MSYS2 as well
      cargo_bin = boost::filesystem::path(userprofile) / ".cargo" / "bin";
    const char delimiter = ';';
#else
    auto cargo_bin = filesystem::get_home_path() / ".cargo" / "bin";
    const char delimiter = ':';
#endif
    boost::system::error_code ec;
    if(!cargo_bin.empty() && boost::filesystem::is_directory(cargo_bin, ec)) {
      std::string env;
      if(auto c_env = std::getenv("PATH"))
        env = c_env;
      Glib::setenv("PATH", !env.empty() ? env + delimiter + cargo_bin.string() : cargo_bin.string());
      filesystem::rust_sysroot_path = {};
      filesystem::rust_nightly_sysroot_path = {};
      filesystem::executable_search_paths = {};
    }
  }
}

size_t Notebook::size() {
  return source_views.size();
}

Source::View *Notebook::get_view(size_t index) {
  if(index >= size())
    return nullptr;
  return source_views[index];
}

Source::View *Notebook::get_current_view() {
  if(intermediate_view) {
    for(auto view : source_views) {
      if(view == intermediate_view)
        return view;
    }
  }
  for(auto view : source_views) {
    if(view == current_view)
      return view;
  }
  // In case there exist a tab that has not yet received focus again in a different notebook
  for(int notebook_index = 0; notebook_index < 2; ++notebook_index) {
    auto page = notebooks[notebook_index].get_current_page();
    if(page >= 0)
      return get_view(notebook_index, page);
  }
  return nullptr;
}

std::vector<Source::View *> &Notebook::get_views() {
  return source_views;
}

bool Notebook::open(Source::View *view) {
  for(size_t c = 0; c < size(); c++) {
    if(view == source_views[c]) {
      auto notebook_page = get_notebook_page(c);
      notebooks[notebook_page.first].set_current_page(notebook_page.second);
      focus_view(source_views[c]);
      return true;
    }
  }
  return false;
}

bool Notebook::open(const boost::filesystem::path &file_path_, Position position) {
  boost::system::error_code ec;
  if(file_path_.empty() || (boost::filesystem::exists(file_path_, ec) && !boost::filesystem::is_regular_file(file_path_, ec))) {
    Terminal::get().print("\e[31mError\e[m: could not open " + filesystem::get_short_path(file_path_).string() + "\n", true);
    return false;
  }

  auto file_path = filesystem::get_normal_path(file_path_);

  if((position == Position::right || position == Position::split) && !split)
    toggle_split();

  // Use canonical path to follow symbolic links
  if(position == Position::infer) {
    auto canonical_file_path = filesystem::get_canonical_path(file_path);
    for(size_t c = 0; c < size(); c++) {
      bool equal;
      {
        LockGuard lock(source_views[c]->canonical_file_path_mutex);
        equal = canonical_file_path == source_views[c]->canonical_file_path;
      }
      if(equal) {
        auto notebook_page = get_notebook_page(c);
        notebooks[notebook_page.first].set_current_page(notebook_page.second);
        focus_view(source_views[c]);
        return true;
      }
    }
  }

  if(boost::filesystem::exists(file_path, ec)) {
    std::ifstream can_read(file_path.string());
    if(!can_read) {
      Terminal::get().print("\e[31mError\e[m: could not open " + filesystem::get_short_path(file_path).string() + "\n", true);
      return false;
    }
    can_read.close();
  }

  auto last_view = get_current_view();

  auto language = Source::guess_language(file_path);
  std::string language_id = language ? language->get_id() : "";
  std::string language_protocol_language_id = language_id;

  if(language_protocol_language_id == "js") {
    if(file_path.extension() == ".ts")
      language_protocol_language_id = "typescript";
    else if(file_path.extension() == ".tsx")
      language_protocol_language_id = "typescriptreact";
    else
      language_protocol_language_id = "javascript";
  }

  size_t source_views_previous_size = source_views.size();
  if(language_id == "chdr" || language_id == "cpphdr" || language_id == "c" || language_id == "cpp" || language_id == "objc")
    source_views.emplace_back(new Source::ClangView(file_path, language));
  else if(language && !language_protocol_language_id.empty() && !filesystem::find_executable(language_protocol_language_id + "-language-server").empty())
    source_views.emplace_back(new Source::LanguageProtocolView(file_path, language, language_protocol_language_id, filesystem::escape_argument(language_protocol_language_id + "-language-server")));
  else if(language && language_protocol_language_id == "rust") {
    if(!filesystem::find_executable("rust-analyzer").empty())
      source_views.emplace_back(new Source::LanguageProtocolView(file_path, language, language_protocol_language_id, "rust-analyzer"));
    else {
      if(filesystem::find_executable("rustup").empty())
        install_rust();
      if(!filesystem::find_executable("rustup").empty()) {
        // Try find rust-analyzer installed with rustup
        auto sysroot = filesystem::get_rust_sysroot_path();
        if(!sysroot.empty()) {
          auto rust_analyzer = sysroot / "bin" / "rust-analyzer";
          if(filesystem::is_executable(rust_analyzer))
            source_views.emplace_back(new Source::LanguageProtocolView(file_path, language, language_protocol_language_id, filesystem::escape_argument(rust_analyzer.string())));
          else {
            // Workaround while rust-analyzer is in nightly toolchain only
            auto nightly_sysroot = filesystem::get_rust_nightly_sysroot_path();
            if(!nightly_sysroot.empty()) {
              auto nightly_rust_analyzer = nightly_sysroot / "bin" / "rust-analyzer";
              if(filesystem::is_executable(nightly_rust_analyzer))
                source_views.emplace_back(new Source::LanguageProtocolView(file_path, language, language_protocol_language_id, filesystem::escape_argument(nightly_rust_analyzer.string())));
            }
          }
        }
        if(source_views_previous_size == source_views.size()) {
          // Install rust-analyzer
          auto install_rust_analyzer = [this](const std::string &command, bool &show_dialog) {
            Gtk::MessageDialog dialog(*static_cast<Gtk::Window *>(get_toplevel()), "Install Rust language server?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
            Gtk::Image image;
            image.set_from_icon_name("dialog-question", Gtk::BuiltinIconSize::ICON_SIZE_DIALOG);
            dialog.set_image(image);
            dialog.set_default_response(Gtk::RESPONSE_YES);
            dialog.set_secondary_text("Rust language server not found. Would you like to install rust-analyzer?");
            dialog.show_all();
            int result = dialog.run();
            dialog.hide();
            if(result == Gtk::RESPONSE_YES) {
              bool canceled = false;
              Dialog::Message message("Installing rust-analyzer", [&canceled] {
                canceled = true;
              });
              boost::optional<int> exit_status;

              Terminal::get().print("\e[2mRunning: " + command + "\e[m\n");
              auto process = Terminal::get().async_process(command, "", [&exit_status](int exit_status_) {
                exit_status = exit_status_;
              });
              bool killed = false;
              while(!exit_status) {
                if(canceled && !killed) {
                  process->kill();
                  killed = true;
                  show_dialog = true; // Show dialog again
                }
                while(Gtk::Main::events_pending())
                  Gtk::Main::iteration();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
              }
              if(exit_status == 0) {
                Terminal::get().print("\e[32mSuccess\e[m: installed rust-analyzer\n");
                return true;
              }
            }
            return false;
          };
          static bool show_dialog = true;
          if(show_dialog) {
            show_dialog = false;
            std::stringstream stdin_stream, stdout_stream;
            // Workaround while rust-analyzer is called rust-analyzer-preview instead of rust-analyzer
            if(Terminal::get().process(stdin_stream, stdout_stream, "rustup component list") == 0) {
              bool rust_analyzer_in_toolchain = false;
              std::string line;
              while(std::getline(stdout_stream, line)) {
                if(starts_with(line, "rust-analyzer")) {
                  if(install_rust_analyzer(std::string("rustup component add ") + (starts_with(line, "rust-analyzer-preview") ? "rust-analyzer-preview" : "rust-analyzer"), show_dialog))
                    source_views.emplace_back(new Source::LanguageProtocolView(file_path, language, language_protocol_language_id, filesystem::escape_argument((filesystem::get_rust_sysroot_path() / "bin" / "rust-analyzer").string())));
                  rust_analyzer_in_toolchain = true;
                  break;
                }
              }
              // Workaround while rust-analyzer is in nightly toolchain only
              if(!rust_analyzer_in_toolchain && install_rust_analyzer("rustup toolchain install nightly --component rust-analyzer-preview", show_dialog)) {
                filesystem::rust_nightly_sysroot_path = {};
                source_views.emplace_back(new Source::LanguageProtocolView(file_path, language, language_protocol_language_id, filesystem::escape_argument((filesystem::get_rust_nightly_sysroot_path() / "bin" / "rust-analyzer").string())));
              }
            }
          }
        }
      }
    }
  }
  if(source_views_previous_size == source_views.size()) {
    if(!language_id.empty()) {
      static std::set<std::string> shown;
      if(shown.find(language_id) == shown.end()) {
        if(language_id == "js") {
          Terminal::get().print("\e[33mWarning\e[m: could not find JavaScript/TypeScript language server.\n");
          Terminal::get().print("For installation instructions please visit: https://gitlab.com/cppit/jucipp/-/blob/master/docs/language_servers.md#javascripttypescript.\n");
          shown.emplace(language_id);
        }
        else if(language_id == "python") {
          Terminal::get().print("\e[33mWarning\e[m: could not find Python language server.\n");
          Terminal::get().print("For installation instructions please visit: https://gitlab.com/cppit/jucipp/-/blob/master/docs/language_servers.md#python3.\n");
          shown.emplace(language_id);
        }
        else if(language_id == "rust") {
          auto rust_installed = !filesystem::get_rust_sysroot_path().empty();
          Terminal::get().print(std::string("\e[33mWarning\e[m: could not find Rust") + (rust_installed ? " language server" : "") + ".\n");
          Terminal::get().print("For installation instructions please visit: https://gitlab.com/cppit/jucipp/-/blob/master/docs/language_servers.md#rust.\n");
          if(!rust_installed)
            Terminal::get().print("You will need to restart juCi++ after installing Rust.\n");
          shown.emplace(language_id);
        }
        else if(language_id == "go") {
          Terminal::get().print("\e[33mWarning\e[m: could not find Go language server.\n");
          Terminal::get().print("For installation instructions please visit: https://gitlab.com/cppit/jucipp/-/blob/master/docs/language_servers.md#go.\n");
          shown.emplace(language_id);
        }
        else if(language_id == "julia") {
          Terminal::get().print("\e[33mWarning\e[m: could not find Julia language server.\n");
          Terminal::get().print("For installation instructions please visit: https://gitlab.com/cppit/jucipp/-/blob/master/docs/language_servers.md#julia.\n");
          shown.emplace(language_id);
        }
      }
    }
    source_views.emplace_back(new Source::GenericView(file_path, language));
  }

  auto view = source_views.back();

  if(position == Position::split) {
    auto previous_view = get_current_view();
    if(previous_view) {
      view->replace_text(previous_view->get_buffer()->get_text());
      position = get_notebook_page(previous_view).first == 0 ? Position::right : Position::left;
    }
  }

  view->configure();

  view->update_status_location = [this](Source::BaseView *view) {
    if(get_current_view() == view) {
      auto iter = view->get_buffer()->get_insert()->get_iter();
      auto status_location_text = ' ' + std::to_string(iter.get_line() + 1) + ':' + std::to_string(iter.get_line_offset() + 1);

      if(view->get_buffer()->get_has_selection()) {
        Gtk::TextIter start, end;
        view->get_buffer()->get_selection_bounds(start, end);
        if(start > end)
          std::swap(start, end);
        int lines = end.get_line() - start.get_line() + (end.starts_line() ? 0 : 1); // Do not count lines where the end iter is at start of line
        int words = 0;
        bool in_word = false;
        auto iter = start;
        do {
          if(*iter <= ' ')
            in_word = false;
          else if(!in_word) {
            ++words;
            in_word = true;
          }
        } while(iter.forward_char() && iter < end);
        int chars = end.get_offset() - start.get_offset();
        status_location_text += " (" + std::to_string(lines) + ':' + std::to_string(words) + ':' + std::to_string(chars) + ')';
      }

      status_location.set_text(status_location_text);
    }
  };
  view->update_status_file_path = [this](Source::BaseView *view) {
    if(get_current_view() == view) {
      if(!Directories::get().path.empty() && filesystem::file_in_path(view->file_path, Directories::get().path))
        status_file_path.set_text(' ' + filesystem::get_relative_path(view->file_path, Directories::get().path).string());
      else
        status_file_path.set_text(' ' + filesystem::get_short_path(view->file_path).string());
    }
  };
  view->update_status_branch = [this](Source::BaseView *view) {
    if(get_current_view() == view) {
      if(!view->status_branch.empty())
        status_branch.set_text(" (" + view->status_branch + ")");
      else
        status_branch.set_text("");
    }
  };
  view->update_status_diagnostics = [this](Source::BaseView *view) {
    if(get_current_view() == view) {
      std::string diagnostic_info;

      auto num_warnings = std::get<0>(view->status_diagnostics);
      auto num_errors = std::get<1>(view->status_diagnostics);
      auto num_fix_its = std::get<2>(view->status_diagnostics);
      if(num_warnings > 0 || num_errors > 0 || num_fix_its > 0) {
        auto normal_color = status_diagnostics.get_style_context()->get_color(Gtk::StateFlags::STATE_FLAG_NORMAL);
        auto light_theme = (normal_color.get_red() + normal_color.get_green() + normal_color.get_blue()) / 3 < 0.5;

        Gdk::RGBA yellow;
        yellow.set_rgba(1.0, 1.0, 0.2);
        double factor = 0.5;
        yellow.set_red(normal_color.get_red() + factor * (yellow.get_red() - normal_color.get_red()));
        yellow.set_green(normal_color.get_green() + factor * (yellow.get_green() - normal_color.get_green()));
        yellow.set_blue(normal_color.get_blue() + factor * (yellow.get_blue() - normal_color.get_blue()));
        Gdk::RGBA red;
        red.set_rgba(1.0, 0.0, 0.0);
        factor = light_theme ? 0.5 : 0.35;
        red.set_red(normal_color.get_red() + factor * (red.get_red() - normal_color.get_red()));
        red.set_green(normal_color.get_green() + factor * (red.get_green() - normal_color.get_green()));
        red.set_blue(normal_color.get_blue() + factor * (red.get_blue() - normal_color.get_blue()));
        Gdk::RGBA green;
        green.set_rgba(0.0, 1.0, 0.0);
        factor = 0.4;
        green.set_red(normal_color.get_red() + factor * (green.get_red() - normal_color.get_red()));
        green.set_green(normal_color.get_green() + factor * (green.get_green() - normal_color.get_green()));
        green.set_blue(normal_color.get_blue() + factor * (green.get_blue() - normal_color.get_blue()));

        std::stringstream yellow_ss, red_ss, green_ss;
        yellow_ss << std::hex << std::setfill('0') << std::setw(2) << (int)(yellow.get_red_u() >> 8) << std::setw(2) << (int)(yellow.get_green_u() >> 8) << std::setw(2) << (int)(yellow.get_blue_u() >> 8);
        red_ss << std::hex << std::setfill('0') << std::setw(2) << (int)(red.get_red_u() >> 8) << std::setw(2) << (int)(red.get_green_u() >> 8) << std::setw(2) << (int)(red.get_blue_u() >> 8);
        green_ss << std::hex << std::setfill('0') << std::setw(2) << (int)(green.get_red_u() >> 8) << std::setw(2) << (int)(green.get_green_u() >> 8) << std::setw(2) << (int)(green.get_blue_u() >> 8);
        if(num_warnings > 0) {
          diagnostic_info += "<span color='#" + yellow_ss.str() + "'>";
          diagnostic_info += std::to_string(num_warnings) + " warning";
          if(num_warnings > 1)
            diagnostic_info += 's';
          diagnostic_info += "</span>";
        }
        if(num_errors > 0) {
          if(num_warnings > 0)
            diagnostic_info += ", ";
          diagnostic_info += "<span color='#" + red_ss.str() + "'>";
          diagnostic_info += std::to_string(num_errors) + " error";
          if(num_errors > 1)
            diagnostic_info += 's';
          diagnostic_info += "</span>";
        }
        if(num_fix_its > 0) {
          if(num_warnings > 0 || num_errors > 0)
            diagnostic_info += ", ";
          diagnostic_info += "<span color='#" + green_ss.str() + "'>";
          diagnostic_info += std::to_string(num_fix_its) + " fix it";
          if(num_fix_its > 1)
            diagnostic_info += 's';
          diagnostic_info += "</span>";
        }
      }
      status_diagnostics.set_markup(diagnostic_info);
    }
  };
  view->update_status_state = [this](Source::BaseView *view) {
    if(get_current_view() == view)
      status_state.set_text(view->status_state + " ");
  };

  scrolled_windows.emplace_back(new Gtk::ScrolledWindow());
  hboxes.emplace_back(new Gtk::Box());
  scrolled_windows.back()->add(*view);
  hboxes.back()->pack_start(*scrolled_windows.back());

  source_maps.emplace_back(Glib::wrap(gtk_source_map_new()));
  gtk_source_map_set_view(GTK_SOURCE_MAP(source_maps.back()->gobj()), view->gobj());
  source_maps.back()->get_style_context()->add_class("juci_source_map");

  configure(source_views.size() - 1);

  // Set up tab label
  tab_labels.emplace_back(new TabLabel([this, view]() {
    close(view);
  }));
  view->update_tab_label = [this](Source::BaseView *view) {
    std::string title = view->file_path.filename().string();
    if(view->get_buffer()->get_modified())
      title += '*';
    else
      title += ' ';
    for(size_t c = 0; c < size(); ++c) {
      if(source_views[c] == view) {
        tab_labels[c]->label.set_text(title);
        tab_labels[c]->set_tooltip_text(filesystem::get_short_path(view->file_path).string());
        return;
      }
    }
  };
  view->update_tab_label(view);

  // Add star on tab label when the page is not saved:
  view->get_buffer()->signal_modified_changed().connect([view]() {
    if(view->update_tab_label)
      view->update_tab_label(view);
  });

  // Cursor history
  auto add_cursor_location = [this, view](const Gtk::TextIter &iter) {
    if(current_cursor_location != cursor_locations.end()) {
      // Remove history newer than current (create new history branch)
      for(auto it = std::next(current_cursor_location); it != cursor_locations.end();)
        it = cursor_locations.erase(it);
    }
    current_cursor_location = cursor_locations.emplace(cursor_locations.end(), view, iter);
    if(cursor_locations.size() > 10)
      cursor_locations.erase(cursor_locations.begin());
  };
  view->get_buffer()->signal_mark_set().connect([this, view, add_cursor_location](const Gtk::TextIter & /*iter*/, const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark) {
    if(mark->get_name() == "insert") {
      if(disable_next_update_cursor_locations) {
        disable_next_update_cursor_locations = false;
        return;
      }

      auto iter = mark->get_iter();
      if(current_cursor_location != cursor_locations.end()) {
        if(current_cursor_location->view == view && abs(current_cursor_location->mark->get_iter().get_line() - iter.get_line()) <= 2) {
          current_cursor_location->view->get_buffer()->move_mark(current_cursor_location->mark, iter); // Move current cursor

          // Combine cursor histories adjacent and similar to current cursor
          auto it = std::next(current_cursor_location);
          if(it != cursor_locations.end()) {
            if(it->view == current_cursor_location->view && abs(it->mark->get_iter().get_line() - current_cursor_location->mark->get_iter().get_line()) <= 2)
              cursor_locations.erase(it);
          }
          if(current_cursor_location != cursor_locations.begin()) {
            it = std::prev(current_cursor_location);
            if(it->view == current_cursor_location->view && abs(it->mark->get_iter().get_line() - current_cursor_location->mark->get_iter().get_line()) <= 2)
              cursor_locations.erase(it);
          }

          return;
        }
      }
      add_cursor_location(iter);
    }
  });
  view->get_buffer()->signal_insert().connect([this, view, add_cursor_location](const Gtk::TextIter & /*iter*/, const Glib::ustring & /*text*/, int /*bytes*/) {
    if(current_cursor_location == cursor_locations.end() || current_cursor_location->view != view)
      add_cursor_location(view->get_buffer()->get_insert()->get_iter());
  });
  view->get_buffer()->signal_erase().connect([this, view, add_cursor_location](const Gtk::TextIter & /*start*/, const Gtk::TextIter & /*end*/) {
    if(current_cursor_location == cursor_locations.end() || current_cursor_location->view != view)
      add_cursor_location(view->get_buffer()->get_insert()->get_iter());

    // Combine adjacent cursor histories that are similar
    if(cursor_locations.size() > 1) {
      auto prev_it = cursor_locations.begin();
      for(auto it = std::next(prev_it); it != cursor_locations.end();) {
        if(prev_it->view == it->view && abs(prev_it->mark->get_iter().get_line() - it->mark->get_iter().get_line()) <= 2) {
          if(current_cursor_location == it)
            current_cursor_location = prev_it;
          it = cursor_locations.erase(it);
        }
        else {
          ++it;
          ++prev_it;
        }
      }
    }
  });

#ifdef JUCI_ENABLE_DEBUG
  if(dynamic_cast<Source::ClangView *>(view) || view->language_id == "rust") {
    view->toggle_breakpoint = [view](int line_nr) {
      if(view->get_source_buffer()->get_source_marks_at_line(line_nr, "debug_breakpoint").size() > 0) {
        auto start_iter = view->get_buffer()->get_iter_at_line(line_nr);
        auto end_iter = view->get_iter_at_line_end(line_nr);
        view->get_source_buffer()->remove_source_marks(start_iter, end_iter, "debug_breakpoint");
        view->get_source_buffer()->remove_source_marks(start_iter, end_iter, "debug_breakpoint_and_stop");
        if(Project::current && Project::debugging)
          Project::current->debug_remove_breakpoint(view->file_path, line_nr + 1, view->get_buffer()->get_line_count() + 1);
      }
      else {
        auto iter = view->get_buffer()->get_iter_at_line(line_nr);
        gtk_source_buffer_create_source_mark(view->get_source_buffer()->gobj(), nullptr, "debug_breakpoint", iter.gobj()); // Gsv::Buffer::create_source_mark is bugged
        if(view->get_source_buffer()->get_source_marks_at_line(line_nr, "debug_stop").size() > 0)
          gtk_source_buffer_create_source_mark(view->get_source_buffer()->gobj(), nullptr, "debug_breakpoint_and_stop", iter.gobj()); // Gsv::Buffer::create_source_mark is bugged
        if(Project::current && Project::debugging)
          Project::current->debug_add_breakpoint(view->file_path, line_nr + 1);
      }
    };
  }
#endif

  view->signal_focus_in_event().connect([this, view](GdkEventFocus *) {
    if(on_focus_page)
      on_focus_page(view);
    set_current_view(view);
    return false;
  });

  if(position == Position::infer) {
    if(!split)
      position = Position::left;
    else if(notebooks[0].get_n_pages() == 0)
      position = Position::left;
    else if(notebooks[1].get_n_pages() == 0)
      position = Position::right;
    else if(last_view)
      position = get_notebook_page(last_view).first == 0 ? Position::left : Position::right;
  }
  size_t notebook_index = position == Position::right ? 1 : 0;
  auto &notebook = notebooks[notebook_index];

  notebook.append_page(*hboxes.back(), *tab_labels.back());

  notebook.set_tab_reorderable(*hboxes.back(), true);
  notebook.set_tab_detachable(*hboxes.back(), true);
  show_all_children();

  notebook.set_current_page(notebook.get_n_pages() - 1);
  last_index.reset();
  if(last_view) {
    auto index = get_index(last_view);
    if(get_notebook_page(index).first == notebook_index)
      last_index = index;
  }

  set_focus_child(*source_views.back());
  focus_view(view);

  return true;
}

void Notebook::install_rust() {
  static bool show_dialog = true;
  if(show_dialog) {
    show_dialog = false;
    Gtk::MessageDialog dialog(*static_cast<Gtk::Window *>(get_toplevel()), "Install Rust?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
    Gtk::Image image;
    image.set_from_icon_name("dialog-question", Gtk::BuiltinIconSize::ICON_SIZE_DIALOG);
    dialog.set_image(image);
    dialog.set_default_response(Gtk::RESPONSE_YES);
    dialog.set_secondary_text("Rust not found. Would you like to install Rust?");
    dialog.show_all();
    int result = dialog.run();
    dialog.hide();
    if(result == Gtk::RESPONSE_YES) {
      bool canceled = false;
      Dialog::Message message("Installing Rust", [&canceled] {
        canceled = true;
      });
      boost::optional<int> exit_status;
      std::string command = "curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y";
      Terminal::get().print("\e[2mRunning: " + command + "\e[m\n");
      auto process = Terminal::get().async_process(command, "", [&exit_status](int exit_status_) {
        exit_status = exit_status_;
      });
      bool killed = false;
      while(!exit_status) {
        if(canceled && !killed) {
          process->kill();
          killed = true;
          show_dialog = true; // Show dialog again
        }
        while(Gtk::Main::events_pending())
          Gtk::Main::iteration();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
      if(exit_status == 0) {
        if(filesystem::find_executable("rustup").empty()) {
#ifdef _WIN32
          boost::filesystem::path cargo_bin;
          if(auto userprofile = std::getenv("USERPROFILE")) // rustup uses this environment variable on MSYS2 as well
            cargo_bin = boost::filesystem::path(userprofile) / ".cargo" / "bin";
          const char delimiter = ';';
#else
          auto cargo_bin = filesystem::get_home_path() / ".cargo" / "bin";
          const char delimiter = ':';
#endif
          if(!cargo_bin.empty()) {
            std::string env;
            if(auto c_env = std::getenv("PATH"))
              env = c_env;
            Glib::setenv("PATH", !env.empty() ? env + delimiter + cargo_bin.string() : cargo_bin.string());
          }
        }
        filesystem::rust_sysroot_path = {};
        filesystem::rust_nightly_sysroot_path = {};
        filesystem::executable_search_paths = {};
        Terminal::get().print("\e[32mSuccess\e[m: installed Rust and updated PATH environment variable\n");
      }
    }
  }
}

void Notebook::open_uri(const std::string &uri) {
#ifdef __APPLE__
  Terminal::get().process("open " + filesystem::escape_argument(uri));
#else
  GError *error = nullptr;
#if GTK_VERSION_GE(3, 22)
  gtk_show_uri_on_window(nullptr, uri.c_str(), GDK_CURRENT_TIME, &error);
#else
  gtk_show_uri(nullptr, uri.c_str(), GDK_CURRENT_TIME, &error);
#endif
  g_clear_error(&error);
#endif
}


void Notebook::configure(size_t index) {
  if(Config::get().source.show_map) {
    if(hboxes.at(index)->get_children().size() == 1)
      hboxes.at(index)->pack_end(*source_maps.at(index), Gtk::PACK_SHRINK);
  }
  else if(hboxes.at(index)->get_children().size() == 2)
    hboxes.at(index)->remove(*source_maps.at(index));
}

bool Notebook::save(size_t index) {
  if(!source_views[index]->save())
    return false;
  Project::on_save(index);
  return true;
}

bool Notebook::save_current() {
  if(auto view = get_current_view())
    return save(get_index(view));
  return false;
}

bool Notebook::close(size_t index) {
  if(auto view = get_view(index)) {
    if(view->get_buffer()->get_modified()) {
      if(!save_modified_dialog(index))
        return false;
    }
    if(view == get_current_view()) {
      bool focused = false;
      if(last_index) {
        auto last_notebook_page = get_notebook_page(*last_index);
        if(get_notebook_page(view).first == last_notebook_page.first) {
          focus_view(source_views[*last_index]);
          notebooks[last_notebook_page.first].set_current_page(last_notebook_page.second);
          last_index.reset();
          focused = true;
        }
      }
      if(!focused) {
        auto notebook_page = get_notebook_page(view);
        if(notebook_page.second > 0)
          focus_view(get_view(notebook_page.first, notebook_page.second - 1));
        else {
          size_t notebook_index = notebook_page.first == 0 ? 1 : 0;
          if(notebooks[notebook_index].get_n_pages() > 0)
            focus_view(get_view(notebook_index, notebooks[notebook_index].get_current_page()));
          else
            set_current_view(nullptr);
        }
      }
    }
    else if(index == last_index)
      last_index.reset();
    else if(index < last_index)
      (*last_index)--;

    auto notebook_page = get_notebook_page(index);
    notebooks[notebook_page.first].remove_page(notebook_page.second);
    source_maps.erase(source_maps.begin() + index);

    if(on_close_page)
      on_close_page(view);

    delete_cursor_locations(view);

    SelectionDialog::get() = nullptr;
    CompletionDialog::get() = nullptr;

    if(auto clang_view = dynamic_cast<Source::ClangView *>(view))
      clang_view->async_delete();
    else
      delete view;
    source_views.erase(source_views.begin() + index);
    scrolled_windows.erase(scrolled_windows.begin() + index);
    hboxes.erase(hboxes.begin() + index);
    tab_labels.erase(tab_labels.begin() + index);
  }
  return true;
}

bool Notebook::close(Source::View *view) {
  return close(get_index(view));
}

bool Notebook::close_current() {
  return close(get_current_view());
}

void Notebook::delete_cursor_locations(Source::View *view) {
  for(auto it = cursor_locations.begin(); it != cursor_locations.end();) {
    if(it->view == view) {
      if(current_cursor_location != cursor_locations.end() && current_cursor_location->view == view)
        current_cursor_location = cursor_locations.end();
      it = cursor_locations.erase(it);
    }
    else
      ++it;
  }

  // Update current location
  if(current_cursor_location == cursor_locations.end()) {
    if(auto current_view = get_current_view()) {
      auto iter = current_view->get_buffer()->get_insert()->get_iter();
      for(auto it = cursor_locations.begin(); it != cursor_locations.end(); ++it) {
        if(it->view == current_view && it->mark->get_iter() == iter) {
          current_cursor_location = it;
          break;
        }
      }
    }
  }
}

void Notebook::next() {
  if(auto view = get_current_view()) {
    auto notebook_page = get_notebook_page(view);
    int page = notebook_page.second + 1;
    if(page >= notebooks[notebook_page.first].get_n_pages())
      notebooks[notebook_page.first].set_current_page(0);
    else
      notebooks[notebook_page.first].set_current_page(page);
  }
}

void Notebook::previous() {
  if(auto view = get_current_view()) {
    auto notebook_page = get_notebook_page(view);
    int page = notebook_page.second - 1;
    if(page < 0)
      notebooks[notebook_page.first].set_current_page(notebooks[notebook_page.first].get_n_pages() - 1);
    else
      notebooks[notebook_page.first].set_current_page(page);
  }
}

void Notebook::toggle_split() {
  if(!split) {
    pack2(notebooks[1], true, true);
    set_position(get_width() / 2);
    show_all();
    // Make sure the position is correct
    // TODO: report bug to gtk if it is not fixed in gtk3.22
    Glib::signal_timeout().connect(
        [this] {
          set_position(get_width() / 2);
          return false;
        },
        200);
  }
  else {
    for(size_t c = size() - 1; c != static_cast<size_t>(-1); --c) {
      if(get_notebook_page(c).first == 1 && !close(c))
        return;
    }
    remove(notebooks[1]);
  }
  split = !split;
}

std::vector<std::pair<size_t, Source::View *>> Notebook::get_notebook_views() {
  std::vector<std::pair<size_t, Source::View *>> notebook_views;
  for(size_t notebook_index = 0; notebook_index < notebooks.size(); ++notebook_index) {
    for(int page = 0; page < notebooks[notebook_index].get_n_pages(); ++page)
      notebook_views.emplace_back(notebook_index, get_view(notebook_index, page));
  }
  return notebook_views;
}

void Notebook::update_status(Source::BaseView *view) {
  if(view->update_status_location)
    view->update_status_location(view);
  if(view->update_status_file_path)
    view->update_status_file_path(view);
  if(view->update_status_branch)
    view->update_status_branch(view);
  if(view->update_status_diagnostics)
    view->update_status_diagnostics(view);
  if(view->update_status_state)
    view->update_status_state(view);
}

void Notebook::clear_status() {
  status_location.set_text("");
  status_file_path.set_text("");
  status_branch.set_text("");
  status_diagnostics.set_text("");
  status_state.set_text("");
}

Source::View *Notebook::get_view(size_t notebook_index, int page) {
  if(notebook_index >= notebooks.size())
    throw std::out_of_range("notebook index out of bounds");
  auto widget = notebooks[notebook_index].get_nth_page(page);
  if(!widget)
    throw std::out_of_range("page number out of bounds");
  auto hbox = dynamic_cast<Gtk::Box *>(widget);
  auto scrolled_window = dynamic_cast<Gtk::ScrolledWindow *>(hbox->get_children()[0]);
  return dynamic_cast<Source::View *>(scrolled_window->get_children()[0]);
}

void Notebook::focus_view(Source::View *view) {
  intermediate_view = view;
  view->grab_focus();
}

size_t Notebook::get_index(Source::View *view) {
  for(size_t c = 0; c < size(); ++c) {
    if(source_views[c] == view)
      return c;
  }
  throw std::out_of_range("view not found");
}

std::pair<size_t, int> Notebook::get_notebook_page(size_t index) {
  for(size_t c = 0; c < notebooks.size(); ++c) {
    auto page_num = notebooks[c].page_num(*hboxes[index]);
    if(page_num >= 0)
      return {c, page_num};
  }
  throw std::out_of_range("index out of bounds");
}

std::pair<size_t, int> Notebook::get_notebook_page(Source::View *view) {
  return get_notebook_page(get_index(view));
}

void Notebook::set_current_view(Source::View *view) {
  intermediate_view = nullptr;
  if(current_view != view) {
    if(auto view = get_current_view()) {
      view->hide_tooltips();
      view->hide_dialogs();
    }
    current_view = view;
    if(view && on_change_page)
      on_change_page(view);
  }
}

bool Notebook::save_modified_dialog(size_t index) {
  Gtk::MessageDialog dialog(*static_cast<Gtk::Window *>(get_toplevel()), "Save file?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
  Gtk::Image image;
  image.set_from_icon_name("document-save", Gtk::BuiltinIconSize::ICON_SIZE_DIALOG);
  dialog.set_image(image);
  dialog.set_default_response(Gtk::RESPONSE_YES);
  dialog.set_secondary_text("Do you want to save " + filesystem::get_short_path(get_view(index)->file_path).string() + "?");
  dialog.show_all();
  int result = dialog.run();
  if(result == Gtk::RESPONSE_YES)
    return save(index);
  else if(result == Gtk::RESPONSE_NO)
    return true;
  return false;
}
