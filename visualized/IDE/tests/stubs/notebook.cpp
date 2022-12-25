#include "notebook.hpp"

Notebook::Notebook() {}

Source::View *Notebook::get_current_view() {
  return nullptr;
}

bool Notebook::open(const boost::filesystem::path &file_path, Position position) { return true; }

bool Notebook::open(Source::View *view) { return true; }

void Notebook::open_uri(const std::string &uri) {}

bool Notebook::close(Source::View *view) { return true; }

bool Notebook::close_current() { return true; }
