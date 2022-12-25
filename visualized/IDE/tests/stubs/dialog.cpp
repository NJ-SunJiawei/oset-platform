#include "dialog.hpp"

Dialog::Message::Message(const std::string &text, std::function<void()> &&on_cancel, bool show_progress_bar) : Gtk::Window(Gtk::WindowType::WINDOW_POPUP) {}

void Dialog::Message::set_fraction(double fraction) {}

bool Dialog::Message::on_delete_event(GdkEventAny *event) {
  return true;
}
