#include "dialog.hpp"
#include "filesystem.hpp"
#include <cmath>

Dialog::Message::Message(const std::string &text, std::function<void()> &&on_cancel, bool show_progress_bar) : Gtk::Window(Gtk::WindowType::WINDOW_POPUP) {
  set_transient_for(*Glib::RefPtr<Gtk::Application>::cast_dynamic(Gtk::Application::get_default())->get_active_window());

  set_position(Gtk::WindowPosition::WIN_POS_CENTER_ON_PARENT);
  set_modal(true);
  set_type_hint(Gdk::WindowTypeHint::WINDOW_TYPE_HINT_NOTIFICATION);
  property_decorated() = false;
  set_skip_taskbar_hint(true);
  auto visual = get_screen()->get_rgba_visual();
  if(visual)
    gtk_widget_set_visual(reinterpret_cast<GtkWidget *>(gobj()), visual->gobj());
  get_style_context()->add_class("juci_message_window");

  auto vbox = Gtk::manage(new Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL));
  vbox->get_style_context()->add_class("juci_message_box");
  auto hbox = Gtk::manage(new Gtk::Box(Gtk::Orientation::ORIENTATION_HORIZONTAL));
  auto image = Gtk::manage(new Gtk::Image());
  image->set_from_icon_name("dialog-information", Gtk::BuiltinIconSize::ICON_SIZE_BUTTON);
  hbox->pack_start(*image, Gtk::PackOptions::PACK_SHRINK);
  auto label = Gtk::manage(new Gtk::Label(text));
  label->set_padding(7, 7);
  hbox->pack_start(*label);
  vbox->pack_start(*hbox);
  if(on_cancel) {
    auto cancel_button = Gtk::manage(new Gtk::Button("Cancel"));
    cancel_button->signal_clicked().connect([label, on_cancel = std::move(on_cancel)] {
      label->set_text("Canceling...");
      if(on_cancel)
        on_cancel();
    });
    vbox->pack_start(*cancel_button);
  }
  if(show_progress_bar)
    vbox->pack_start(progress_bar);
  add(*vbox);
  show_all_children();
  show_now();

  while(Gtk::Main::events_pending())
    Gtk::Main::iteration();
}

void Dialog::Message::set_fraction(double fraction) {
  progress_bar.set_fraction(fraction);
}

bool Dialog::Message::on_delete_event(GdkEventAny *event) {
  return true;
}

std::string Dialog::gtk_dialog(const boost::filesystem::path &path, const std::string &title,
                               const std::vector<std::pair<std::string, Gtk::ResponseType>> &buttons,
                               Gtk::FileChooserAction action) {
  // Workaround for crash on MacOS when filtering files in file/folder dialogs.
  // See also https://github.com/cppit/jucipp/issues/259.
  // TODO 2018: check if this bug has been fixed
#ifdef __APPLE__
  class FileChooserDialog : public Gtk::FileChooserDialog {
    Gtk::FileChooserAction action;

  public:
    FileChooserDialog(const Glib::ustring &title, Gtk::FileChooserAction action) : Gtk::FileChooserDialog(title, action), action(action) {}

  protected:
    bool on_key_press_event(GdkEventKey *event) override {
      if(action == Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN || action == Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SELECT_FOLDER) {
        auto unicode = gdk_keyval_to_unicode(event->keyval);
        if(unicode > 31 && unicode != 127)
          return true;
      }
      return Gtk::FileChooserDialog::on_key_press_event(event);
    }
  };
  FileChooserDialog dialog(title, action);
#else
  Gtk::FileChooserDialog dialog(title, action);
#endif

  dialog.set_transient_for(*Glib::RefPtr<Gtk::Application>::cast_dynamic(Gtk::Application::get_default())->get_active_window());
  dialog.set_position(Gtk::WindowPosition::WIN_POS_CENTER_ON_PARENT);

  if(title == "Save File As")
    gtk_file_chooser_set_filename(reinterpret_cast<GtkFileChooser *>(dialog.gobj()), path.string().c_str());
  else if(!path.empty())
    gtk_file_chooser_set_current_folder(reinterpret_cast<GtkFileChooser *>(dialog.gobj()), path.string().c_str());
  else {
    auto current_path = filesystem::get_current_path();
    if(!current_path.empty())
      gtk_file_chooser_set_current_folder(reinterpret_cast<GtkFileChooser *>(dialog.gobj()), current_path.string().c_str());
  }

  for(auto &button : buttons)
    dialog.add_button(button.first, button.second);
  return dialog.run() == Gtk::RESPONSE_OK ? dialog.get_filename() : "";
}

std::string Dialog::open_folder(const boost::filesystem::path &path) {
  return gtk_dialog(path, "Open Folder",
                    {std::make_pair("Cancel", Gtk::RESPONSE_CANCEL), std::make_pair("Open", Gtk::RESPONSE_OK)},
                    Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
}

std::string Dialog::new_file(const boost::filesystem::path &path) {
  return gtk_dialog(path, "New File",
                    {std::make_pair("Cancel", Gtk::RESPONSE_CANCEL), std::make_pair("Save", Gtk::RESPONSE_OK)},
                    Gtk::FILE_CHOOSER_ACTION_SAVE);
}

std::string Dialog::new_folder(const boost::filesystem::path &path) {
  return gtk_dialog(path, "New Folder",
                    {std::make_pair("Cancel", Gtk::RESPONSE_CANCEL), std::make_pair("Create", Gtk::RESPONSE_OK)},
                    Gtk::FILE_CHOOSER_ACTION_CREATE_FOLDER);
}

std::string Dialog::open_file(const boost::filesystem::path &path) {
  return gtk_dialog(path, "Open File",
                    {std::make_pair("Cancel", Gtk::RESPONSE_CANCEL), std::make_pair("Select", Gtk::RESPONSE_OK)},
                    Gtk::FILE_CHOOSER_ACTION_OPEN);
}

std::string Dialog::save_file_as(const boost::filesystem::path &path) {
  return gtk_dialog(path, "Save File As",
                    {std::make_pair("Cancel", Gtk::RESPONSE_CANCEL), std::make_pair("Save", Gtk::RESPONSE_OK)},
                    Gtk::FILE_CHOOSER_ACTION_SAVE);
}
