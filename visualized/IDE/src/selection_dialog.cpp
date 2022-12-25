#include "selection_dialog.hpp"
#include <algorithm>

SelectionDialogBase::ListViewText::ListViewText(bool use_markup) : Gtk::TreeView(), use_markup(use_markup) {
  list_store = Gtk::ListStore::create(column_record);
  set_model(list_store);
  append_column("", cell_renderer);
  if(use_markup)
    get_column(0)->add_attribute(cell_renderer.property_markup(), column_record.text);
  else
    get_column(0)->add_attribute(cell_renderer.property_text(), column_record.text);

  get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
  set_enable_search(true);
  set_headers_visible(false);
  set_hscroll_policy(Gtk::ScrollablePolicy::SCROLL_NATURAL);
  set_activate_on_single_click(true);
  set_hover_selection(false);
}

void SelectionDialogBase::ListViewText::append(const std::string &value) {
  auto new_row = list_store->append();
  new_row->set_value(column_record.text, value);
  new_row->set_value(column_record.index, size++);
}

void SelectionDialogBase::ListViewText::erase_rows() {
  list_store->clear();
  size = 0;
}

void SelectionDialogBase::ListViewText::clear() {
  unset_model();
  list_store.reset();
  size = 0;
}

SelectionDialogBase::SelectionDialogBase(Source::BaseView *view_, const boost::optional<Gtk::TextIter> &start_iter, bool show_search_entry_, bool use_markup)
    : start_mark(start_iter ? Source::Mark(*start_iter) : Source::Mark()), view(view_), window(Gtk::WindowType::WINDOW_POPUP), vbox(Gtk::Orientation::ORIENTATION_VERTICAL), list_view_text(use_markup), show_search_entry(show_search_entry_) {
  window.set_transient_for(*Glib::RefPtr<Gtk::Application>::cast_dynamic(Gtk::Application::get_default())->get_active_window());

  window.set_type_hint(Gdk::WindowTypeHint::WINDOW_TYPE_HINT_COMBO);

  window.get_style_context()->add_class("juci_selection_dialog");

  search_entry.signal_changed().connect(
      [this] {
        if(on_search_entry_changed)
          on_search_entry_changed(search_entry.get_text());
      },
      false);

  list_view_text.set_search_entry(search_entry);

  window.set_default_size(0, 0);
  window.property_decorated() = false;
  window.set_skip_taskbar_hint(true);

  scrolled_window.set_policy(Gtk::PolicyType::POLICY_AUTOMATIC, Gtk::PolicyType::POLICY_AUTOMATIC);

  scrolled_window.add(list_view_text);
  if(show_search_entry)
    vbox.pack_start(search_entry, false, false);
  vbox.pack_start(scrolled_window, true, true);
  window.add(vbox);

  list_view_text.signal_realize().connect([this]() {
    auto application_window = Glib::RefPtr<Gtk::Application>::cast_dynamic(Gtk::Application::get_default())->get_active_window();

    // Calculate window width and height
    int row_width = 0, padding_height = 0, window_height = 0;
    Gdk::Rectangle rect;
    auto children = list_view_text.get_model()->children();
    size_t c = 0;
    for(auto it = children.begin(); it != children.end() && c < 10; ++it) {
      list_view_text.get_cell_area(list_view_text.get_model()->get_path(it), *(list_view_text.get_column(0)), rect);
      if(c == 0) {
        row_width = rect.get_width() + rect.get_x() * 2;
        padding_height = rect.get_y() * 2;
      }
      window_height += rect.get_height() + padding_height;
      ++c;
    }

    if(view && row_width > view->get_width() * 2 / 3)
      row_width = view->get_width() * 2 / 3;
    else if(row_width > application_window->get_width() / 2)
      row_width = application_window->get_width() / 2;

    if(show_search_entry)
      window_height += search_entry.get_height();
    int window_width = row_width + 1;
    window.resize(window_width, window_height);

    auto move_window_to_center = [this, application_window, window_width, window_height] {
      int root_x, root_y;
      application_window->get_position(root_x, root_y);
      root_x += application_window->get_width() / 2 - window_width / 2;
      root_y += application_window->get_height() / 2 - window_height / 2;
      window.move(root_x, root_y);
    };

    if(view) {
      Gdk::Rectangle visible_rect;
      view->get_visible_rect(visible_rect);
      int visible_window_x, visible_window_max_y;
      view->buffer_to_window_coords(Gtk::TextWindowType::TEXT_WINDOW_TEXT, visible_rect.get_x(), visible_rect.get_y() + visible_rect.get_height(), visible_window_x, visible_window_max_y);

      Gdk::Rectangle iter_rect;
      view->get_iter_location(start_mark->get_iter(), iter_rect);
      int buffer_x = iter_rect.get_x();
      int buffer_y = iter_rect.get_y() + iter_rect.get_height();
      int window_x, window_y;
      view->buffer_to_window_coords(Gtk::TextWindowType::TEXT_WINDOW_TEXT, buffer_x, buffer_y, window_x, window_y);

      if(window_y < 0 || window_y > visible_window_max_y) // Move dialog to center if it is above or below visible parts of view
        move_window_to_center();
      else {
        window_x = std::max(window_x, visible_window_x); // Adjust right if dialog is left of view

        int root_x, root_y;
        view->get_window(Gtk::TextWindowType::TEXT_WINDOW_TEXT)->get_root_coords(window_x, window_y, root_x, root_y);

        // Adjust left if dialog is right of screen
        auto screen_width = Gdk::Screen::get_default()->get_width();
        root_x = root_x + window_width > screen_width ? screen_width - window_width : root_x;

        window.move(root_x, root_y + 1); // TODO: replace 1 with some margin
      }
    }
    else
      move_window_to_center();
  });

  list_view_text.signal_cursor_changed().connect([this] {
    cursor_changed();
  });
}

void SelectionDialogBase::cursor_changed() {
  if(!is_visible())
    return;
  auto it = list_view_text.get_selection()->get_selected();
  boost::optional<unsigned int> index;
  if(it)
    index = it->get_value(list_view_text.column_record.index);
  if(last_index == index)
    return;
  if(on_change) {
    std::string text;
    if(it)
      text = it->get_value(list_view_text.column_record.text);
    on_change(index, text);
  }
  last_index = index;
}
void SelectionDialogBase::add_row(const std::string &row) {
  list_view_text.append(row);
}

void SelectionDialogBase::erase_rows() {
  list_view_text.erase_rows();
}

void SelectionDialogBase::show() {
  window.show_all();
  if(view)
    view->grab_focus();

  if(list_view_text.get_model()->children().size() > 0) {
    if(!list_view_text.get_selection()->get_selected()) {
      list_view_text.set_cursor(list_view_text.get_model()->get_path(list_view_text.get_model()->children().begin()));
      cursor_changed();
    }
    else if(list_view_text.get_model()->children().begin() != list_view_text.get_selection()->get_selected()) {
      Glib::signal_idle().connect([this] {
        if((this == SelectionDialog::get().get() || this == CompletionDialog::get().get()) && is_visible())
          list_view_text.scroll_to_row(list_view_text.get_model()->get_path(list_view_text.get_selection()->get_selected()), 0.5);
        return false;
      });
    }
  }
  if(on_show)
    on_show();
}

void SelectionDialogBase::set_cursor_at_last_row() {
  auto children = list_view_text.get_model()->children();
  if(children.size() > 0) {
    list_view_text.set_cursor(list_view_text.get_model()->get_path(children[children.size() - 1]));
    cursor_changed();
  }
}

void SelectionDialogBase::hide() {
  if(!is_visible())
    return;
  window.hide();
  if(on_hide)
    on_hide();
  list_view_text.clear();
  last_index.reset();
}

std::unique_ptr<SelectionDialog> SelectionDialog::instance;

SelectionDialog::SelectionDialog(Source::BaseView *view, const boost::optional<Gtk::TextIter> &start_iter, bool show_search_entry, bool use_markup)
    : SelectionDialogBase(view, start_iter, show_search_entry, use_markup) {
  auto search_text = std::make_shared<std::string>();
  auto filter_model = Gtk::TreeModelFilter::create(list_view_text.get_model());

  filter_model->set_visible_func([this, search_text](const Gtk::TreeModel::const_iterator &iter) {
    std::string row_lc;
    iter->get_value(0, row_lc);
    auto search_text_lc = *search_text;
    std::transform(row_lc.begin(), row_lc.end(), row_lc.begin(), ::tolower);
    std::transform(search_text_lc.begin(), search_text_lc.end(), search_text_lc.begin(), ::tolower);
    if(list_view_text.use_markup) {
      size_t pos = 0;
      while((pos = row_lc.find('<', pos)) != std::string::npos) {
        auto pos2 = row_lc.find('>', pos + 1);
        row_lc.erase(pos, pos2 - pos + 1);
      }
      search_text_lc = Glib::Markup::escape_text(search_text_lc);
    }
    if(row_lc.find(search_text_lc) != std::string::npos)
      return true;
    return false;
  });

  list_view_text.set_model(filter_model);

  list_view_text.set_search_equal_func([](const Glib::RefPtr<Gtk::TreeModel> &model, int column, const Glib::ustring &key, const Gtk::TreeModel::iterator &iter) {
    return false;
  });

  search_entry.signal_changed().connect([this, search_text, filter_model]() {
    *search_text = search_entry.get_text();
    filter_model->refilter();
    list_view_text.set_search_entry(search_entry); // TODO:Report the need of this to GTK's git (bug)
    if(search_text->empty()) {
      if(list_view_text.get_model()->children().size() > 0)
        list_view_text.set_cursor(list_view_text.get_model()->get_path(list_view_text.get_model()->children().begin()));
    }
  });

  auto activate = [this]() {
    auto it = list_view_text.get_selection()->get_selected();
    if(on_select && it)
      on_select(it->get_value(list_view_text.column_record.index), it->get_value(list_view_text.column_record.text), true);
    hide();
  };
  search_entry.signal_activate().connect([activate]() {
    activate();
  });
  list_view_text.signal_row_activated().connect([activate](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *) {
    activate();
  });
}

bool SelectionDialog::on_key_press(GdkEventKey *event) {
  if((event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_KP_Down) && list_view_text.get_model()->children().size() > 0) {
    auto it = list_view_text.get_selection()->get_selected();
    if(it) {
      it++;
      if(it)
        list_view_text.set_cursor(list_view_text.get_model()->get_path(it));
      else
        list_view_text.set_cursor(list_view_text.get_model()->get_path(list_view_text.get_model()->children().begin()));
    }
    return true;
  }
  else if((event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up) && list_view_text.get_model()->children().size() > 0) {
    auto it = list_view_text.get_selection()->get_selected();
    if(it) {
      it--;
      if(it)
        list_view_text.set_cursor(list_view_text.get_model()->get_path(it));
      else {
        auto last_it = list_view_text.get_model()->children().end();
        last_it--;
        if(last_it)
          list_view_text.set_cursor(list_view_text.get_model()->get_path(last_it));
      }
    }
    return true;
  }
  else if(event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter || event->keyval == GDK_KEY_ISO_Left_Tab || event->keyval == GDK_KEY_Tab) {
    auto it = list_view_text.get_selection()->get_selected();
    if(it) {
      auto column = list_view_text.get_column(0);
      list_view_text.row_activated(list_view_text.get_model()->get_path(it), *column);
    }
    else
      hide();
    return true;
  }
  else if(event->keyval == GDK_KEY_Escape) {
    hide();
    return true;
  }
  else if(event->keyval == GDK_KEY_Left || event->keyval == GDK_KEY_KP_Left || event->keyval == GDK_KEY_Right || event->keyval == GDK_KEY_KP_Right) {
    hide();
    return false;
  }
  else if(show_search_entry) {
#ifdef __APPLE__ // OS X bug most likely: Gtk::Entry will not work if window is of type POPUP
    if(event->is_modifier)
      return true;
    else if(event->keyval == GDK_KEY_BackSpace) {
      int start_pos, end_pos;
      if(search_entry.get_selection_bounds(start_pos, end_pos)) {
        search_entry.delete_selection();
        return true;
      }
      auto length = search_entry.get_text_length();
      if(length > 0)
        search_entry.delete_text(length - 1, length);
      return true;
    }
    else if(event->keyval == GDK_KEY_v && event->state & GDK_META_MASK) {
      search_entry.paste_clipboard();
      return true;
    }
    else if(event->keyval == GDK_KEY_c && event->state & GDK_META_MASK) {
      search_entry.copy_clipboard();
      return true;
    }
    else if(event->keyval == GDK_KEY_x && event->state & GDK_META_MASK) {
      search_entry.cut_clipboard();
      return true;
    }
    else if(event->keyval == GDK_KEY_a && event->state & GDK_META_MASK) {
      search_entry.select_region(0, -1);
      return true;
    }
    else {
      search_entry.on_key_press_event(event);
      return true;
    }
#else
    search_entry.on_key_press_event(event);
    return true;
#endif
  }
  hide();
  return false;
}

std::unique_ptr<CompletionDialog> CompletionDialog::instance;

CompletionDialog::CompletionDialog(Source::BaseView *view, const Gtk::TextIter &start_iter) : SelectionDialogBase(view, start_iter, false, false) {
  show_offset = view->get_buffer()->get_insert()->get_iter().get_offset();

  auto search_text = std::make_shared<std::string>();
  auto filter_model = Gtk::TreeModelFilter::create(list_view_text.get_model());
  if(show_offset == start_mark->get_iter().get_offset()) {
    filter_model->set_visible_func([search_text](const Gtk::TreeModel::const_iterator &iter) {
      std::string row_lc;
      iter->get_value(0, row_lc);
      auto search_text_lc = *search_text;
      std::transform(row_lc.begin(), row_lc.end(), row_lc.begin(), ::tolower);
      std::transform(search_text_lc.begin(), search_text_lc.end(), search_text_lc.begin(), ::tolower);
      if(row_lc.find(search_text_lc) != std::string::npos)
        return true;
      return false;
    });
  }
  else {
    filter_model->set_visible_func([search_text](const Gtk::TreeModel::const_iterator &iter) {
      std::string row;
      iter->get_value(0, row);
      if(row.find(*search_text) == 0)
        return true;
      return false;
    });
  }
  list_view_text.set_model(filter_model);
  search_entry.signal_changed().connect([this, search_text, filter_model]() {
    *search_text = search_entry.get_text();
    filter_model->refilter();
    list_view_text.set_search_entry(search_entry); // TODO:Report the need of this to GTK's git (bug)
  });

  list_view_text.signal_row_activated().connect([this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *) {
    select();
  });

  auto text = view->get_buffer()->get_text(start_mark->get_iter(), view->get_buffer()->get_insert()->get_iter());
  if(text.size() > 0) {
    search_entry.set_text(text);
    list_view_text.set_search_entry(search_entry);
  }
}

void CompletionDialog::select(bool hide_window) {
  row_in_entry = true;

  auto it = list_view_text.get_selection()->get_selected();
  if(on_select && it)
    on_select(it->get_value(list_view_text.column_record.index), it->get_value(list_view_text.column_record.text), hide_window);
  if(hide_window)
    hide();
}

bool CompletionDialog::on_key_release(GdkEventKey *event) {
  if(event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_KP_Down || event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up ||
     (event->keyval >= GDK_KEY_Shift_L && event->keyval <= GDK_KEY_Hyper_R))
    return false;

  if(show_offset > view->get_buffer()->get_insert()->get_iter().get_offset())
    hide();
  else {
    auto text = view->get_buffer()->get_text(start_mark->get_iter(), view->get_buffer()->get_insert()->get_iter());
    search_entry.set_text(text);
    list_view_text.set_search_entry(search_entry);
    if(text == "") {
      if(list_view_text.get_model()->children().size() > 0)
        list_view_text.set_cursor(list_view_text.get_model()->get_path(list_view_text.get_model()->children().begin()));
    }
    cursor_changed();
  }
  return false;
}

bool CompletionDialog::on_key_press(GdkEventKey *event) {
  if(view->is_token_char(gdk_keyval_to_unicode(event->keyval)) || event->keyval == GDK_KEY_BackSpace) {
    if(row_in_entry) {
      view->get_buffer()->erase(start_mark->get_iter(), view->get_buffer()->get_insert()->get_iter());
      row_in_entry = false;
      if(event->keyval == GDK_KEY_BackSpace)
        return true;
    }
    return false;
  }
  if(event->keyval == GDK_KEY_Shift_L || event->keyval == GDK_KEY_Shift_R || event->keyval == GDK_KEY_Alt_L || event->keyval == GDK_KEY_Alt_R ||
     event->keyval == GDK_KEY_Control_L || event->keyval == GDK_KEY_Control_R || event->keyval == GDK_KEY_Meta_L || event->keyval == GDK_KEY_Meta_R)
    return false;
  if((event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_KP_Down) && list_view_text.get_model()->children().size() > 0) {
    auto it = list_view_text.get_selection()->get_selected();
    if(it) {
      it++;
      if(it) {
        list_view_text.set_cursor(list_view_text.get_model()->get_path(it));
        cursor_changed();
      }
      else {
        list_view_text.set_cursor(list_view_text.get_model()->get_path(list_view_text.get_model()->children().begin()));
        cursor_changed();
      }
    }
    else
      list_view_text.set_cursor(list_view_text.get_model()->get_path(list_view_text.get_model()->children().begin()));
    select(false);
    return true;
  }
  if((event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up) && list_view_text.get_model()->children().size() > 0) {
    auto it = list_view_text.get_selection()->get_selected();
    if(it) {
      it--;
      if(it) {
        list_view_text.set_cursor(list_view_text.get_model()->get_path(it));
        cursor_changed();
      }
      else {
        auto last_it = list_view_text.get_model()->children().end();
        last_it--;
        if(last_it) {
          list_view_text.set_cursor(list_view_text.get_model()->get_path(last_it));
          cursor_changed();
        }
      }
    }
    else {
      auto last_it = list_view_text.get_model()->children().end();
      last_it--;
      if(last_it)
        list_view_text.set_cursor(list_view_text.get_model()->get_path(last_it));
    }
    select(false);
    return true;
  }
  if(event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter || event->keyval == GDK_KEY_ISO_Left_Tab || event->keyval == GDK_KEY_Tab) {
    select();
    return true;
  }
  hide();
  if(event->keyval == GDK_KEY_Escape)
    return true;
  return false;
}
