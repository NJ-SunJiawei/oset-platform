#include "source_diff.hpp"
#include "config.hpp"
#include "filesystem.hpp"
#include "info.hpp"
#include "terminal.hpp"
#include <boost/version.hpp>

Source::DiffView::Renderer::Renderer() : Gsv::GutterRenderer() {
  set_padding(4, 0);
}

void Source::DiffView::Renderer::draw_vfunc(const Cairo::RefPtr<Cairo::Context> &cr, const Gdk::Rectangle &background_area,
                                            const Gdk::Rectangle &cell_area, Gtk::TextIter &start, Gtk::TextIter &end,
                                            Gsv::GutterRendererState p6) {
  if(start.has_tag(tag_added) || end.has_tag(tag_added)) {
    cr->set_source_rgba(green.get_red(), green.get_green(), green.get_blue(), green.get_alpha());
    cr->rectangle(cell_area.get_x(), cell_area.get_y(), 4, cell_area.get_height());
    cr->fill();
  }
  else if(start.has_tag(tag_modified) || end.has_tag(tag_modified)) {
    cr->set_source_rgba(yellow.get_red(), yellow.get_green(), yellow.get_blue(), yellow.get_alpha());
    cr->rectangle(cell_area.get_x(), cell_area.get_y(), 4, cell_area.get_height());
    cr->fill();
  }
  if(start.has_tag(tag_removed_below) || end.has_tag(tag_removed_below)) {
    cr->set_source_rgba(red.get_red(), red.get_green(), red.get_blue(), red.get_alpha());
    cr->rectangle(cell_area.get_x() - 4, cell_area.get_y() + cell_area.get_height() - 2, 8, 2);
    cr->fill();
  }
  if(start.has_tag(tag_removed_above) || end.has_tag(tag_removed_above)) {
    cr->set_source_rgba(red.get_red(), red.get_green(), red.get_blue(), red.get_alpha());
    cr->rectangle(cell_area.get_x() - 4, cell_area.get_y(), 8, 2);
    cr->fill();
  }
}

Source::DiffView::DiffView(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language) : BaseView(file_path, language), renderer(new Renderer()) {
  canonical_file_path = filesystem::get_canonical_path(file_path);

  renderer->tag_added = get_buffer()->create_tag("git_added");
  renderer->tag_modified = get_buffer()->create_tag("git_modified");
  renderer->tag_removed = get_buffer()->create_tag("git_removed");
  renderer->tag_removed_below = get_buffer()->create_tag();
  renderer->tag_removed_above = get_buffer()->create_tag();
}

Source::DiffView::~DiffView() {
  if(repository) {
    get_gutter(Gtk::TextWindowType::TEXT_WINDOW_LEFT)->remove(renderer.get());
    buffer_insert_connection.disconnect();
    buffer_erase_connection.disconnect();
    monitor_changed_connection.disconnect();
    delayed_buffer_changed_connection.disconnect();
    delayed_monitor_changed_connection.disconnect();

    parse_stop = true;
    if(parse_thread.joinable())
      parse_thread.join();
  }
}

void Source::DiffView::configure() {
  // Set colors
  auto &yellow = renderer->yellow;
  auto &red = renderer->red;
  auto &green = renderer->green;
  auto normal_color = get_style_context()->get_color(Gtk::StateFlags::STATE_FLAG_NORMAL);
  auto light_theme = (normal_color.get_red() + normal_color.get_green() + normal_color.get_blue()) / 3 < 0.5;
  yellow.set_rgba(1.0, 1.0, 0.2);
  double factor = light_theme ? 0.85 : 0.5;
  yellow.set_red(normal_color.get_red() + factor * (yellow.get_red() - normal_color.get_red()));
  yellow.set_green(normal_color.get_green() + factor * (yellow.get_green() - normal_color.get_green()));
  yellow.set_blue(normal_color.get_blue() + factor * (yellow.get_blue() - normal_color.get_blue()));
  red.set_rgba(1.0, 0.0, 0.0);
  factor = light_theme ? 0.8 : 0.35;
  red.set_red(normal_color.get_red() + factor * (red.get_red() - normal_color.get_red()));
  red.set_green(normal_color.get_green() + factor * (red.get_green() - normal_color.get_green()));
  red.set_blue(normal_color.get_blue() + factor * (red.get_blue() - normal_color.get_blue()));
  green.set_rgba(0.0, 1.0, 0.0);
  factor = light_theme ? 0.7 : 0.4;
  green.set_red(normal_color.get_red() + factor * (green.get_red() - normal_color.get_red()));
  green.set_green(normal_color.get_green() + factor * (green.get_green() - normal_color.get_green()));
  green.set_blue(normal_color.get_blue() + factor * (green.get_blue() - normal_color.get_blue()));

  if(Config::get().source.show_git_diff) {
    if(repository)
      return;
  }
  else if(repository) {
    get_gutter(Gtk::TextWindowType::TEXT_WINDOW_LEFT)->remove(renderer.get());
    buffer_insert_connection.disconnect();
    buffer_erase_connection.disconnect();
    monitor_changed_connection.disconnect();
    delayed_buffer_changed_connection.disconnect();
    delayed_monitor_changed_connection.disconnect();

    parse_stop = true;
    if(parse_thread.joinable())
      parse_thread.join();
    repository = nullptr;

    LockGuard lock(parse_mutex);
    diff = nullptr;
    return;
  }
  else
    return;

  try {
    repository = Git::get_repository(this->file_path.parent_path());
  }
  catch(const std::exception &) {
    return;
  }

  get_gutter(Gtk::TextWindowType::TEXT_WINDOW_LEFT)->insert(renderer.get(), -40);
  parse_state = ParseState::starting;
  parse_stop = false;
  monitor_changed = false;

  buffer_insert_connection = get_buffer()->signal_insert().connect(
      [this](const Gtk::TextIter &iter, const Glib::ustring &text, int) {
        // Do not perform git diff if no newline is added and line is already marked as added
        if(!iter.starts_line() && iter.has_tag(renderer->tag_added)) {
          bool newline = false;
          for(auto &c : text.raw()) {
            if(c == '\n') {
              newline = true;
              break;
            }
          }
          if(!newline)
            return;
        }
        // Remove tag_removed_above/below if newline is inserted
        else if(!text.empty() && text[0] == '\n' && iter.has_tag(renderer->tag_removed)) {
          auto start_iter = get_buffer()->get_iter_at_line(iter.get_line());
          auto end_iter = get_iter_at_line_end(iter.get_line());
          end_iter.forward_char();
          get_buffer()->remove_tag(renderer->tag_removed_above, start_iter, end_iter);
          get_buffer()->remove_tag(renderer->tag_removed_below, start_iter, end_iter);
        }
        parse_state = ParseState::idle;
        delayed_buffer_changed_connection.disconnect();
        delayed_buffer_changed_connection = Glib::signal_timeout().connect(
            [this]() {
              parse_state = ParseState::starting;
              return false;
            },
            250);
      },
      false);

  buffer_erase_connection = get_buffer()->signal_erase().connect(
      [this](const Gtk::TextIter &start_iter, const Gtk::TextIter &end_iter) {
        // Do not perform git diff if start_iter and end_iter is at the same line in addition to the line is tagged added
        if(start_iter.get_line() == end_iter.get_line() && start_iter.has_tag(renderer->tag_added))
          return;

        parse_state = ParseState::idle;
        delayed_buffer_changed_connection.disconnect();
        delayed_buffer_changed_connection = Glib::signal_timeout().connect(
            [this]() {
              parse_state = ParseState::starting;
              return false;
            },
            250);
      },
      false);

  monitor_changed_connection = repository->monitor->signal_changed().connect([this](const Glib::RefPtr<Gio::File> &file,
                                                                                    const Glib::RefPtr<Gio::File> &,
                                                                                    Gio::FileMonitorEvent monitor_event) {
    if(monitor_event != Gio::FileMonitorEvent::FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
      delayed_monitor_changed_connection.disconnect();
      delayed_monitor_changed_connection = Glib::signal_timeout().connect(
          [this]() {
            monitor_changed = true;
            parse_state = ParseState::starting;
            LockGuard lock(parse_mutex);
            diff = nullptr;
            return false;
          },
          500);
    }
  });

  parse_thread = std::thread([this]() {
    std::string status_branch;
    try {
      {
        LockGuard lock(parse_mutex);
        diff = get_diff();
      }
      status_branch = repository->get_branch();
    }
    catch(const std::exception &) {
      status_branch = "";
    }
    dispatcher.post([this, status_branch = std::move(status_branch)] {
      this->status_branch = status_branch;
      if(update_status_branch)
        update_status_branch(this);
    });

    try {
      while(true) {
        while(!parse_stop && parse_state != ParseState::starting && parse_state != ParseState::processing)
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if(parse_stop)
          break;
        auto expected = ParseState::starting;
        if(parse_state.compare_exchange_strong(expected, ParseState::preprocessing)) {
          dispatcher.post([this] {
            auto expected = ParseState::preprocessing;
            if(parse_mutex.try_lock()) {
              if(parse_state.compare_exchange_strong(expected, ParseState::processing))
                parse_buffer = get_buffer()->get_text();
              parse_mutex.unlock();
            }
            else
              parse_state.compare_exchange_strong(expected, ParseState::starting);
          });
        }
        else if(parse_state == ParseState::processing && parse_mutex.try_lock()) {
          bool expected_monitor_changed = true;
          if(monitor_changed.compare_exchange_strong(expected_monitor_changed, false)) {
            try {
              diff = get_diff();
              dispatcher.post([this, status_branch = repository->get_branch()] {
                this->status_branch = status_branch;
                if(update_status_branch)
                  update_status_branch(this);
              });
            }
            catch(const std::exception &) {
              dispatcher.post([this] {
                get_buffer()->remove_tag(renderer->tag_added, get_buffer()->begin(), get_buffer()->end());
                get_buffer()->remove_tag(renderer->tag_modified, get_buffer()->begin(), get_buffer()->end());
                get_buffer()->remove_tag(renderer->tag_removed, get_buffer()->begin(), get_buffer()->end());
                get_buffer()->remove_tag(renderer->tag_removed_below, get_buffer()->begin(), get_buffer()->end());
                get_buffer()->remove_tag(renderer->tag_removed_above, get_buffer()->begin(), get_buffer()->end());
                renderer->queue_draw();
                this->status_branch = "";
                if(update_status_branch)
                  update_status_branch(this);
              });
            }
          }

          Git::Repository::Diff::Lines diff_lines;
          if(diff)
            diff_lines = diff->get_lines(parse_buffer.raw());
          auto expected = ParseState::processing;
          if(parse_state.compare_exchange_strong(expected, ParseState::postprocessing)) {
            parse_mutex.unlock();
            dispatcher.post([this, diff_lines = std::move(diff_lines)] {
              auto expected = ParseState::postprocessing;
              if(parse_state.compare_exchange_strong(expected, ParseState::idle))
                update_tags(diff_lines);
            });
          }
          else
            parse_mutex.unlock();
        }
      }
    }
    catch(const std::exception &e) {
      dispatcher.post([this, e_what = e.what()] {
        get_buffer()->remove_tag(renderer->tag_added, get_buffer()->begin(), get_buffer()->end());
        get_buffer()->remove_tag(renderer->tag_modified, get_buffer()->begin(), get_buffer()->end());
        get_buffer()->remove_tag(renderer->tag_removed, get_buffer()->begin(), get_buffer()->end());
        get_buffer()->remove_tag(renderer->tag_removed_below, get_buffer()->begin(), get_buffer()->end());
        get_buffer()->remove_tag(renderer->tag_removed_above, get_buffer()->begin(), get_buffer()->end());
        renderer->queue_draw();
        Terminal::get().print(std::string("\e[31mError (git)\e[m: ") + e_what + '\n', true);
      });
    }
  });
}

void Source::DiffView::rename(const boost::filesystem::path &path) {
  Source::BaseView::rename(path);

  LockGuard lock(canonical_file_path_mutex);
  canonical_file_path = filesystem::get_canonical_path(path);
}

void Source::DiffView::goto_next_diff() {
  auto iter = get_buffer()->get_insert()->get_iter();
  auto insert_iter = iter;
  bool wrapped = false;
  iter.forward_char();
  for(;;) {
    auto toggled_tags = iter.get_toggled_tags();
    for(auto &toggled_tag : toggled_tags) {
      if(toggled_tag->property_name() == "git_added" ||
         toggled_tag->property_name() == "git_modified" ||
         toggled_tag->property_name() == "git_removed") {
        get_buffer()->place_cursor(iter);
        scroll_to(get_buffer()->get_insert(), 0.0, 1.0, 0.5);
        return;
      }
    }
    if(wrapped && (iter == insert_iter || iter == get_buffer()->end()))
      break;
    if(!wrapped && iter == get_buffer()->end()) {
      iter = get_buffer()->begin();
      wrapped = true;
    }
    else
      iter.forward_char();
  }
  Info::get().print("No changes found in current buffer");
}

std::string Source::DiffView::get_diff_details() {
  std::string details;
  {
    LockGuard lock(parse_mutex);
    if(diff) {
      auto line_nr = get_buffer()->get_insert()->get_iter().get_line();
      auto iter = get_buffer()->get_iter_at_line(line_nr);
      if(iter.has_tag(renderer->tag_removed_above))
        --line_nr;
      parse_buffer = get_buffer()->get_text();
      details = diff->get_details(parse_buffer.raw(), line_nr);
    }
  }
  if(details.empty())
    Info::get().print("No changes found at current line");
  return details;
}

/// Return repository diff instance. Throws exception on error
std::unique_ptr<Git::Repository::Diff> Source::DiffView::get_diff() {
  auto work_path = filesystem::get_normal_path(repository->get_work_path());
  boost::filesystem::path relative_path;
  {
    LockGuard lock(canonical_file_path_mutex);
    if(!filesystem::file_in_path(canonical_file_path, work_path))
      throw std::runtime_error("not a relative path");
    relative_path = filesystem::get_relative_path(canonical_file_path, work_path);
  }
  return std::make_unique<Git::Repository::Diff>(repository->get_diff(relative_path));
}

void Source::DiffView::update_tags(const Git::Repository::Diff::Lines &diff_lines) {
  get_buffer()->remove_tag(renderer->tag_added, get_buffer()->begin(), get_buffer()->end());
  get_buffer()->remove_tag(renderer->tag_modified, get_buffer()->begin(), get_buffer()->end());
  get_buffer()->remove_tag(renderer->tag_removed, get_buffer()->begin(), get_buffer()->end());
  get_buffer()->remove_tag(renderer->tag_removed_below, get_buffer()->begin(), get_buffer()->end());
  get_buffer()->remove_tag(renderer->tag_removed_above, get_buffer()->begin(), get_buffer()->end());

  for(auto &added : diff_lines.added) {
    auto start_iter = get_buffer()->get_iter_at_line(added.first);
    auto end_iter = get_iter_at_line_end(added.second - 1);
    end_iter.forward_char();
    get_buffer()->apply_tag(renderer->tag_added, start_iter, end_iter);
  }
  for(auto &modified : diff_lines.modified) {
    auto start_iter = get_buffer()->get_iter_at_line(modified.first);
    auto end_iter = get_iter_at_line_end(modified.second - 1);
    end_iter.forward_char();
    get_buffer()->apply_tag(renderer->tag_modified, start_iter, end_iter);
  }
  for(auto &line_nr : diff_lines.removed) {
    Gtk::TextIter removed_start, removed_end;
    if(line_nr >= 0) {
      auto start_iter = get_buffer()->get_iter_at_line(line_nr);
      removed_start = start_iter;
      auto end_iter = get_iter_at_line_end(line_nr);
      end_iter.forward_char();
      removed_end = end_iter;
      get_buffer()->apply_tag(renderer->tag_removed_below, start_iter, end_iter);
    }
    if(line_nr + 1 < get_buffer()->get_line_count()) {
      auto start_iter = get_buffer()->get_iter_at_line(line_nr + 1);
      if(line_nr < 0)
        removed_start = start_iter;
      auto end_iter = get_iter_at_line_end(line_nr + 1);
      end_iter.forward_char();
      removed_end = end_iter;
      get_buffer()->apply_tag(renderer->tag_removed_above, start_iter, end_iter);
    }
    get_buffer()->apply_tag(renderer->tag_removed, removed_start, removed_end);
  }

  renderer->queue_draw();
}
