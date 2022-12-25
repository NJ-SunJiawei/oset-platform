#include "tooltips.hpp"
#include "config.hpp"
#include "filesystem.hpp"
#include "info.hpp"
#include "notebook.hpp"
#include "selection_dialog.hpp"
#include "utility.hpp"
#include <algorithm>
#include <limits>
#include <regex>

std::set<Tooltip *> Tooltips::shown_tooltips;
Gdk::Rectangle Tooltips::drawn_tooltips_rectangle = Gdk::Rectangle();

Tooltip::Tooltip(Gsv::View *view, const Gtk::TextIter &start_iter, const Gtk::TextIter &end_iter, std::function<void(Tooltip &toolip)> set_buffer_)
    : start_mark(start_iter), end_mark(end_iter), view(view), set_buffer(std::move(set_buffer_)) {}

Tooltip::Tooltip(std::function<void(Tooltip &)> set_buffer_)
    : view(nullptr), set_buffer(std::move(set_buffer_)) {}

Tooltip::~Tooltip() {
  Tooltips::shown_tooltips.erase(this);
}

void Tooltip::update() {
  if(view) {
    auto iter = start_mark->get_iter();
    auto end_iter = end_mark->get_iter();
    view->get_iter_location(iter, activation_rectangle);
    if(iter.get_offset() < end_iter.get_offset()) {
      while(iter.forward_char() && iter != end_iter) {
        Gdk::Rectangle rectangle;
        view->get_iter_location(iter, rectangle);
        activation_rectangle.join(rectangle);
      }
    }
    int location_window_x, location_window_y;
    view->buffer_to_window_coords(Gtk::TextWindowType::TEXT_WINDOW_TEXT, activation_rectangle.get_x(), activation_rectangle.get_y(), location_window_x, location_window_y);
    activation_rectangle.set_x(location_window_x);
    activation_rectangle.set_y(location_window_y);
  }
}

void Tooltip::show(bool disregard_drawn, const std::function<void()> &on_motion) {
  Tooltips::shown_tooltips.emplace(this);

  if(!window) {
    // init window
    window = std::make_unique<Gtk::Window>(Gtk::WindowType::WINDOW_POPUP);

    if(auto active_window = Glib::RefPtr<Gtk::Application>::cast_dynamic(Gtk::Application::get_default())->get_active_window())
      window->set_transient_for(*active_window);

    window->set_type_hint(Gdk::WindowTypeHint::WINDOW_TYPE_HINT_TOOLTIP);

    window->set_events(Gdk::POINTER_MOTION_MASK);
    window->property_decorated() = false;
    window->set_accept_focus(false);
    window->set_skip_taskbar_hint(true);
    window->set_default_size(0, 0);

    window->signal_motion_notify_event().connect([on_motion](GdkEventMotion *event) {
      if(on_motion)
        on_motion();
      return false;
    });

    window->get_style_context()->add_class("juci_tooltip_window");
    auto visual = window->get_screen()->get_rgba_visual();
    if(visual)
      gtk_widget_set_visual(reinterpret_cast<GtkWidget *>(window->gobj()), visual->gobj());

    auto box = Gtk::manage(new Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL));
    box->get_style_context()->add_class("juci_tooltip_box");
    window->add(*box);

    buffer = Gtk::TextBuffer::create();

    if(view) {
      auto create_tag_from_style = [this](const std::string &style_name) {
        if(auto style = view->get_source_buffer()->get_style_scheme()->get_style(style_name)) {
          auto tag = buffer->create_tag(style_name);
          if(style->property_foreground_set())
            tag->property_foreground() = style->property_foreground();
          if(style->property_background_set())
            tag->property_background() = style->property_background();
        }
      };
      create_tag_from_style("def:warning");
      create_tag_from_style("def:error");
    }
    link_tag = buffer->create_tag("link");
    link_tag->property_underline() = Pango::Underline::UNDERLINE_SINGLE;
    link_tag->property_foreground_rgba() = window->get_style_context()->get_color(Gtk::StateFlags::STATE_FLAG_LINK);

    set_buffer(*this);

    remove_trailing_newlines();

    wrap_lines();

    auto tooltip_text_view = Gtk::manage(new Gtk::TextView(buffer));
    tooltip_text_view->get_style_context()->add_class("juci_tooltip_text_view");
    tooltip_text_view->set_editable(false);

    static auto link_mouse_cursor = Gdk::Cursor::create(Gdk::CursorType::HAND1);
    static auto default_mouse_cursor = Gdk::Cursor::create(Gdk::CursorType::XTERM);
    tooltip_text_view->signal_motion_notify_event().connect([this, tooltip_text_view](GdkEventMotion *event) {
      Gtk::TextIter iter;
      int location_x, location_y;
      tooltip_text_view->window_to_buffer_coords(Gtk::TextWindowType::TEXT_WINDOW_TEXT, event->x, event->y, location_x, location_y);
      tooltip_text_view->get_iter_at_location(iter, location_x, location_y);
      if(iter.has_tag(link_tag))
        tooltip_text_view->get_window(Gtk::TextWindowType::TEXT_WINDOW_TEXT)->set_cursor(link_mouse_cursor);
      else
        tooltip_text_view->get_window(Gtk::TextWindowType::TEXT_WINDOW_TEXT)->set_cursor(default_mouse_cursor);
      return false;
    });
    tooltip_text_view->signal_button_press_event().connect([this, tooltip_text_view](GdkEventButton *event) {
      if(event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY) {
        Gtk::TextIter iter;
        int location_x, location_y;
        tooltip_text_view->window_to_buffer_coords(Gtk::TextWindowType::TEXT_WINDOW_TEXT, event->x, event->y, location_x, location_y);
        tooltip_text_view->get_iter_at_location(iter, location_x, location_y);
        if(iter.has_tag(link_tag)) {
          auto start = iter;
          if(!start.begins_tag(link_tag))
            start.backward_to_tag_toggle(link_tag);
          auto end = iter;
          end.forward_to_tag_toggle(link_tag);
          std::string text = tooltip_text_view->get_buffer()->get_text(start, end);
          std::string link;
          for(auto &tag : start.get_tags()) {
            auto it = links.find(tag);
            if(it != links.end()) {
              link = it->second;
              break;
            }
            it = reference_links.find(tag);
            if(it != reference_links.end()) {
              auto reference_it = references.find(it->second);
              if(reference_it != references.end())
                link = reference_it->second;
              break;
            }
          }
          if(link.empty())
            link = text;

          if(starts_with(link, "http://") || starts_with(link, "https://")) {
            Notebook::get().open_uri(link);
            return true;
          }

          const static std::regex regex("^([^:]+):([^:]+)(:[^:]+)?$", std::regex::optimize);
          std::smatch sm;
          if(std::regex_match(link, sm, regex)) {
            auto path = boost::filesystem::path(sm[1].str());
            if(path.is_relative()) {
              if(auto source_view = dynamic_cast<Source::View *>(view))
                path = filesystem::get_normal_path(source_view->file_path.parent_path() / path);
            }

            boost::system::error_code ec;
            if(boost::filesystem::is_regular_file(path, ec)) {
              if(Notebook::get().open(path)) {
                try {
                  auto line = std::stoi(sm[2].str()) - 1;
                  auto offset = sm.length(3) ? std::stoi(sm[3].str().substr(1)) - 1 : 0;
                  auto view = Notebook::get().get_current_view();
                  view->place_cursor_at_line_offset(line, offset);
                  view->scroll_to_cursor_delayed(true, false);
                }
                catch(...) {
                }
              }
              return true;
            }
          }

          auto path = boost::filesystem::path(link);
          if(auto source_view = dynamic_cast<Source::View *>(view))
            path = filesystem::get_normal_path(source_view->file_path.parent_path() / path);
          boost::system::error_code ec;
          if(boost::filesystem::is_regular_file(path, ec) && Notebook::get().open(path))
            return true;
          Info::get().print("Could not open: " + link);
        }
      }
      return false;
    });

    Gdk::Rectangle rect;
    auto iter = buffer->begin();
    tooltip_text_view->get_iter_location(iter, rect);
    int min_x = rect.get_x();
    int max_x = rect.get_x() + rect.get_width();
    int height = 0;
    while(true) {
      if(iter.ends_line()) {
        tooltip_text_view->get_iter_location(iter, rect);
        max_x = std::max(max_x, rect.get_x() + rect.get_width());
        height += rect.get_height();
      }
      if(iter.is_end())
        break;
      iter.forward_char();
    }
    size = {max_x - min_x, height};
    size.first += 6;  // 2xpadding
    size.second += 8; // 2xpadding + 2

    scrolled_window = Gtk::manage(new Gtk::ScrolledWindow());
    scrolled_window->get_style_context()->add_class("juci_tooltip_scrolledwindow");
    scrolled_window->property_hscrollbar_policy() = Gtk::PolicyType::POLICY_NEVER;
    scrolled_window->property_vscrollbar_policy() = Gtk::PolicyType::POLICY_NEVER;
    scrolled_window->add(*tooltip_text_view);

#if GTK_VERSION_GE(3, 20)
    box->add(*scrolled_window);
#else
    auto box2 = Gtk::manage(new Gtk::Box());
    box2->pack_start(*scrolled_window, true, true, 3);
    box->pack_start(*box2, true, true, 3);
#endif

    window->signal_realize().connect([this] {
      if(!view) {
        auto &dialog = SelectionDialog::get();
        if(dialog && dialog->is_visible()) {
          int root_x, root_y;
          dialog->get_position(root_x, root_y);
          root_x -= 3; // -1xpadding
          rectangle.set_x(root_x);
          rectangle.set_y(root_y - size.second);
          if(rectangle.get_y() < 0)
            rectangle.set_y(0);
        }
      }
      window->move(rectangle.get_x(), rectangle.get_y());
    });
  }

  if(buffer->size() == 0)
    return; // Do not show empty tooltips

  int root_x = 0, root_y = 0;
  if(view) {
    Gdk::Rectangle visible_rect;
    view->get_visible_rect(visible_rect);
    int visible_window_x, visible_window_y;
    view->buffer_to_window_coords(Gtk::TextWindowType::TEXT_WINDOW_TEXT, visible_rect.get_x(), visible_rect.get_y(), visible_window_x, visible_window_y);

    auto window_x = std::max(activation_rectangle.get_x(), visible_window_x); // Adjust tooltip right if it is left of view
    auto window_y = activation_rectangle.get_y();
    view->get_window(Gtk::TextWindowType::TEXT_WINDOW_TEXT)->get_root_coords(window_x, window_y, root_x, root_y);
    root_x -= 3; // -1xpadding

    if(root_y < size.second)
      root_x += visible_rect.get_width() * 0.1;         // Adjust tooltip right if it might be above cursor
    rectangle.set_y(std::max(0, root_y - size.second)); // Move tooptip down if it is above screen

    // Move tooltip left if it is right of screen
    auto screen_width = Gdk::Screen::get_default()->get_width();
    rectangle.set_x(root_x + size.first > screen_width ? std::max(0, screen_width - size.first) /* Move tooptip right if it is left of screen */ : root_x);
  }

  rectangle.set_width(size.first);
  rectangle.set_height(size.second);

  if(!disregard_drawn) {
    if(Tooltips::drawn_tooltips_rectangle.get_width() != 0) {
      if(rectangle.intersects(Tooltips::drawn_tooltips_rectangle)) { // Put tooltip above other tooltips
        int new_y = Tooltips::drawn_tooltips_rectangle.get_y() - size.second;
        if(new_y >= 0)
          rectangle.set_y(new_y);
        else { // Put tooltip to the right if the tooltip would be placed above the screen
          int new_x = Tooltips::drawn_tooltips_rectangle.get_x() + Tooltips::drawn_tooltips_rectangle.get_width() + 2;
          if(new_x + size.first < Gdk::Screen::get_default()->get_width())
            rectangle.set_x(new_x);
          else { // Put tooltip below if the tooltip would be placed outside of the screen on the right side
            new_y = Tooltips::drawn_tooltips_rectangle.get_y() + Tooltips::drawn_tooltips_rectangle.get_height();
            int root_x, root_y;
            if(view) {
              view->get_window(Gtk::TextWindowType::TEXT_WINDOW_TEXT)->get_root_coords(activation_rectangle.get_x(), activation_rectangle.get_y(), root_x, root_y);
              if(new_y < root_y + activation_rectangle.get_height())
                new_y = root_y + activation_rectangle.get_height() + 2;
            }
            rectangle.set_y(new_y);
          }
        }
      }
      Tooltips::drawn_tooltips_rectangle.join(rectangle);
    }
    else
      Tooltips::drawn_tooltips_rectangle = rectangle;
  }

  auto screen_width = Gdk::Screen::get_default()->get_width();
  auto screen_height = Gdk::Screen::get_default()->get_height();
  auto width = rectangle.get_width();
  if(rectangle.get_x() + width > screen_width) {
    width -= rectangle.get_x() + width - screen_width;
    scrolled_window->property_hscrollbar_policy() = Gtk::PolicyType::POLICY_AUTOMATIC;
  }
  auto height = rectangle.get_height();
  if(rectangle.get_y() + height > screen_height) {
    height -= rectangle.get_y() + height - screen_height;
    scrolled_window->property_vscrollbar_policy() = Gtk::PolicyType::POLICY_AUTOMATIC;
  }
  width -= 6;
  height -= 8;
  if(width > 0 && height > 0)
    scrolled_window->set_size_request(width, height);

  if(window->get_realized())
    window->move(rectangle.get_x(), rectangle.get_y());
  window->show_all();
  shown = true;
}

void Tooltip::hide(const boost::optional<std::pair<int, int>> &last_mouse_pos, const boost::optional<std::pair<int, int>> &mouse_pos) {
  // Keep tooltip if mouse is moving towards it
  // Calculated using dot product between the mouse_pos vector and the corners of the tooltip window
  if(view && window && shown && last_mouse_pos && mouse_pos) {
    static int root_x, root_y;
    view->get_window(Gtk::TextWindowType::TEXT_WINDOW_TEXT)->get_root_coords(last_mouse_pos->first, last_mouse_pos->second, root_x, root_y);
    int diff_x = mouse_pos->first - last_mouse_pos->first;
    int diff_y = mouse_pos->second - last_mouse_pos->second;
    class Corner {
    public:
      Corner(int x, int y) : x(x - root_x), y(y - root_y) {}
      int x, y;
    };
    std::vector<Corner> corners;
    corners.emplace_back(rectangle.get_x(), rectangle.get_y());
    corners.emplace_back(rectangle.get_x() + rectangle.get_width(), rectangle.get_y());
    corners.emplace_back(rectangle.get_x(), rectangle.get_y() + rectangle.get_height());
    corners.emplace_back(rectangle.get_x() + rectangle.get_width(), rectangle.get_y() + rectangle.get_height());
    for(auto &corner : corners) {
      if(diff_x * corner.x + diff_y * corner.y >= 0)
        return;
    }
  }
  Tooltips::shown_tooltips.erase(this);
  if(window)
    window->hide();
  shown = false;
}

void Tooltip::wrap_lines() {
  auto iter = buffer->begin();
  if(!iter)
    return;

  while(true) {
    auto last_space = buffer->end();
    bool long_line = true;
    for(int c = 0; c <= max_columns; c++) {
      if(*iter == ' ')
        last_space = iter;
      if(iter.ends_line()) {
        long_line = false;
        break;
      }
      if(!iter.forward_char())
        return;
    }
    if(long_line) {
      if(!last_space) { // If word is longer than max_columns
        while(true) {
          if(iter.ends_line())
            break;
          if(*iter == ' ') {
            last_space = iter;
            break;
          }
          if(!iter.forward_char())
            return;
        }
      }
      if(last_space) {
        auto mark = buffer->create_mark(last_space);

        auto next = last_space;
        next.forward_char();
        buffer->erase(last_space, next);
        buffer->insert(mark->get_iter(), "\n");

        iter = mark->get_iter();
        buffer->delete_mark(mark);
      }
    }
    if(!iter.forward_char())
      return;
  }
}


void Tooltip::create_tags() {
  if(!h1_tag) {
    h1_tag = buffer->create_tag();
    h1_tag->property_weight() = Pango::WEIGHT_ULTRAHEAVY;
    h1_tag->property_underline() = Pango::UNDERLINE_SINGLE;

    h2_tag = buffer->create_tag();
    h2_tag->property_weight() = Pango::WEIGHT_ULTRAHEAVY;

    h3_tag = buffer->create_tag();
    h3_tag->property_style() = Pango::Style::STYLE_ITALIC;

    auto source_font_family = Pango::FontDescription(Config::get().source.font).get_family();

    code_tag = buffer->create_tag();
    code_tag->property_family() = source_font_family;
    auto background_rgba = Gdk::RGBA();
    auto normal_color = window->get_style_context()->get_color(Gtk::StateFlags::STATE_FLAG_NORMAL);
    auto light_theme = (normal_color.get_red() + normal_color.get_green() + normal_color.get_blue()) / 3 < 0.5;
    if(light_theme)
      background_rgba.set_rgba(1.0, 1.0, 1.0, 0.4);
    else
      background_rgba.set_rgba(0.0, 0.0, 0.0, 0.2);
    code_tag->property_background_rgba() = background_rgba;

    code_block_tag = buffer->create_tag();
    code_block_tag->property_family() = source_font_family;
    code_block_tag->property_paragraph_background_rgba() = background_rgba;

    bold_tag = buffer->create_tag();
    bold_tag->property_weight() = Pango::WEIGHT_ULTRAHEAVY;

    italic_tag = buffer->create_tag();
    italic_tag->property_style() = Pango::Style::STYLE_ITALIC;

    strikethrough_tag = buffer->create_tag();
    strikethrough_tag->property_strikethrough() = true;
  }
}

void Tooltip::insert_with_links_tagged(const std::string &text) {
  if(text.empty())
    return;
  const static std::regex http_regex("https?://[\\w\\-.~:/?#%\\[\\]@!$&'()*+,;=]+[\\w\\-~/#@$*+;=]", std::regex::optimize);
  std::smatch sm;
  std::sregex_iterator it(text.begin(), text.end(), http_regex);
  std::sregex_iterator end;
  size_t start_pos = 0;
  for(; it != end; ++it) {
    buffer->insert(buffer->get_insert()->get_iter(), &text[start_pos], &text[it->position()]);
    buffer->insert_with_tag(buffer->get_insert()->get_iter(), &text[it->position()], text.c_str() + it->position() + it->length(), link_tag);
    start_pos = it->position() + it->length();
  }
  if(start_pos < text.size())
    buffer->insert(buffer->get_insert()->get_iter(), &text[start_pos], text.c_str() + text.size());
}

void Tooltip::insert_markdown(const std::string &input) {
  create_tags();

  size_t i = 0;

  auto forward_to = [&](const std::vector<char> chars) {
    for(; i < input.size(); i++) {
      if(std::any_of(chars.begin(), chars.end(), [&input, &i](char chr) { return input[i] == chr; }))
        return true;
    }
    return false;
  };

  auto forward_passed = [&](const std::vector<char> chars) {
    for(; i < input.size(); i++) {
      if(std::none_of(chars.begin(), chars.end(), [&input, &i](char chr) { return input[i] == chr; }))
        return true;
    }
    return false;
  };

  auto unescape = [&](size_t &i) {
    if(input[i] == '\\') {
      if(i + 1 < input.size())
        i++;
      return true;
    }
    return false;
  };

  std::function<void(size_t, size_t)> insert_text = [&](size_t from, size_t to) {
    if(from == to)
      return;

    auto i = from;

    std::string partial;

    auto is_whitespace_character = [&](size_t i) {
      return input[i] == ' ' || input[i] == '\t' || input[i] == '\n' || input[i] == '\r' || input[i] == '\f';
    };

    auto is_punctuation_character = [&](size_t i) {
      static std::set<char> punctuation_characters = {'!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', ':', ';', '<', '=', '>', '?', '@', '[', '\\', ']', '^', '_', '`', '{', '|', '}', '~'};
      return punctuation_characters.find(input[i]) != punctuation_characters.end();
    };

    auto left_flanking_delimiter_run = [&](size_t i, size_t n) {
      return !(i + n >= to || is_whitespace_character(i + n)) &&
             (!is_punctuation_character(i + n) ||
              (i == from || is_whitespace_character(i - 1) || is_punctuation_character(i - 1)));
    };
    auto right_flanking_delimiter_run = [&](size_t i, size_t n) {
      return !(i == from || is_whitespace_character(i - 1)) &&
             (!is_punctuation_character(i - 1) ||
              (i + n >= to || is_whitespace_character(i + n) || is_punctuation_character(i + n)));
    };

    /// Using rules in https://spec.commonmark.org/0.29/#emphasis-and-strong-emphasis
    auto insert_emphasis = [&] {
      auto i_saved = i;
      std::string prefix;
      if(input[i] == '*' || input[i] == '_') {
        prefix += input[i];
        ++i;
        for(; prefix.size() < 3 && input[i] == prefix.back(); ++i)
          prefix += input[i];
      }
      if(prefix.empty())
        return false;
      if(prefix.back() == '*') { // Rule 1 and 5
        if(!left_flanking_delimiter_run(i - prefix.size(), prefix.size())) {
          i = i_saved;
          return false;
        }
      }
      else { // Rule 2 and 6
        if(!(left_flanking_delimiter_run(i - prefix.size(), prefix.size()) &&
             (!right_flanking_delimiter_run(i - prefix.size(), prefix.size()) || i - prefix.size() == from || is_punctuation_character(i - prefix.size() - 1)))) {
          i = i_saved;
          return false;
        }
      }
      auto start = i;
      for(; i < to - (prefix.size() - 1); i++) {
        if(!unescape(i)) {
          if(starts_with(input, i, prefix)) {
            if(prefix.back() == '*') { // Rule 3 and 7
              if(right_flanking_delimiter_run(i, prefix.size()))
                break;
              else if(left_flanking_delimiter_run(i, prefix.size())) {
                i = i_saved;
                return false;
              }
            }
            else { // Rule 4 and 8
              if(right_flanking_delimiter_run(i, prefix.size()) &&
                 (!left_flanking_delimiter_run(i, prefix.size()) || i + prefix.size() == to || is_punctuation_character(i + prefix.size())))
                break;
              else if(left_flanking_delimiter_run(i, prefix.size()) &&
                      (!right_flanking_delimiter_run(i, prefix.size()) || i == from || is_punctuation_character(i - 1))) {
                i = i_saved;
                return false;
              }
            }
          }
        }
      }
      if(i >= to - (prefix.size() - 1)) {
        i = i_saved;
        return false;
      }
      insert_with_links_tagged(partial);
      partial.clear();
      auto start_offset = buffer->get_insert()->get_iter().get_offset();
      insert_text(start, i);
      if(prefix.size() == 1)
        buffer->apply_tag(italic_tag, buffer->get_iter_at_offset(start_offset), buffer->get_insert()->get_iter());
      else if(prefix.size() == 2)
        buffer->apply_tag(bold_tag, buffer->get_iter_at_offset(start_offset), buffer->get_insert()->get_iter());
      else {
        buffer->apply_tag(italic_tag, buffer->get_iter_at_offset(start_offset), buffer->get_insert()->get_iter());
        buffer->apply_tag(bold_tag, buffer->get_iter_at_offset(start_offset), buffer->get_insert()->get_iter());
      }
      i += prefix.size() - 1;
      return true;
    };

    auto insert_strikethrough = [&] {
      if(starts_with(input, i, "~~")) {
        auto i_saved = i;
        i += 2;
        if(i < to) {
          auto start = i;
          for(; i < to; i++) {
            if(!unescape(i) && starts_with(input, i, "~~"))
              break;
          }
          if(i == to) {
            i = i_saved;
            return false;
          }
          insert_with_links_tagged(partial);
          partial.clear();
          auto start_offset = buffer->get_insert()->get_iter().get_offset();
          insert_text(start, i);
          buffer->apply_tag(strikethrough_tag, buffer->get_iter_at_offset(start_offset), buffer->get_insert()->get_iter());
          i++;
          return true;
        }
        i = i_saved;
      }
      return false;
    };

    auto insert_code = [&] {
      if(input[i] == '`') {
        auto i_saved = i;
        i++;
        if(i < to) {
          bool two_backticks = false;
          if(input[i] == '`') {
            two_backticks = true;
            i++;
          }
          if(i < to) {
            auto start = i;
            for(; i < to; i++) {
              if(input[i] == '`') {
                if(!two_backticks)
                  break;
                if(i + 1 < to && input[i + 1] == '`')
                  break;
              }
            }
            if(i == to) {
              i = i_saved;
              return false;
            }
            insert_with_links_tagged(partial);
            partial.clear();
            buffer->insert_with_tag(buffer->get_insert()->get_iter(), input.substr(start, i - start), code_tag);
            if(two_backticks)
              i++;
            return true;
          }
        }
        i = i_saved;
      }
      return false;
    };

    auto insert_link = [&] {
      if(input[i] == '[') {
        auto i_saved = i;
        i++;
        if(i < to) {
          auto text_start = i;
          for(; i < to; i++) {
            if(!unescape(i) && input[i] == ']')
              break;
          }
          if(i == to) {
            i = i_saved;
            return false;
          }
          auto text_end = i;
          i++;
          if(i < to && input[i] == '(') {
            i++;
            auto link_start = i;
            for(; i < to; i++) {
              if(!unescape(i) && input[i] == ')')
                break;
            }
            if(i == to) {
              i = i_saved;
              return false;
            }
            insert_with_links_tagged(partial);
            partial.clear();
            auto start_offset = buffer->get_insert()->get_iter().get_offset();
            insert_text(text_start, text_end);
            auto start = buffer->get_iter_at_offset(start_offset);
            auto end = buffer->get_insert()->get_iter();
            buffer->apply_tag(link_tag, start, end);
            auto tag = buffer->create_tag();
            buffer->apply_tag(tag, start, end);
            links.emplace(tag, input.substr(link_start, i - link_start));
            return true;
          }
          else if(i < to && input[i] == '[') {
            i++;
            auto link_start = i;
            for(; i < to; i++) {
              if(!unescape(i) && input[i] == ']')
                break;
            }
            if(i == to) {
              i = i_saved;
              return false;
            }
            insert_with_links_tagged(partial);
            partial.clear();
            auto start_offset = buffer->get_insert()->get_iter().get_offset();
            insert_text(text_start, text_end);
            auto start = buffer->get_iter_at_offset(start_offset);
            auto end = buffer->get_insert()->get_iter();
            buffer->apply_tag(link_tag, start, end);
            auto tag = buffer->create_tag();
            buffer->apply_tag(tag, start, end);
            reference_links.emplace(tag, input.substr(link_start, i - link_start));
            return true;
          }
          else if(text_start != text_end) {
            insert_with_links_tagged(partial);
            partial.clear();
            auto start_offset = buffer->get_insert()->get_iter().get_offset();
            insert_text(text_start, text_end);
            auto start = buffer->get_iter_at_offset(start_offset);
            auto end = buffer->get_insert()->get_iter();
            buffer->apply_tag(link_tag, start, end);
            auto tag = buffer->create_tag();
            buffer->apply_tag(tag, start, end);
            reference_links.emplace(tag, buffer->get_text(start, end));
            i = text_end;
            return true;
          }
        }
        i = i_saved;
      }
      return false;
    };

    for(; i < to; i++) {
      if(unescape(i)) {
        partial += input[i];
        continue;
      }
      else if(starts_with(input, i, "&nbsp;")) {
        partial += ' ';
        i += 5;
        continue;
      }
      else if(insert_code() || insert_emphasis() || insert_strikethrough() || insert_link())
        continue;
      if(input[i] == '\n' && i + 1 < to)
        partial += ' ';
      else if(input[i] == ' ' && ((i > from && (input[i - 1] == '\n' || input[i - 1] == ' '))))
        continue;
      else
        partial += input[i];
    }
    insert_with_links_tagged(partial);
  };

  auto insert_header = [&] {
    size_t end_next_line = std::string::npos;
    int header = 0;
    for(; i < input.size() && header < 6 && input[i] == '#'; i++)
      header++;
    if(header == 0) {
      auto i_saved = i;
      forward_to({'\n'});
      i++;
      if(i < input.size() && (input[i] == '=' || input[i] == '-')) {
        forward_passed({input[i]});
        if(i == input.size() || input[i] == '\n') {
          if(input[i - 1] == '=')
            header = 1;
          else
            header = 2;
          end_next_line = i;
        }
      }
      i = i_saved;
    }
    if(header == 0)
      return false;
    forward_passed({' '});
    auto start = i;
    forward_to({'\n'});
    auto end = buffer->end();
    if(end.backward_char() && !end.starts_line())
      buffer->insert_at_cursor("\n");
    auto start_offset = buffer->get_insert()->get_iter().get_offset();
    insert_text(start, i);
    if(header == 1)
      buffer->apply_tag(h1_tag, buffer->get_iter_at_offset(start_offset), buffer->get_insert()->get_iter());
    else if(header == 2)
      buffer->apply_tag(h2_tag, buffer->get_iter_at_offset(start_offset), buffer->get_insert()->get_iter());
    else if(header == 3)
      buffer->apply_tag(h3_tag, buffer->get_iter_at_offset(start_offset), buffer->get_insert()->get_iter());
    buffer->insert_at_cursor("\n\n");
    if(end_next_line != std::string::npos)
      i = end_next_line;
    return true;
  };

  auto is_empty_line = [&] {
    auto i_saved = i;
    if(forward_passed({' ', '\t'}) && input[i] == '\n')
      return true;
    i = i_saved;
    return false;
  };

  auto insert_horizontal_line = [&] {
    auto i_saved = i;
    char symbol = 0;
    if(starts_with(input, i, "---") || starts_with(input, i, "***") || starts_with(input, i, "___"))
      symbol = input[i];
    if(symbol != 0) {
      forward_passed({symbol});
      if(i == input.size() || input[i] == '\n') {
        if(i < input.size())
          ++i;
        buffer->insert_at_cursor("---\n");
        if(is_empty_line())
          buffer->insert_at_cursor("\n");
        return true;
      }
    }
    i = i_saved;
    return false;
  };

  auto insert_code_block = [&] {
    if(starts_with(input, i, "```")) {
      auto i_saved = i;
      if(forward_to({'\n'})) {
        auto language = input.substr(i_saved + 3, i - (i_saved + 3));
        i++;
        if(i < input.size()) {
          auto start = i;
          while(i < input.size() && !(input[i - 1] == '\n' && starts_with(input, i, "```")))
            i++;
          if(i == input.size()) {
            i = i_saved;
            return false;
          }
          insert_code(input.substr(start, i - start), language, true);
          i += 4;
          if(is_empty_line())
            buffer->insert_at_cursor("\n");
          return true;
        }
      }
      i = i_saved;
    }
    return false;
  };

  auto insert_reference = [&] {
    if(input[i] == '[') {
      auto i_saved = i;
      i++;
      if(i < input.size()) {
        auto reference_start = i;
        for(; i < input.size(); i++) {
          if(!unescape(i) && input[i] == ']')
            break;
        }
        if(i == input.size()) {
          i = i_saved;
          return false;
        }
        auto reference_end = i;
        i++;
        if(forward_passed({' ', '\n'}) && input[i] == ':') {
          i++;
          if(forward_passed({' ', '\n'})) {
            auto link_start = i;
            forward_to({' ', '\n'});
            auto start_offset = buffer->get_insert()->get_iter().get_offset();
            insert_text(reference_start, reference_end);
            auto start = buffer->get_iter_at_offset(start_offset);
            auto end = buffer->get_insert()->get_iter();
            references.emplace(buffer->get_text(start, end), input.substr(link_start, i - link_start));
            buffer->erase(start, end);
            return true;
          }
        }
      }
      i = i_saved;
    }
    return false;
  };

  auto is_number = [&] {
    return input[i] >= '0' && input[i] <= '9';
  };

  auto forward_passed_number = [&] {
    while(i < input.size() && is_number())
      ++i;
    return i < input.size();
  };

  std::function<bool()> insert_list = [&] {
    auto i_saved = i;
    if(starts_with(input, i, "- ") || starts_with(input, i, "+ ") || starts_with(input, i, "* ") || (is_number() && forward_passed_number() && starts_with(input, i, ". "))) {
      auto start = i_saved;
      while(true) {
        forward_to({'\n'});
        ++i;
        if(i >= input.size()) {
          insert_text(start, i - 1);
          break;
        }
        if(is_empty_line()) {
          insert_text(start, i - 1);
          buffer->insert_at_cursor("\n\n");
          break;
        }
        auto i_saved = i;
        // TODO: Fix check: starts_with(input, i, "1. ")
        if(forward_passed({' ', '\t'})) {
          auto i_saved2 = i;
          if(starts_with(input, i, "- ") || starts_with(input, i, "+ ") || starts_with(input, i, "* ") || (is_number() && forward_passed_number() && starts_with(input, i, ". "))) {
            i = i_saved2;
            insert_text(start, i_saved - 1);
            buffer->insert_at_cursor('\n' + input.substr(i_saved, i - i_saved));
            insert_list();
            break;
          }
        }
        i = i_saved;
      }
      return true;
    }
    i = i_saved;
    return false;
  };

  auto forward_passed_empty_line = [&] {
    while(is_empty_line())
      ++i;
    return i < input.size();
  };

  while(forward_passed_empty_line()) {
    if(insert_header() || insert_horizontal_line() || insert_code_block() || insert_reference() || insert_list())
      continue;
    // Insert paragraph:
    auto start = i;
    bool ends_with_empty_line = false;
    for(; forward_to({'\n'}); i++) {
      ++i;
      ScopeGuard guard{[&i] { --i; }};
      if(is_empty_line()) {
        ends_with_empty_line = true;
        break;
      }
      if(i < input.size() && (input[i] == '#' ||
                              starts_with(input, i, "```") || starts_with(input, i, "- ") ||
                              starts_with(input, i, "+ ") || starts_with(input, i, "* ") ||
                              starts_with(input, i, "1. "))) {
        break;
      }
    }
    while(start < i && (input[start] == ' ' || input[start] == '\t' || input[start] == '\n'))
      ++start;
    insert_text(start, i);
    buffer->insert_at_cursor(ends_with_empty_line ? "\n\n" : "\n");
  }

  // Remove invalid reference links
  for(auto link_it = reference_links.begin(); link_it != reference_links.end();) {
    auto reference_it = references.find(link_it->second);
    if(reference_it == references.end()) {
      auto it = buffer->begin();
      if(it.begins_tag(link_it->first) || it.forward_to_tag_toggle(link_it->first)) {
        auto start = it;
        it.forward_to_tag_toggle(link_it->first);
        buffer->remove_tag(link_tag, start, it);
        buffer->remove_tag(link_it->first, buffer->begin(), buffer->end());
        auto start_mark = Source::Mark(start);
        buffer->insert(it, "]");
        buffer->insert(start_mark->get_iter(), "[");
        link_it = reference_links.erase(link_it);
        continue;
      }
    }
    ++link_it;
  }

  remove_trailing_newlines();
}

void Tooltip::insert_code(const std::string &code, boost::variant<std::string, Glib::RefPtr<Gsv::Language>> language_variant, bool block) {
  create_tags();

  auto insert_iter = buffer->get_insert()->get_iter();
  Source::Mark start_mark(insert_iter);

  bool style_format_type_description = false;
  if(!block) {
    auto pos = code.find("\n");
    if(pos != std::string::npos)
      block = true;
    else if(insert_iter == buffer->begin() && utf8_character_count(code) > static_cast<size_t>(max_columns)) {
      block = true;
      style_format_type_description = true;
    }
  }

  buffer->insert_with_tag(insert_iter, code, block ? code_block_tag : code_tag);

  if(view) {
    Glib::RefPtr<Gsv::Language> language;
    if(auto language_ptr = boost::get<Glib::RefPtr<Gsv::Language>>(&language_variant))
      language = *language_ptr;
    else if(auto language_identifier = boost::get<std::string>(&language_variant)) {
      if(!language_identifier->empty()) {
        language = Source::LanguageManager::get_default()->get_language(*language_identifier);
        if(!language)
          language = Source::guess_language('.' + *language_identifier);
        if(!language) {
          if(auto source_view = dynamic_cast<Source::View *>(view))
            language = Source::LanguageManager::get_default()->get_language(source_view->language_id);
        }
      }
    }
    if(language) {
      Gsv::View tmp_view;
      tmp_view.get_buffer()->set_text(code);
      auto scheme = view->get_source_buffer()->get_style_scheme();
      tmp_view.get_source_buffer()->set_style_scheme(scheme);
      tmp_view.get_source_buffer()->set_language(language);
      tmp_view.get_source_buffer()->set_highlight_syntax(true);
      tmp_view.get_source_buffer()->ensure_highlight(tmp_view.get_buffer()->begin(), tmp_view.get_buffer()->end());

      auto start_iter = start_mark->get_iter();
      tmp_view.get_buffer()->get_tag_table()->foreach([this, &tmp_view, &start_iter](const Glib::RefPtr<Gtk::TextTag> &tmp_tag) {
        if(tmp_tag->property_foreground_set()) {
          auto tag = buffer->create_tag();
          tag->property_foreground_rgba() = tmp_tag->property_foreground_rgba().get_value();

          auto tmp_iter = tmp_view.get_source_buffer()->begin();
          Gtk::TextIter tmp_start;
          if(tmp_iter.begins_tag(tmp_tag))
            tmp_start = tmp_iter;
          while(tmp_iter.forward_to_tag_toggle(tmp_tag)) {
            if(tmp_iter.ends_tag(tmp_tag)) {
              auto start = start_iter;
              start.forward_chars(tmp_start.get_offset());
              auto end = start_iter;
              end.forward_chars(tmp_iter.get_offset());
              buffer->apply_tag(tag, start, end);
            }
            else
              tmp_start = tmp_iter;
          }
        }
      });
    }
  }

  // Make long type descriptions readable
  if(style_format_type_description) {
    int initial_max_columns = max_columns;
    std::list<Source::Mark> open_brackets;
    for(auto iter = start_mark->get_iter(); iter; iter.forward_char()) {
      if(*iter == '(' || *iter == '[' || *iter == '{' || *iter == '<')
        open_brackets.emplace_back(iter);
      else if(*iter == ')' || *iter == ']' || *iter == '}' || *iter == '>') {
        if(!open_brackets.empty())
          open_brackets.pop_back();
      }
      else if(*iter == ',' && !open_brackets.empty()) {
        auto line_offset = open_brackets.back()->get_iter().get_line_offset();
        if(iter.forward_char()) {
          Source::Mark mark(iter);
          auto previous = iter;
          if(*iter == ' ' && iter.forward_char()) {
            buffer->erase(previous, iter);
            iter = mark->get_iter();
          }
          buffer->insert(mark->get_iter(), '\n' + std::string(line_offset + 1, ' '));
          iter = mark->get_iter();
        }
      }
      else if(*iter == ' ' && open_brackets.empty() && iter.get_line_offset() + (buffer->end().get_offset() - iter.get_offset()) > initial_max_columns) { // Add new line between return value, parameters and specifiers
        auto previous = iter;
        if(iter.forward_char()) {
          Source::Mark mark(iter);
          buffer->erase(previous, iter);
          buffer->insert(mark->get_iter(), "\n");
          iter = mark->get_iter();
        }
      }
      max_columns = std::max(max_columns, iter.get_line_offset() + 1);
    }
  }
}

void Tooltip::insert_doxygen(const std::string &input_, bool remove_delimiters) {
  create_tags();

  auto get_text = [&] {
    auto &input = input_;
    size_t i = 0;
    auto forward_passed = [&](const std::vector<char> chars) {
      for(; i < input.size(); i++) {
        if(std::none_of(chars.begin(), chars.end(), [&](char chr) { return input[i] == chr; }))
          return true;
      }
      return false;
    };

    size_t min_indent = std::numeric_limits<size_t>::max();
    auto forward_to_text = [&](bool first_line) -> bool {
      auto line_start = i;
      auto after_delimiter = i;
      forward_passed({' ', '\t'});
      if(i < input.size() && (input[i] == '/' || input[i] == '*')) {
        forward_passed({'/'});
        if(i < input.size() && input[i] == '*' && forward_passed({'*'})) {
          if(input[i] == '/')
            return false;
        }
        forward_passed({'!'});
        after_delimiter = i;
      }
      forward_passed({' ', '\t'});
      if(!first_line && i < input.size() && input[i] != '\n') {
        min_indent = std::min(min_indent, i - line_start);
        if(i - line_start > min_indent && line_start + min_indent >= after_delimiter)
          i = line_start + min_indent;
      }
      return i < input.size();
    };

    std::string text;
    text.reserve(input.size());
    bool first_line = true;
    for(; i < input.size(); ++i) {
      if(!forward_to_text(first_line))
        break;
      if(!(first_line && input[i] == '\n')) {
        for(; i < input.size(); ++i) {
          text += input[i];
          if(input[i] == '\n')
            break;
        }
      }
      first_line = false;
    }
    if(text.size() >= 2 && text[text.size() - 2] == '*' && text[text.size() - 1] == '/') {
      text.erase(text.size() - 2, 2);
      while(!text.empty() && text.back() == ' ')
        text.pop_back();
    }
    return text;
  };

  const auto &input = remove_delimiters ? get_text() : input_;
  size_t i = 0;
  auto forward_passed = [&](const std::vector<char> chars) {
    for(; i < input.size(); i++) {
      if(std::none_of(chars.begin(), chars.end(), [&](char chr) { return input[i] == chr; }))
        return true;
    }
    return false;
  };

  auto get_token = [&] {
    auto start = i;
    while(i < input.size() && ((input[i] >= 'a' && input[i] <= 'z') || (input[i] >= 'A' && input[i] <= 'Z') || (input[i] >= '0' && input[i] <= '9') || input[i] == '_'))
      ++i;
    return input.substr(start, i - start);
  };

  auto is_escape_character = [&] {
    static std::set<char> escape_characters{'\\', '`', '*', '_', '{', '}', '[', ']', '(', ')', '#', '+', '-', '.', '!', '|'};
    return escape_characters.find(input[i]) != escape_characters.end();
  };

  auto get_word = [&](bool escape) {
    std::string word;
    for(; i < input.size() && input[i] != ' ' && input[i] != '\t' && input[i] != '\n'; ++i) {
      if((input[i] == '.' || input[i] == ',' || input[i] == ':' || input[i] == ';' || input[i] == '!' || input[i] == '?') &&
         (i + 1 >= input.size() || input[i + 1] == ' ' || input[i + 1] == '\t' || input[i + 1] == '\n')) {
        break;
      }
      if(escape && is_escape_character())
        word += "\\";
      word += input[i];
    }
    return word;
  };

  std::string markdown;
  markdown.reserve(input.size());
  bool first_parameter = true;

  while(i < input.size()) {
    if(input[i] == '\\' || input[i] == '@') {
      auto i_saved = i;
      ++i;
      auto token = get_token();
      if(token.empty()) {
        if(i < input.size()) {
          if(input[i] == '\\')
            markdown += "\\\\";
          else
            markdown += input[i];
        }
        ++i;
      }
      else if(token == "a" || token == "e" || token == "em") {
        forward_passed({' ', '\t', '\n'});
        markdown += '*' + get_word(true) + '*';
      }
      else if(token == "b") {
        forward_passed({' ', '\t', '\n'});
        markdown += "**" + get_word(true) + "**";
      }
      else if(token == "c" || token == "p") {
        forward_passed({' ', '\t', '\n'});
        markdown += '`' + get_word(false) + '`';
      }
      else if(token == "brief")
        forward_passed({' ', '\t', '\n'});
      else if(token == "param") {
        std::string direction;
        if(input[i] == '[') {
          ++i;
          if(i >= input.size())
            break;
          auto pos = input.find(']', i);
          if(pos != std::string::npos) {
            direction = input.substr(i, pos - i);
            i = pos + 1;
          }
        }
        forward_passed({' ', '\t', '\n'});
        if(first_parameter) {
          markdown += "\n\n**Parameters:**\n";
          first_parameter = false;
        }
        markdown += "- `" + get_word(false) + '`';
        if(!direction.empty())
          markdown += " (" + direction + ')';
      }
      else if(token == "return" || token == "returns" || token == "result") {
        forward_passed({' ', '\t', '\n'});
        markdown += "\n\n**Returns** ";
      }
      else if(token == "sa" || token == "see") {
        forward_passed({' ', '\t', '\n'});
        markdown += "\n\nSee also ";
      }
      else if(token == "throw" || token == "throws" || token == "exception") {
        forward_passed({' ', '\t', '\n'});
        markdown += "\n\n**Throws** ";
      }
      else if(token == "deprecated") {
        forward_passed({' ', '\t', '\n'});
        markdown += "\n\n**Deprecated** ";
      }
      else if(token == "note") {
        forward_passed({' ', '\t', '\n'});
        markdown += "\n\n**Note** ";
      }
      else if(token == "code") {
        std::string language_id;
        if(input[i] == '{') {
          ++i;
          if(i >= input.size())
            break;
          if(input[i] == '.') {
            auto pos = input.find('}', i + 1);
            if(pos != std::string::npos) {
              language_id = input.substr(i + 1, pos - (i + 1));
              i = pos + 1;
            }
          }
        }
        auto start = i + 1;
        for(; i < input.size(); ++i) {
          if(input[i] == '\n') {
            ++i;
            if(i >= input.size())
              break;
            if(input[i] == '\\' || input[i] == '@') {
              auto end = i - 1;
              ++i;
              auto token = get_token();
              if(token == "endcode") {
                if(language_id.empty() && view) {
                  if(auto source_view = dynamic_cast<Source::View *>(view))
                    language_id = source_view->language_id;
                }
                markdown += "```" + language_id + '\n' + input.substr(start, end - start) + "\n```\n";
                ++i;
                break;
              }
            }
          }
        }
      }
      else if(token == "dot") {
        for(; i < input.size(); ++i) {
          if(input[i] == '\n') {
            ++i;
            if(i >= input.size())
              break;
            if(input[i] == '\\') {
              ++i;
              auto token = get_token();
              if(token == "enddot") {
                markdown += "\n\n";
                ++i;
                break;
              }
            }
          }
        }
      }
      else if(token == "verbatim") {
        auto start = i + 1;
        for(; i < input.size(); ++i) {
          if(input[i] == '\n') {
            ++i;
            if(i >= input.size())
              break;
            if(input[i] == '\\') {
              auto end = i - 1;
              ++i;
              auto token = get_token();
              if(token == "endverbatim") {
                markdown += "```\n" + input.substr(start, end - start) + "\n```\n";
                ++i;
                break;
              }
            }
          }
        }
      }
      else {
        auto i2 = i_saved;
        if(i2 > 0) {
          --i2;
          while(i2 > 0 && (input[i2] == ' ' || input[i2] == '\t'))
            --i2;
        }
        if(input[i2] == '\n' || i2 == 0)
          markdown += "\n\n";
        if(input[i_saved] == '\\')
          markdown += '\\';
        markdown += input[i_saved];
        markdown += token;
      }
    }
    else if(input[i] == '%')
      ++i;
    else if(starts_with(input, i, "<tt>")) {
      i += 4;
      auto pos = input.find("</tt>", i);
      if(pos != std::string::npos) {
        markdown += '`' + input.substr(i, pos - i) + '`';
        i = pos + 5;
      }
    }
    else if(starts_with(input, i, "<code>")) {
      i += 6;
      auto pos = input.find("</code>", i);
      if(pos != std::string::npos) {
        markdown += '`' + input.substr(i, pos - i) + '`';
        i = pos + 7;
      }
    }
    else if(starts_with(input, i, "<em>")) {
      i += 4;
      markdown += '*';
      for(; i < input.size(); ++i) {
        if(input[i] == '\\' && i + 1 < input.size()) {
          markdown += input[i];
          markdown += input[i + 1];
          ++i;
        }
        else {
          if(starts_with(input, i, "</em>")) {
            i += 5;
            break;
          }
          if(is_escape_character())
            markdown += '\\';
          markdown += input[i];
        }
      }
      markdown += '*';
    }
    else if(starts_with(input, i, "<b>")) {
      i += 3;
      markdown += "**";
      for(; i < input.size(); ++i) {
        if(input[i] == '\\' && i + 1 < input.size()) {
          markdown += input[i];
          markdown += input[i + 1];
          ++i;
        }
        else {
          if(starts_with(input, i, "</b>")) {
            i += 4;
            break;
          }
          if(is_escape_character())
            markdown += '\\';
          markdown += input[i];
        }
      }
      markdown += "**";
    }
    else if(input[i] == '`') {
      auto i_saved = i;
      std::string graves;
      do {
        graves += input[i];
        ++i;
      } while(i < input.size() && input[i] == '`');
      auto pos = input.find(graves, i);
      if(pos != std::string::npos) {
        markdown += input.substr(i_saved, pos + graves.size() - i_saved);
        i = pos + graves.size();
      }
    }
    else if(input[i] == '"') {
      markdown += "\"";
      ++i;
      for(; i < input.size(); ++i) {
        if(input[i] == '\\' && i + 1 < input.size()) {
          markdown += input[i];
          markdown += input[i + 1];
          ++i;
        }
        else {
          if(is_escape_character())
            markdown += '\\';
          markdown += input[i];
          if(input[i] == '"')
            break;
        }
      }
      ++i;
    }
    else {
      markdown += input[i];
      ++i;
    }
  }

  if(!markdown.empty())
    insert_markdown(markdown);
}

void Tooltip::insert_docstring(const std::string &input_) {
  create_tags();

  // Workaround for python-language-server that returns unnecessary function signatures
  const static std::regex regex("^([a-zA-Z0-9_]+\\([^\n]*\\)( -> [^\n]+)?(\n|$))+(\n|$)", std::regex::extended | std::regex::optimize);
  std::smatch sm;
  const std::string &input = std::regex_search(input_, sm, regex) ? sm.suffix() : input_;

  std::string partial;
  partial.reserve(input.size());
  size_t i = 0;

  auto unescape = [&](size_t &i) {
    if(input[i] == '\\') {
      if(i + 1 < input.size())
        i++;
      return true;
    }
    return false;
  };

  auto forward_to = [&](const std::vector<char> chars) {
    for(; i < input.size(); i++) {
      if(std::any_of(chars.begin(), chars.end(), [&input, &i](char chr) { return input[i] == chr; }))
        return true;
    }
    return false;
  };

  auto forward_passed = [&](const std::vector<char> chars) {
    for(; i < input.size(); i++) {
      if(std::none_of(chars.begin(), chars.end(), [&input, &i](char chr) { return input[i] == chr; }))
        return true;
    }
    return false;
  };

  auto is_whitespace_character = [&](size_t i) {
    const static std::string utf8_space = " "; // Python docstring sometimes uses this...
    return input[i] == ' ' || input[i] == '\t' || input[i] == '\n' || input[i] == '\r' || input[i] == '\f' ||
           ((input[i] == utf8_space[0] && i + 1 < input.size() && input[i + 1] == utf8_space[1]) || (i >= 1 && input[i - 1] == utf8_space[0] && input[i] == utf8_space[1]));
  };

  auto insert_header = [&] {
    if(is_whitespace_character(i))
      return false;
    auto i_saved = i;
    forward_to({'\n'});
    auto end_header = i;
    i++;
    int header = 0;
    if(i < input.size() && (input[i] == '=' || input[i] == '-')) {
      forward_passed({input[i]});
      if(i == input.size() || input[i] == '\n') {
        if(input[i - 1] == '=')
          header = 1;
        else
          header = 2;
      }
    }
    if(header == 0) {
      i = i_saved;
      return false;
    }
    insert_with_links_tagged(partial);
    partial.clear();
    auto start_offset = buffer->get_insert()->get_iter().get_offset();
    insert_with_links_tagged(input.substr(i_saved, end_header - i_saved));
    if(header == 1)
      buffer->apply_tag(h1_tag, buffer->get_iter_at_offset(start_offset), buffer->get_insert()->get_iter());
    else if(header == 2)
      buffer->apply_tag(h2_tag, buffer->get_iter_at_offset(start_offset), buffer->get_insert()->get_iter());
    buffer->insert_at_cursor("\n");
    return true;
  };

  auto is_punctuation_character = [&](size_t i) {
    static std::set<char> punctuation_characters = {'!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', ':', ';', '<', '=', '>', '?', '@', '[', '\\', ']', '^', '_', '`', '{', '|', '}', '~'};
    return punctuation_characters.find(input[i]) != punctuation_characters.end();
  };

  auto left_flanking_delimiter_run = [&](size_t i, size_t n) {
    return !(i + n >= input.size() || is_whitespace_character(i + n)) &&
           (!is_punctuation_character(i + n) ||
            (i == 0 || is_whitespace_character(i - 1) || is_punctuation_character(i - 1)));
  };
  auto right_flanking_delimiter_run = [&](size_t i, size_t n) {
    return !(i == 0 || is_whitespace_character(i - 1)) &&
           (!is_punctuation_character(i - 1) ||
            (i + n >= input.size() || is_whitespace_character(i + n) || is_punctuation_character(i + n)));
  };

  /// Using rules in https://spec.commonmark.org/0.29/#emphasis-and-strong-emphasis
  auto insert_emphasis = [&] {
    auto i_saved = i;
    std::string prefix;
    if(input[i] == '*') {
      prefix += input[i];
      ++i;
      for(; prefix.size() < 2 && input[i] == prefix.back(); ++i)
        prefix += input[i];
    }
    if(prefix.empty())
      return false;
    // Rule 1 and 5
    if(!left_flanking_delimiter_run(i - prefix.size(), prefix.size())) {
      i = i_saved;
      return false;
    }
    auto start = i;
    for(; i < input.size() - (prefix.size() - 1); i++) {
      if(!unescape(i)) {
        if(starts_with(input, i, prefix)) {
          // Rule 3 and 7
          if(right_flanking_delimiter_run(i, prefix.size()))
            break;
          else if(left_flanking_delimiter_run(i, prefix.size())) {
            i = i_saved;
            return false;
          }
        }
      }
    }
    if(i >= input.size() - (prefix.size() - 1)) {
      i = i_saved;
      return false;
    }
    insert_with_links_tagged(partial);
    partial.clear();
    auto start_offset = buffer->get_insert()->get_iter().get_offset();
    insert_with_links_tagged(input.substr(start, i - start));
    if(prefix.size() == 1)
      buffer->apply_tag(italic_tag, buffer->get_iter_at_offset(start_offset), buffer->get_insert()->get_iter());
    else if(prefix.size() == 2)
      buffer->apply_tag(bold_tag, buffer->get_iter_at_offset(start_offset), buffer->get_insert()->get_iter());
    i += prefix.size() - 1;
    return true;
  };

  auto insert_code_or_link = [&] {
    if(input[i] == '`') {
      auto i_saved = i;
      i++;
      if(i < input.size()) {
        bool two_backticks = false;
        if(input[i] == '`') {
          two_backticks = true;
          i++;
        }
        if(i < input.size()) {
          auto start = i;
          for(; i < input.size(); i++) {
            if(input[i] == '`') {
              if(!two_backticks)
                break;
              if(i + 1 < input.size() && input[i + 1] == '`')
                break;
            }
          }
          if(i == input.size()) {
            i = i_saved;
            return false;
          }
          insert_with_links_tagged(partial);
          partial.clear();
          if(!two_backticks && i + 1 < input.size() && input[i + 1] == '_') { // Insert link
            const static std::regex regex("^([^<]*?) ?<([^>]+)>$");
            std::smatch sm;
            if(std::regex_match(input.begin() + start, input.begin() + i, sm, regex)) {
              if(sm[1].length() == 0)
                insert_with_links_tagged(sm[2].str());
              else {
                auto start_offset = buffer->get_insert()->get_iter().get_offset();
                buffer->insert_at_cursor(sm[1].str());
                auto start = buffer->get_iter_at_offset(start_offset);
                auto end = buffer->get_insert()->get_iter();
                buffer->apply_tag(link_tag, start, end);
                auto tag = buffer->create_tag();
                buffer->apply_tag(tag, start, end);
                links.emplace(tag, sm[2].str());
              }
            }
            else
              insert_with_links_tagged(input.substr(start, i - start));
            ++i;
          }
          else {
            buffer->insert_with_tag(buffer->get_insert()->get_iter(), input.substr(start, i - start), code_tag);
            if(two_backticks)
              i++;
          }
          return true;
        }
      }
      i = i_saved;
    }
    return false;
  };

  auto insert_code_block = [&] {
    auto i_saved = i;
    if(input[i] == '\n' && i >= 3 && input[i - 1] == '\n' && input[i - 2] == ':' && input[i - 3] == ':' && i + 1 < input.size()) {
      ++i;
      insert_with_links_tagged(partial.substr(0, partial.size() - 2)); // Remove one ':'
      partial.clear();
      buffer->insert_at_cursor("\n\n");
      auto start = i;
      for(; i < input.size(); ++i) {
        if(starts_with(input, i, "\n\n") && i + 2 < input.size() && !is_whitespace_character(i + 2))
          break;
      }
      insert_code(input.substr(start, i - start), "python", true);
      buffer->insert_at_cursor("\n");
      return true;
    }
    while(i < input.size() && input[i] != '\n' && is_whitespace_character(i))
      ++i;
    if(starts_with(input, i, ">>>")) {
      i += 3;
      insert_with_links_tagged(partial);
      partial.clear();
      for(; i < input.size(); ++i) {
        if(starts_with(input, i, "\n\n") && i + 2 < input.size() && !is_whitespace_character(i + 2))
          break;
      }
      insert_code(input.substr(i_saved, i - i_saved), "python", true);
      buffer->insert_at_cursor("\n");
      return true;
    }
    i = i_saved;
    return false;
  };

  for(; i < input.size(); ++i) {
    if(i == 0 || input[i - 1] == '\n') {
      if(insert_code_block() || insert_header())
        continue;
    }
    if(!unescape(i) && (insert_code_or_link() || insert_emphasis()))
      continue;
    partial += input[i];
  }
  if(!partial.empty())
    insert_with_links_tagged(partial);
}

void Tooltip::remove_trailing_newlines() {
  auto end = buffer->end();
  while(end.starts_line() && end.backward_char()) {
  }
  buffer->erase(end, buffer->end());
}

void Tooltips::show(const Gdk::Rectangle &rectangle, bool disregard_drawn) {
  for(auto &tooltip : tooltip_list) {
    tooltip.update();
    if(rectangle.intersects(tooltip.activation_rectangle))
      tooltip.show(disregard_drawn, on_motion);
    else
      tooltip.hide({}, {});
  }
}

void Tooltips::show(bool disregard_drawn) {
  for(auto &tooltip : tooltip_list) {
    tooltip.update();
    tooltip.show(disregard_drawn, on_motion);
  }
}

void Tooltips::hide(const boost::optional<std::pair<int, int>> &last_mouse_pos, const boost::optional<std::pair<int, int>> &mouse_pos) {
  for(auto &tooltip : tooltip_list)
    tooltip.hide(last_mouse_pos, mouse_pos);
}
