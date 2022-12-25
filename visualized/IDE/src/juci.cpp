#include "juci.hpp"
#include "config.hpp"
#include "directories.hpp"
#include "filesystem.hpp"
#include "menu.hpp"
#include "notebook.hpp"
#include "terminal.hpp"
#include "utility.hpp"
#include "window.hpp"
#ifndef _WIN32
#include <csignal>
#endif
#ifdef __APPLE__
#include "dispatcher.hpp"
#include "window_macos.hpp"
#endif

int Application::on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine> &cmd) {
  Glib::set_prgname("juci");
  Glib::OptionContext ctx("[PATH ...]");
  Glib::OptionGroup gtk_group(gtk_get_option_group(true));
  ctx.add_group(gtk_group);
  int argc;
  char **argv = cmd->get_arguments(argc);
  ctx.parse(argc, argv);
  if(argc >= 2) {
    auto current_path = filesystem::get_current_path();
    if(current_path.empty())
      errors.emplace_back("\e[31mError\e[m: could not find current path\n");
    for(int c = 1; c < argc; c++) {
      boost::filesystem::path path(argv[c]);
      if(path.is_relative() && !current_path.empty())
        path = current_path / path;
      boost::system::error_code ec;
      if(boost::filesystem::exists(path, ec)) {
        if(boost::filesystem::is_regular_file(path, ec))
          files.emplace_back(path, 0);
        else if(boost::filesystem::is_directory(path, ec))
          directories.emplace_back(path);
      }
      // Open new file if parent path exists
      else {
        if(path.is_absolute() && boost::filesystem::is_directory(path.parent_path(), ec))
          files.emplace_back(path, 0);
        else
          errors.emplace_back("\e[31mError\e[m: could not create " + path.string() + ".\n");
      }
    }
  }
  activate();
  return 0;
}

void Application::on_activate() {
  std::vector<std::pair<int, int>> file_offsets;
  boost::filesystem::path current_file;
  Window::get().load_session(directories, files, file_offsets, current_file, directories.empty() && files.empty());

  Window::get().add_widgets();

  add_window(Window::get());
  Window::get().show();

  bool first_directory = true;
  for(auto &directory : directories) {
    if(first_directory) {
      Directories::get().open(directory);
      first_directory = false;
    }
    else {
      std::string files_in_directory;
      for(auto it = files.begin(); it != files.end();) {
        if(filesystem::file_in_path(it->first, directory)) {
          files_in_directory += " " + filesystem::escape_argument(it->first.string());
          it = files.erase(it);
        }
        else
          it++;
      }
      std::thread another_juci_app([directory, files_in_directory]() {
        auto command = "juci " + filesystem::escape_argument(directory.string()) + files_in_directory;
        Terminal::get().async_print("Executing: " + command + "\n");
        Terminal::get().process(command, "", false);
      });
      another_juci_app.detach();
    }
  }

  for(size_t i = 0; i < files.size(); ++i) {
    if(Notebook::get().open(files[i].first, files[i].second == 0 ? Notebook::Position::left : Notebook::Position::right)) {
      if(i < file_offsets.size()) {
        auto view = Notebook::get().get_current_view();
        view->place_cursor_at_line_offset(file_offsets[i].first, file_offsets[i].second);
        view->scroll_to_cursor_delayed(true, false);
      }
    }
  }

  for(auto &error : errors)
    Terminal::get().print(std::move(error), true);

  if(!current_file.empty()) {
    if(Notebook::get().open(current_file)) {
      auto view = Notebook::get().get_current_view();
      auto iter = view->get_buffer()->get_insert()->get_iter();
      // To update cursor history
      view->place_cursor_at_line_offset(iter.get_line(), iter.get_line_offset());
      view->scroll_to_cursor_delayed(true, false);
    }
  }

#ifdef __APPLE__
  static Dispatcher dispatcher;
  dispatcher.post([] {
    macos_force_foreground_level(); // Must run after all other events
  });
#endif
}

void Application::on_startup() {
  Gtk::Application::on_startup();

  Menu::get().build();

  if(!Menu::get().juci_menu || !Menu::get().window_menu) {
    std::cerr << "Menu not found." << std::endl;
  }
  else {
    set_app_menu(Menu::get().juci_menu);
    set_menubar(Menu::get().window_menu);
  }
}

Application::Application() : Gtk::Application("no.sout.juci", Gio::APPLICATION_NON_UNIQUE | Gio::APPLICATION_HANDLES_COMMAND_LINE) {
  Glib::set_application_name("System");

  // Gtk::MessageDialog without buttons caused text to be selected, this prevents that
  Gtk::Settings::get_default()->property_gtk_label_select_on_focus() = false;
}

int main(int argc, char *argv[]) {
#ifndef _WIN32
  signal(SIGPIPE, SIG_IGN); // Do not terminate application when writing to a process fails
#endif
  return Application().run(argc, argv);
}
