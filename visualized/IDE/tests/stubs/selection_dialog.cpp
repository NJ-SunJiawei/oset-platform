#include "selection_dialog.hpp"

SelectionDialogBase::ListViewText::ListViewText(bool use_markup) {}

SelectionDialogBase::SelectionDialogBase(Source::BaseView *view, const boost::optional<Gtk::TextIter> &start_iter, bool show_search_entry, bool use_markup)
    : view(view), list_view_text(use_markup) {}

void SelectionDialogBase::show() {}

void SelectionDialogBase::hide() {}

void SelectionDialogBase::add_row(const std::string &row) {}

std::unique_ptr<SelectionDialog> SelectionDialog::instance;

SelectionDialog::SelectionDialog(Source::BaseView *view, const boost::optional<Gtk::TextIter> &start_iter, bool show_search_entry, bool use_markup)
    : SelectionDialogBase(view, start_iter, show_search_entry, use_markup) {}

bool SelectionDialog::on_key_press(GdkEventKey *event) { return true; }

std::unique_ptr<CompletionDialog> CompletionDialog::instance;

CompletionDialog::CompletionDialog(Source::BaseView *view, const Gtk::TextIter &start_iter)
    : SelectionDialogBase(view, start_iter, false, false) {}

bool CompletionDialog::on_key_press(GdkEventKey *event) { return true; }

bool CompletionDialog::on_key_release(GdkEventKey *event) { return true; }
