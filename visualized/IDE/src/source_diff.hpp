#pragma once
#include "dispatcher.hpp"
#include "git.hpp"
#include "mutex.hpp"
#include "source_base.hpp"
#include <atomic>
#include <boost/filesystem.hpp>
#include <map>
#include <set>
#include <thread>

namespace Source {
  class DiffView : virtual public Source::BaseView {
    enum class ParseState {
      idle,
      starting,
      preprocessing,
      processing,
      postprocessing
    };

    class Renderer : public Gsv::GutterRenderer {
    public:
      Renderer();

      Glib::RefPtr<Gtk::TextTag> tag_added;
      Glib::RefPtr<Gtk::TextTag> tag_modified;
      Glib::RefPtr<Gtk::TextTag> tag_removed;
      Glib::RefPtr<Gtk::TextTag> tag_removed_below;
      Glib::RefPtr<Gtk::TextTag> tag_removed_above;

      Gdk::RGBA yellow, red, green;

    protected:
      void draw_vfunc(const Cairo::RefPtr<Cairo::Context> &cr, const Gdk::Rectangle &background_area,
                      const Gdk::Rectangle &cell_area, Gtk::TextIter &start, Gtk::TextIter &end,
                      Gsv::GutterRendererState p6) override;
    };

  public:
    DiffView(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language);
    ~DiffView() override;

    void configure() override;

    void rename(const boost::filesystem::path &path) override;

    void goto_next_diff();
    std::string get_diff_details();

    Mutex canonical_file_path_mutex;
    /// Use canonical path to follow symbolic links
    boost::filesystem::path canonical_file_path GUARDED_BY(canonical_file_path_mutex);

  private:
    std::unique_ptr<Renderer> renderer;
    Dispatcher dispatcher;

    Mutex parse_mutex;

    std::shared_ptr<Git::Repository> repository;
    std::unique_ptr<Git::Repository::Diff> diff GUARDED_BY(parse_mutex);
    std::unique_ptr<Git::Repository::Diff> get_diff();

    std::thread parse_thread;
    std::atomic<ParseState> parse_state;
    std::atomic<bool> parse_stop;
    Glib::ustring parse_buffer GUARDED_BY(parse_mutex);
    sigc::connection buffer_insert_connection;
    sigc::connection buffer_erase_connection;
    sigc::connection monitor_changed_connection;
    sigc::connection delayed_buffer_changed_connection;
    sigc::connection delayed_monitor_changed_connection;
    std::atomic<bool> monitor_changed;

    void update_tags(const Git::Repository::Diff::Lines &diff_lines);
  };
} // namespace Source
