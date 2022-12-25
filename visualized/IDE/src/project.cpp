#include "project.hpp"
#include "commands.hpp"
#include "config.hpp"
#include "directories.hpp"
#include "filesystem.hpp"
#include "menu.hpp"
#include "mutex.hpp"
#include "notebook.hpp"
#include "selection_dialog.hpp"
#include "terminal.hpp"
#include <fstream>
#ifdef JUCI_ENABLE_DEBUG
#include "debug_lldb.hpp"
#endif
#include "ctags.hpp"
#include "info.hpp"
#include "snippets.hpp"
#include "source_clang.hpp"
#include "usages_clang.hpp"
#include <future>

boost::filesystem::path Project::debug_last_stop_file_path;
std::unordered_map<std::string, std::string> Project::run_arguments;
std::unordered_map<std::string, Project::DebugRunArguments> Project::debug_run_arguments;
std::atomic<bool> Project::compiling(false);
std::atomic<bool> Project::debugging(false);
std::pair<boost::filesystem::path, std::pair<int, int>> Project::debug_stop;
std::string Project::debug_status;
std::shared_ptr<Project::Base> Project::current;
std::unique_ptr<Project::DebugOptions> Project::Base::debug_options;

boost::filesystem::path Project::get_preferably_view_folder() {
  boost::filesystem::path view_folder;
  if(auto view = Notebook::get().get_current_view())
    return view->file_path.parent_path();
  else if(!Directories::get().path.empty())
    return Directories::get().path;
  else {
    auto current_path = filesystem::get_current_path();
    return !current_path.empty() ? current_path : boost::filesystem::path();
  }
}

boost::filesystem::path Project::get_preferably_directory_folder() {
  if(!Directories::get().path.empty())
    return Directories::get().path;
  else if(auto view = Notebook::get().get_current_view())
    return view->file_path.parent_path();
  else {
    auto current_path = filesystem::get_current_path();
    return !current_path.empty() ? current_path : boost::filesystem::path();
  }
}

void Project::save_files(const boost::filesystem::path &path) {
  for(size_t c = 0; c < Notebook::get().size(); c++) {
    auto view = Notebook::get().get_view(c);
    if(view->get_buffer()->get_modified()) {
      if(filesystem::file_in_path(view->file_path, path))
        Notebook::get().save(c);
    }
  }
}

void Project::on_save(size_t index) {
  auto view = Notebook::get().get_view(index);
  if(!view)
    return;

  if(view->file_path == Config::get().home_juci_path / "snippets.json") {
    Snippets::get().load();
    for(auto view : Notebook::get().get_views())
      view->set_snippets();
  }

  if(view->file_path == Config::get().home_juci_path / "commands.json")
    Commands::get().load();

  boost::filesystem::path build_path;
  if(view->language_id == "cmake") {
    if(view->file_path.filename() == "CMakeLists.txt")
      build_path = view->file_path;
    else
      build_path = filesystem::find_file_in_path_parents("CMakeLists.txt", view->file_path.parent_path());
  }
  else if(view->language_id == "meson") {
    if(view->file_path.filename() == "meson.build")
      build_path = view->file_path;
    else
      build_path = filesystem::find_file_in_path_parents("meson.build", view->file_path.parent_path());
  }

  if(!build_path.empty()) {
    auto build = Build::create(build_path);
    if(dynamic_cast<CMakeBuild *>(build.get()) || dynamic_cast<MesonBuild *>(build.get())) {
      build->update_default(true);
      Usages::Clang::erase_all_caches_for_project(build->project_path, build->get_default_path());
      boost::system::error_code ec;
      if(boost::filesystem::exists(build->get_debug_path()), ec)
        build->update_debug(true);

      for(size_t c = 0; c < Notebook::get().size(); c++) {
        auto source_view = Notebook::get().get_view(c);
        if(auto source_clang_view = dynamic_cast<Source::ClangView *>(source_view)) {
          if(filesystem::file_in_path(source_clang_view->file_path, build->project_path))
            source_clang_view->full_reparse_needed = true;
        }
      }
    }
  }
}

Gtk::Label &Project::debug_status_label() {
  static Gtk::Label label;
  return label;
}

void Project::debug_update_status(const std::string &new_debug_status) {
  debug_status = new_debug_status;
  if(debug_status.empty())
    debug_status_label().set_text("");
  else
    debug_status_label().set_text(debug_status);
  debug_activate_menu_items();
}

void Project::debug_activate_menu_items() {
  auto &menu = Menu::get();
  auto view = Notebook::get().get_current_view();
  menu.actions["debug_stop"]->set_enabled(!debug_status.empty());
  menu.actions["debug_kill"]->set_enabled(!debug_status.empty());
  menu.actions["debug_step_over"]->set_enabled(!debug_status.empty());
  menu.actions["debug_step_into"]->set_enabled(!debug_status.empty());
  menu.actions["debug_step_out"]->set_enabled(!debug_status.empty());
  menu.actions["debug_backtrace"]->set_enabled(!debug_status.empty());
  menu.actions["debug_show_variables"]->set_enabled(!debug_status.empty());
  menu.actions["debug_run_command"]->set_enabled(!debug_status.empty());
  menu.actions["debug_toggle_breakpoint"]->set_enabled(view && view->toggle_breakpoint);
  menu.actions["debug_goto_stop"]->set_enabled(!debug_status.empty());
}

void Project::debug_update_stop() {
  if(!debug_last_stop_file_path.empty()) {
    for(size_t c = 0; c < Notebook::get().size(); c++) {
      auto view = Notebook::get().get_view(c);
      if(view->file_path == debug_last_stop_file_path) {
        view->get_source_buffer()->remove_source_marks(view->get_buffer()->begin(), view->get_buffer()->end(), "debug_stop");
        view->get_source_buffer()->remove_source_marks(view->get_buffer()->begin(), view->get_buffer()->end(), "debug_breakpoint_and_stop");
        break;
      }
    }
  }
  // Add debug stop source mark
  debug_last_stop_file_path.clear();
  for(size_t c = 0; c < Notebook::get().size(); c++) {
    auto view = Notebook::get().get_view(c);
    if(view->file_path == debug_stop.first) {
      if(debug_stop.second.first < view->get_buffer()->get_line_count()) {
        auto iter = view->get_buffer()->get_iter_at_line(debug_stop.second.first);
        gtk_source_buffer_create_source_mark(view->get_source_buffer()->gobj(), nullptr, "debug_stop", iter.gobj()); // Gsv::Buffer::create_source_mark is bugged
        if(view->get_source_buffer()->get_source_marks_at_iter(iter, "debug_breakpoint").size() > 0)
          gtk_source_buffer_create_source_mark(view->get_source_buffer()->gobj(), nullptr, "debug_breakpoint_and_stop", iter.gobj()); // Gsv::Buffer::create_source_mark is bugged
        debug_last_stop_file_path = debug_stop.first;
      }
      break;
    }
  }
}

std::shared_ptr<Project::Base> Project::create() {
  std::unique_ptr<Project::Build> build;

  if(auto view = Notebook::get().get_current_view()) {
    build = Build::create(view->file_path);
    if(view->language_id == "markdown")
      return std::shared_ptr<Project::Base>(new Project::Markdown(std::move(build)));
    if(view->language_id == "js")
      return std::shared_ptr<Project::Base>(new Project::JavaScript(std::move(build)));
    if(view->language_id == "python")
      return std::shared_ptr<Project::Base>(new Project::Python(std::move(build)));
    if(view->language_id == "html")
      return std::shared_ptr<Project::Base>(new Project::HTML(std::move(build)));
    if(view->language_id == "go")
      return std::shared_ptr<Project::Base>(new Project::Go(std::move(build)));
    if(view->language_id == "julia")
      return std::shared_ptr<Project::Base>(new Project::Julia(std::move(build)));
  }
  else
    build = Build::create(Directories::get().path);

  if(dynamic_cast<CMakeBuild *>(build.get()) || dynamic_cast<MesonBuild *>(build.get()))
    return std::shared_ptr<Project::Base>(new Project::Clang(std::move(build)));
  if(dynamic_cast<CargoBuild *>(build.get()))
    return std::shared_ptr<Project::Base>(new Project::Rust(std::move(build)));
  if(dynamic_cast<NpmBuild *>(build.get()))
    return std::shared_ptr<Project::Base>(new Project::JavaScript(std::move(build)));
  if(dynamic_cast<PythonMain *>(build.get()))
    return std::shared_ptr<Project::Base>(new Project::Python(std::move(build)));
  if(dynamic_cast<GoBuild *>(build.get()))
    return std::shared_ptr<Project::Base>(new Project::Go(std::move(build)));
  return std::shared_ptr<Project::Base>(new Project::Base(std::move(build)));
}

std::pair<std::string, std::string> Project::Base::get_run_arguments() {
  Info::get().print("Could not find a supported project");
  return {"", ""};
}

void Project::Base::compile() {
  Info::get().print("Could not find a supported project");
}

void Project::Base::compile_and_run() {
  Info::get().print("Could not find a supported project");
}

void Project::Base::compile_and_install() {
  Info::get().print("Could not find a supported project");
}

void Project::Base::recreate_build() {
  Info::get().print("Could not find a supported project");
}

void Project::Base::show_symbols() {
  Ctags ctags(get_preferably_view_folder());
  if(!ctags) {
    Info::get().print("No symbols found in current project");
    return;
  }

  auto view = Notebook::get().get_current_view();
  if(view)
    SelectionDialog::create(view, true, true);
  else
    SelectionDialog::create(true, true);

  std::vector<Source::Offset> rows;

  std::string line;
  while(std::getline(ctags.output, line)) {
    auto location = ctags.get_location(line, true);

    std::string row = location.file_path.string() + ":" + std::to_string(location.line + 1) + ": " + location.source;
    rows.emplace_back(Source::Offset(location.line, location.index, location.file_path));
    SelectionDialog::get()->add_row(row);
  }

  if(rows.size() == 0)
    return;
  SelectionDialog::get()->on_select = [rows = std::move(rows), project_path = std::move(ctags.project_path)](unsigned int index, const std::string &text, bool hide_window) {
    auto offset = rows[index];
    auto full_path = project_path / offset.file_path;
    boost::system::error_code ec;
    if(!boost::filesystem::is_regular_file(full_path, ec))
      return;
    if(Notebook::get().open(full_path)) {
      auto view = Notebook::get().get_current_view();
      view->place_cursor_at_line_index(offset.line, offset.index);
      view->scroll_to_cursor_delayed(true, false);
    }
  };
  if(view)
    view->hide_tooltips();
  SelectionDialog::get()->show();
}

std::pair<std::string, std::string> Project::Base::debug_get_run_arguments() {
  Info::get().print("Could not find a supported project");
  return {"", ""};
}

void Project::Base::debug_compile_and_start() {
  Info::get().print("Could not find a supported project");
}

void Project::Base::debug_start(const std::string &command, const boost::filesystem::path &path, const std::string &remote_host) {
  Info::get().print("Could not find a supported project");
  Project::debugging = false;
}

#ifdef JUCI_ENABLE_DEBUG
std::pair<std::string, std::string> Project::LLDB::debug_get_run_arguments() {
  auto debug_build_path = build->get_debug_path();
  auto default_build_path = build->get_default_path();
  if(debug_build_path.empty() || default_build_path.empty() || !build->update_default())
    return {"", ""};

  auto project_path = build->project_path.string();
  auto run_arguments_it = debug_run_arguments.find(project_path);
  std::string arguments;
  if(run_arguments_it != debug_run_arguments.end())
    arguments = run_arguments_it->second.arguments;

  if(arguments.empty()) {
    auto view = Notebook::get().get_current_view();
    auto executable = build->get_executable(view ? view->file_path : Directories::get().path).string();

    if(!executable.empty()) {
      size_t pos = executable.find(default_build_path.string());
      if(pos != std::string::npos)
        executable.replace(pos, default_build_path.string().size(), debug_build_path.string());
      arguments = filesystem::escape_argument(filesystem::get_short_path(executable).string());
    }
    else
      arguments = filesystem::escape_argument(filesystem::get_short_path(build->get_debug_path()).string());
  }

  return {project_path, arguments};
}

Project::DebugOptions *Project::LLDB::debug_get_options() {
  if(build->project_path.empty())
    return nullptr;

  debug_options = std::make_unique<DebugOptions>();

  auto &arguments = Project::debug_run_arguments[build->project_path.string()];

  auto remote_enabled = Gtk::manage(new Gtk::CheckButton());
  auto remote_host_port = Gtk::manage(new Gtk::Entry());
  remote_enabled->set_active(arguments.remote_enabled);
  remote_enabled->set_label("Enabled");
  remote_enabled->signal_clicked().connect([remote_enabled, remote_host_port] {
    remote_host_port->set_sensitive(remote_enabled->get_active());
  });

  remote_host_port->set_sensitive(arguments.remote_enabled);
  remote_host_port->set_text(arguments.remote_host_port);
  remote_host_port->set_placeholder_text("host:port");
  remote_host_port->signal_activate().connect([] {
    debug_options->hide();
  });

  auto self = this->shared_from_this();
  debug_options->signal_hide().connect([self, remote_enabled, remote_host_port] {
    auto &arguments = Project::debug_run_arguments[self->build->project_path.string()];
    arguments.remote_enabled = remote_enabled->get_active();
    arguments.remote_host_port = remote_host_port->get_text();
  });

  auto remote_vbox = Gtk::manage(new Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL));
  remote_vbox->pack_start(*remote_enabled, true, true);
  remote_vbox->pack_end(*remote_host_port, true, true);

  auto remote_frame = Gtk::manage(new Gtk::Frame());
  remote_frame->set_label("Remote Debugging");
  remote_frame->add(*remote_vbox);

  debug_options->vbox.pack_end(*remote_frame, true, true);

  return debug_options.get();
}

void Project::LLDB::debug_compile_and_start() {
  auto default_build_path = build->get_default_path();
  if(default_build_path.empty() || !build->update_default())
    return;
  auto debug_build_path = build->get_debug_path();
  if(debug_build_path.empty() || !build->update_debug())
    return;

  auto run_arguments_it = debug_run_arguments.find(build->project_path.string());
  std::string run_arguments;
  std::string remote_host;
  if(run_arguments_it != debug_run_arguments.end()) {
    run_arguments = run_arguments_it->second.arguments;
    if(run_arguments_it->second.remote_enabled)
      remote_host = run_arguments_it->second.remote_host_port;
  }

  if(run_arguments.empty()) {
    auto view = Notebook::get().get_current_view();
    run_arguments = build->get_executable(view ? view->file_path : Directories::get().path).string();
    if(run_arguments.empty()) {
      if(!build->is_valid(default_build_path))
        Terminal::get().print("\e[31mError\e[m: build folder no longer valid, please use Recreate Build in the Project menu.\n", true);
      else {
        Terminal::get().print("\e[33mWarning\e[m: could not find executable.\n");
        Terminal::get().print("Either use Set Run Arguments in the Debug menu, or open a source file within a directory where an executable is defined.\n");
      }
      return;
    }
    size_t pos = run_arguments.find(default_build_path.string());
    if(pos != std::string::npos)
      run_arguments.replace(pos, default_build_path.string().size(), debug_build_path.string());
    run_arguments = filesystem::escape_argument(filesystem::get_short_path(run_arguments).string());
  }

  debugging = true;

  if(Config::get().terminal.clear_on_compile)
    Terminal::get().clear();

  Terminal::get().print("\e[2mCompiling and debugging: " + run_arguments + "\e[m\n");
  Terminal::get().async_process(build->get_compile_command(), debug_build_path, [self = shared_from_this(), debug_build_path, run_arguments, project_path = build->project_path, remote_host](int exit_status) {
    if(exit_status != 0) {
      debugging = false;
      if(!self->build->is_valid(debug_build_path))
        Terminal::get().print("\e[31mError\e[m: build folder no longer valid, please use Recreate Build in the Project menu.\n", true);
    }
    else {
      self->debug_start(run_arguments, project_path, remote_host);
    }
  });
}

void Project::LLDB::debug_start(const std::string &command, const boost::filesystem::path &path, const std::string &remote_host) {
  std::vector<std::pair<boost::filesystem::path, int>> breakpoints;
  for(size_t c = 0; c < Notebook::get().size(); c++) {
    auto view = Notebook::get().get_view(c);
    if(filesystem::file_in_path(view->file_path, path)) {
      auto iter = view->get_buffer()->begin();
      if(view->get_source_buffer()->get_source_marks_at_iter(iter, "debug_breakpoint").size() > 0)
        breakpoints.emplace_back(view->file_path, iter.get_line() + 1);
      while(view->get_source_buffer()->forward_iter_to_source_mark(iter, "debug_breakpoint"))
        breakpoints.emplace_back(view->file_path, iter.get_line() + 1);
    }
  }

  static auto on_exit_it = Debug::LLDB::get().on_exit.end();
  if(on_exit_it != Debug::LLDB::get().on_exit.end())
    Debug::LLDB::get().on_exit.erase(on_exit_it);
  Debug::LLDB::get().on_exit.emplace_back([self = shared_from_this(), command](int exit_status) {
    debugging = false;
    Terminal::get().async_print("\e[2m" + command + " returned: " + (exit_status == 0 ? "\e[32m" : "\e[31m") + std::to_string(exit_status) + "\e[m\n");
    self->dispatcher.post([] {
      debug_update_status("");
    });
  });
  on_exit_it = std::prev(Debug::LLDB::get().on_exit.end());

  static auto on_event_it = Debug::LLDB::get().on_event.end();
  if(on_event_it != Debug::LLDB::get().on_event.end())
    Debug::LLDB::get().on_event.erase(on_event_it);
  Debug::LLDB::get().on_event.emplace_back([self = shared_from_this()](const lldb::SBEvent &event) {
    std::string status;
    boost::filesystem::path stop_path;
    unsigned stop_line = 0, stop_column = 0;

    LockGuard lock(Debug::LLDB::get().mutex);
    auto process = lldb::SBProcess::GetProcessFromEvent(event);
    auto state = lldb::SBProcess::GetStateFromEvent(event);
    lldb::SBStream stream;
    event.GetDescription(stream);
    std::string event_desc = stream.GetData();
    event_desc.pop_back();
    auto pos = event_desc.rfind(" = ");
    if(pos != std::string::npos && pos + 3 < event_desc.size())
      status = event_desc.substr(pos + 3);
    if(state == lldb::StateType::eStateStopped) {
      char buffer[100];
      auto thread = process.GetSelectedThread();
      auto n = thread.GetStopDescription(buffer, 100); // Returns number of bytes read. Might include null termination... Although maybe on newer versions only.
      if(n > 0)
        status += " (" + std::string(buffer, n <= 100 ? (buffer[n - 1] == '\0' ? n - 1 : n) : 100) + ")";
      auto line_entry = thread.GetSelectedFrame().GetLineEntry();
      if(line_entry.IsValid()) {
        lldb::SBStream stream;
        line_entry.GetFileSpec().GetDescription(stream);
        auto line = line_entry.GetLine();
        status += " " + boost::filesystem::path(stream.GetData()).filename().string() + ":" + std::to_string(line);
        auto column = line_entry.GetColumn();
        if(column == 0)
          column = 1;
        stop_path = filesystem::get_normal_path(stream.GetData());
        stop_line = line - 1;
        stop_column = column - 1;
      }
    }

    self->dispatcher.post([status = std::move(status), stop_path = std::move(stop_path), stop_line, stop_column] {
      debug_update_status(status);
      Project::debug_stop.first = stop_path;
      Project::debug_stop.second.first = stop_line;
      Project::debug_stop.second.second = stop_column;
      debug_update_stop();

      if(Config::get().source.debug_place_cursor_at_stop && !stop_path.empty()) {
        if(Notebook::get().open(stop_path)) {
          auto view = Notebook::get().get_current_view();
          view->place_cursor_at_line_index(stop_line, stop_column);
          view->scroll_to_cursor_delayed(true, false);
        }
      }
      else if(auto view = Notebook::get().get_current_view())
        view->get_buffer()->place_cursor(view->get_buffer()->get_insert()->get_iter());
    });
  });
  on_event_it = std::prev(Debug::LLDB::get().on_event.end());

  std::vector<std::string> startup_commands;
  if(dynamic_cast<CargoBuild *>(build.get())) {
    auto sysroot = filesystem::get_rust_sysroot_path().string();
    if(!sysroot.empty()) {
      std::string line;
      std::ifstream input(sysroot + "/lib/rustlib/etc/lldb_commands", std::ios::binary);
      if(input) {
        startup_commands.emplace_back("command script import \"" + sysroot + "/lib/rustlib/etc/lldb_lookup.py\"");
        while(std::getline(input, line))
          startup_commands.emplace_back(line);
      }
    }
  }
  Debug::LLDB::get().start(command, path, breakpoints, startup_commands, remote_host);
}

void Project::LLDB::debug_continue() {
  Debug::LLDB::get().continue_debug();
}

void Project::LLDB::debug_stop() {
  if(debugging)
    Debug::LLDB::get().stop();
}

void Project::LLDB::debug_kill() {
  if(debugging)
    Debug::LLDB::get().kill();
}

void Project::LLDB::debug_step_over() {
  if(debugging)
    Debug::LLDB::get().step_over();
}

void Project::LLDB::debug_step_into() {
  if(debugging)
    Debug::LLDB::get().step_into();
}

void Project::LLDB::debug_step_out() {
  if(debugging)
    Debug::LLDB::get().step_out();
}

void Project::LLDB::debug_backtrace() {
  if(debugging) {
    auto view = Notebook::get().get_current_view();
    auto frames = Debug::LLDB::get().get_backtrace();

    if(frames.size() == 0) {
      Info::get().print("No backtrace found");
      return;
    }

    if(view)
      SelectionDialog::create(view, true, true);
    else
      SelectionDialog::create(true, true);

    bool cursor_set = false;
    for(auto &frame : frames) {
      std::string row = "<i>" + frame.module_filename + "</i>";

      // Shorten frame.function_name if it is too long
      if(frame.function_name.size() > 120) {
        frame.function_name = frame.function_name.substr(0, 58) + "...." + frame.function_name.substr(frame.function_name.size() - 58);
      }
      if(frame.file_path.empty())
        row += " - " + Glib::Markup::escape_text(frame.function_name);
      else {
        auto file_path = frame.file_path.filename().string();
        row += ":<b>" + Glib::Markup::escape_text(file_path) + ":" + std::to_string(frame.line_nr) + "</b> - " + Glib::Markup::escape_text(frame.function_name);
      }
      SelectionDialog::get()->add_row(row);
      if(!cursor_set && view && frame.file_path == view->file_path) {
        SelectionDialog::get()->set_cursor_at_last_row();
        cursor_set = true;
      }
    }

    SelectionDialog::get()->on_select = [frames = std::move(frames)](unsigned int index, const std::string &text, bool hide_window) {
      auto &frame = frames[index];
      if(!frame.file_path.empty()) {
        if(Notebook::get().open(frame.file_path)) {
          Debug::LLDB::get().select_frame(frame.index);
          auto view = Notebook::get().get_current_view();
          view->place_cursor_at_line_index(frame.line_nr - 1, frame.line_index - 1);
          view->scroll_to_cursor_delayed(true, true);
        }
      }
    };
    if(view)
      view->hide_tooltips();
    SelectionDialog::get()->show();
  }
}

void Project::LLDB::debug_show_variables() {
  if(debugging) {
    auto view = Notebook::get().get_current_view();
    auto variables = std::make_shared<std::vector<Debug::LLDB::Variable>>(Debug::LLDB::get().get_variables());

    if(variables->size() == 0) {
      Info::get().print("No variables found");
      return;
    }

    if(view)
      SelectionDialog::create(view, true, true);
    else
      SelectionDialog::create(true, true);

    for(auto &variable : *variables) {
      std::string row = "#" + std::to_string(variable.thread_index_id) + ":#" + std::to_string(variable.frame_index) + ":" + variable.file_path.filename().string() + ":" + std::to_string(variable.line_nr) + " - <b>" + Glib::Markup::escape_text(variable.name) + "</b>";

      SelectionDialog::get()->add_row(row);
    }

    SelectionDialog::get()->on_select = [variables](unsigned int index, const std::string &text, bool hide_window) {
      auto &variable = (*variables)[index];
      Debug::LLDB::get().select_frame(variable.frame_index, variable.thread_index_id);
      if(!variable.file_path.empty()) {
        if(Notebook::get().open(variable.file_path)) {
          auto view = Notebook::get().get_current_view();
          view->place_cursor_at_line_index(variable.line_nr - 1, variable.line_index - 1);
          view->scroll_to_cursor_delayed(true, true);
        }
      }
      if(!variable.declaration_found)
        Info::get().print("Debugger did not find declaration for the variable: " + variable.name);
    };

    SelectionDialog::get()->on_hide = [self = this->shared_from_this()]() {
      self->debug_variable_tooltips.hide();
      self->debug_variable_tooltips.clear();
    };

    SelectionDialog::get()->on_change = [self = this->shared_from_this(), variables, view](boost::optional<unsigned int> index, const std::string &text) {
      if(!index) {
        self->debug_variable_tooltips.hide();
        return;
      }
      self->debug_variable_tooltips.clear();

      auto set_tooltip_buffer = [variables, index](Tooltip &tooltip) {
        auto &variable = (*variables)[*index];

        Glib::ustring value = variable.get_value();
        if(!value.empty()) {
          Glib::ustring::iterator iter;
          while(!value.validate(iter)) {
            auto next_char_iter = iter;
            next_char_iter++;
            value.replace(iter, next_char_iter, "?");
          }
          tooltip.insert_code(value.substr(0, value.size() - 1));
        }
      };
      if(view) {
        auto iter = view->get_buffer()->get_insert()->get_iter();
        self->debug_variable_tooltips.emplace_back(view, iter, iter, std::move(set_tooltip_buffer));
      }
      else
        self->debug_variable_tooltips.emplace_back(std::move(set_tooltip_buffer));

      self->debug_variable_tooltips.show(true);
    };

    if(view)
      view->hide_tooltips();
    SelectionDialog::get()->show();
  }
}

void Project::LLDB::debug_run_command(const std::string &command) {
  if(debugging) {
    auto command_return = Debug::LLDB::get().run_command(command);
    Terminal::get().async_print(std::move(command_return.first));
    Terminal::get().async_print(std::move(command_return.second), true);
  }
}

void Project::LLDB::debug_add_breakpoint(const boost::filesystem::path &file_path, int line_nr) {
  Debug::LLDB::get().add_breakpoint(file_path, line_nr);
}

void Project::LLDB::debug_remove_breakpoint(const boost::filesystem::path &file_path, int line_nr, int line_count) {
  Debug::LLDB::get().remove_breakpoint(file_path, line_nr, line_count);
}

bool Project::LLDB::debug_is_running() {
  return Debug::LLDB::get().is_running();
}

void Project::LLDB::debug_write(const std::string &buffer) {
  Debug::LLDB::get().write(buffer);
}
#endif

std::pair<std::string, std::string> Project::Clang::get_run_arguments() {
  auto build_path = build->get_default_path();
  if(build_path.empty() || !build->update_default())
    return {"", ""};

  auto project_path = build->project_path.string();
  auto run_arguments_it = run_arguments.find(project_path);
  std::string arguments;
  if(run_arguments_it != run_arguments.end())
    arguments = run_arguments_it->second;

  if(arguments.empty()) {
    auto view = Notebook::get().get_current_view();
    auto executable = build->get_executable(view ? view->file_path : Directories::get().path);

    if(!executable.empty())
      arguments = filesystem::escape_argument(filesystem::get_short_path(executable).string());
    else
      arguments = filesystem::escape_argument(filesystem::get_short_path(build->get_default_path()).string());
  }

  return {project_path, arguments};
}

void Project::Clang::compile() {
  auto default_build_path = build->get_default_path();
  if(default_build_path.empty() || !build->update_default())
    return;

  compiling = true;

  if(Config::get().terminal.clear_on_compile)
    Terminal::get().clear();

  Terminal::get().print("\e[2mCompiling project: " + filesystem::get_short_path(build->project_path).string() + "\e[m\n");
  Terminal::get().async_process(build->get_compile_command(), default_build_path, [self = shared_from_this(), default_build_path](int exit_status) {
    compiling = false;
    if(exit_status != 0 && !self->build->is_valid(default_build_path))
      Terminal::get().print("\e[31mError\e[m: build folder no longer valid, please use Recreate Build in the Project menu.\n", true);
  });
}

void Project::Clang::compile_and_install() {
  auto default_build_path = build->get_default_path();
  if(default_build_path.empty() || !build->update_default())
    return;

  compiling = true;

  if(Config::get().terminal.clear_on_compile)
    Terminal::get().clear();

  Terminal::get().print("\e[2mCompiling project: " + filesystem::get_short_path(build->project_path).string() + "\e[m\n");
  Terminal::get().async_process(build->get_compile_command(), default_build_path, [self = shared_from_this(), default_build_path](int exit_status) {
    compiling = false;
    if(exit_status != 0 && !self->build->is_valid(default_build_path)){
		Terminal::get().print("\e[31mError\e[m: build folder no longer valid, please use Recreate Build in the Project menu.\n", true);
	}else{
		const std::string &content = "make install";
		Terminal::get().async_process(content, default_build_path, [content](int exit_status) {
		  //Terminal::get().print("\e[2m" + content + " returned: " + (exit_status == 0 ? "\e[32m" : "\e[31m") + std::to_string(exit_status) + "\e[m\n");
		});
	}
  });
}


void Project::Clang::compile_and_run() {
  auto default_build_path = build->get_default_path();
  if(default_build_path.empty() || !build->update_default())
    return;

  auto project_path = build->project_path;

  auto run_arguments_it = run_arguments.find(project_path.string());
  std::string arguments;
  if(run_arguments_it != run_arguments.end())
    arguments = run_arguments_it->second;

  if(arguments.empty()) {
    auto view = Notebook::get().get_current_view();
    auto executable = build->get_executable(view ? view->file_path : Directories::get().path);
    if(executable.empty()) {
      if(!build->is_valid(default_build_path))
        Terminal::get().print("\e[31mError\e[m: build folder no longer valid, please use Recreate Build in the Project menu.\n", true);
      else {
        Terminal::get().print("\e[33mWarning\e[m: could not find executable.\n");
        Terminal::get().print("Either use Set Run Arguments in the Project menu, or open a source file within a directory where an executable is defined.\n");
      }
      return;
    }
    arguments = filesystem::escape_argument(filesystem::get_short_path(executable).string());
  }

  compiling = true;

  if(Config::get().terminal.clear_on_compile)
    Terminal::get().clear();

  Terminal::get().print("\e[2mCompiling and running: " + arguments + "\e[m\n");
  Terminal::get().async_process(build->get_compile_command(), default_build_path, [self = shared_from_this(), arguments, project_path, default_build_path](int exit_status) {
    compiling = false;
    if(exit_status == 0) {
      Terminal::get().async_process(arguments, project_path, [arguments](int exit_status) {
        Terminal::get().print("\e[2m" + arguments + " returned: " + (exit_status == 0 ? "\e[32m" : "\e[31m") + std::to_string(exit_status) + "\e[m\n");
      });
    }
    else if(!self->build->is_valid(default_build_path))
      Terminal::get().print("\e[31mError\e[m: build folder no longer valid, please use Recreate Build in the Project menu.\n", true);
  });
}

void Project::Clang::recreate_build() {
  if(build->project_path.empty())
    return;
  auto default_build_path = build->get_default_path();
  if(default_build_path.empty())
    return;

  auto debug_build_path = build->get_debug_path();
  boost::system::error_code ec;
  bool has_default_build = boost::filesystem::exists(default_build_path, ec);
  bool has_debug_build = !debug_build_path.empty() && boost::filesystem::exists(debug_build_path, ec);

  if(has_default_build || has_debug_build) {
    Gtk::MessageDialog dialog(*static_cast<Gtk::Window *>(Notebook::get().get_toplevel()), "Recreate Build?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO);
    Gtk::Image image;
    image.set_from_icon_name("dialog-question", Gtk::BuiltinIconSize::ICON_SIZE_DIALOG);
    dialog.set_image(image);
    dialog.set_default_response(Gtk::RESPONSE_YES);
    std::string message = "Are you sure you want to recreate ";
    if(has_default_build)
      message += filesystem::get_short_path(default_build_path).string();
    if(has_debug_build) {
      if(has_default_build)
        message += " and ";
      message += filesystem::get_short_path(debug_build_path).string();
    }
    dialog.set_secondary_text(message + "?");
    dialog.show_all();
    if(dialog.run() != Gtk::RESPONSE_YES)
      return;
    Usages::Clang::erase_all_caches_for_project(build->project_path, default_build_path);
    try {
      if(has_default_build) {
        std::vector<boost::filesystem::path> paths;
        for(boost::filesystem::directory_iterator it(default_build_path), end; it != end; ++it)
          paths.emplace_back(*it);
        for(auto &path : paths)
          boost::filesystem::remove_all(path);
      }
      if(has_debug_build && boost::filesystem::exists(debug_build_path)) {
        std::vector<boost::filesystem::path> paths;
        for(boost::filesystem::directory_iterator it(debug_build_path), end; it != end; ++it)
          paths.emplace_back(*it);
        for(auto &path : paths)
          boost::filesystem::remove_all(path);
      }
    }
    catch(const std::exception &e) {
      Terminal::get().print(std::string("\e[31mError\e[m: could not remove build: ") + e.what() + "\n", true);
      return;
    }
  }

  build->update_default(true);
  if(has_debug_build)
    build->update_debug(true);

  for(size_t c = 0; c < Notebook::get().size(); c++) {
    auto source_view = Notebook::get().get_view(c);
    if(auto source_clang_view = dynamic_cast<Source::ClangView *>(source_view)) {
      if(filesystem::file_in_path(source_clang_view->file_path, build->project_path))
        source_clang_view->full_reparse_needed = true;
    }
  }

  if(auto view = Notebook::get().get_current_view()) {
    if(view->full_reparse_needed)
      view->full_reparse();
  }
}


void Project::Markdown::compile_and_run() {
  if(auto view = Notebook::get().get_current_view()) {
    auto command = Config::get().project.markdown_command + ' ' + filesystem::escape_argument(filesystem::get_short_path(view->file_path).string());
    Terminal::get().async_process(
        command, "", [command](int exit_status) {
          if(exit_status == 127)
            Terminal::get().print("\e[31mError\e[m: executable not found: " + command + "\n", true);
        },
        true);
  }
}

void Project::Python::compile_and_run() {
  std::string command = Config::get().project.python_command + ' ';
  boost::filesystem::path path;
  if(dynamic_cast<PythonMain *>(build.get())) {
    command += filesystem::get_short_path(build->project_path).string();
    path = build->project_path;
  }
  else if(auto view = Notebook::get().get_current_view()) {
    command += filesystem::escape_argument(filesystem::get_short_path(view->file_path).string());
    path = view->file_path.parent_path();
  }
  else
    return;

  if(Config::get().terminal.clear_on_compile)
    Terminal::get().clear();

  Terminal::get().print("\e[2mRunning: " + command + "\e[m\n");
  Terminal::get().async_process(command, path, [command](int exit_status) {
    Terminal::get().print("\e[2m" + command + " returned: " + (exit_status == 0 ? "\e[32m" : "\e[31m") + std::to_string(exit_status) + "\e[m\n");
  });
}

void Project::JavaScript::compile_and_run() {
  std::string command;
  boost::filesystem::path path;
  if(dynamic_cast<NpmBuild *>(build.get())) {
    command = "npm start";
    path = build->project_path;
  }
  else if(auto view = Notebook::get().get_current_view()) {
    command = "node " + filesystem::escape_argument(filesystem::get_short_path(view->file_path).string());
    path = view->file_path.parent_path();
  }
  else
    return;

  if(Config::get().terminal.clear_on_compile)
    Terminal::get().clear();

  Terminal::get().print("\e[2mRunning: " + command + "\e[m\n");
  Terminal::get().async_process(command, path, [command](int exit_status) {
    Terminal::get().print("\e[2m" + command + " returned: " + (exit_status == 0 ? "\e[32m" : "\e[31m") + std::to_string(exit_status) + "\e[m\n");
  });
}

void Project::HTML::compile_and_run() {
  if(dynamic_cast<NpmBuild *>(build.get())) {
    std::string command = "npm start";

    if(Config::get().terminal.clear_on_compile)
      Terminal::get().clear();

    Terminal::get().print("\e[2mRunning: " + command + "\e[m\n");
    Terminal::get().async_process(command, build->project_path, [command](int exit_status) {
      Terminal::get().print("\e[2m" + command + " returned: " + (exit_status == 0 ? "\e[32m" : "\e[31m") + std::to_string(exit_status) + "\e[m\n");
    });
  }
  else if(auto view = Notebook::get().get_current_view())
    Notebook::get().open_uri(std::string("file://") + view->file_path.string());
}

std::pair<std::string, std::string> Project::Rust::get_run_arguments() {
  auto project_path = build->project_path.string();
  auto run_arguments_it = run_arguments.find(project_path);
  std::string arguments;
  if(run_arguments_it != run_arguments.end())
    arguments = run_arguments_it->second;

  if(arguments.empty())
    arguments = filesystem::escape_argument(filesystem::get_short_path(build->get_executable(project_path)).string());

  return {project_path, arguments};
}

void Project::Rust::compile() {
  compiling = true;

  if(Config::get().terminal.clear_on_compile)
    Terminal::get().clear();

  Terminal::get().print("\e[2mCompiling project: " + filesystem::get_short_path(build->project_path).string() + "\e[m\n");

  Terminal::get().async_process(build->get_compile_command(), build->project_path, [](int exit_status) {
    compiling = false;
  });
}

void Project::Rust::compile_and_run() {
  compiling = true;

  if(Config::get().terminal.clear_on_compile)
    Terminal::get().clear();

  auto arguments = get_run_arguments().second;
  Terminal::get().print("\e[2mCompiling and running: " + arguments + "\e[m\n");

  auto self = this->shared_from_this();
  Terminal::get().async_process(build->get_compile_command(), build->project_path, [self, arguments = std::move(arguments)](int exit_status) {
    compiling = false;
    if(exit_status == 0) {
      Terminal::get().async_process(arguments, self->build->project_path, [arguments](int exit_status) {
        Terminal::get().print("\e[2m" + arguments + " returned: " + (exit_status == 0 ? "\e[32m" : "\e[31m") + std::to_string(exit_status) + "\e[m\n");
      });
    }
  });
}

void Project::Go::compile_and_run() {
  std::string command;
  boost::filesystem::path path;
  if(dynamic_cast<GoBuild *>(build.get())) {
    command = "go run .";
    path = build->project_path;
  }
  else if(auto view = Notebook::get().get_current_view()) {
    command = "go run " + filesystem::escape_argument(filesystem::get_short_path(view->file_path).string());
    path = view->file_path.parent_path();
  }
  else
    return;

  if(Config::get().terminal.clear_on_compile)
    Terminal::get().clear();

  Terminal::get().print("\e[2mRunning: " + command + "\e[m\n");
  Terminal::get().async_process(command, path, [command](int exit_status) {
    Terminal::get().print("\e[2m" + command + " returned: " + (exit_status == 0 ? "\e[32m" : "\e[31m") + std::to_string(exit_status) + "\e[m\n");
  });
}

void Project::Julia::compile_and_run() {
  if(auto view = Notebook::get().get_current_view()) {
    auto command = "julia " + filesystem::escape_argument(filesystem::get_short_path(Notebook::get().get_current_view()->file_path).string());
    auto path = view->file_path.parent_path();

    if(Config::get().terminal.clear_on_compile)
      Terminal::get().clear();

    Terminal::get().print("\e[2mRunning: " + command + "\e[m\n");
    Terminal::get().async_process(command, path, [command](int exit_status) {
      Terminal::get().print("\e[2m" + command + " returned: " + (exit_status == 0 ? "\e[32m" : "\e[31m") + std::to_string(exit_status) + "\e[m\n");
    });
  }
}
