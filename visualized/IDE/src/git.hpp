#pragma once
#include "mutex.hpp"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <giomm.h>
#include <git2.h>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>

class Git {
  friend class Repository;

public:
  class Error {
    friend class Git;
    std::string message() noexcept;

  public:
    int code = 0;
    operator bool() noexcept { return code != 0; }
  };

  class Repository {
  public:
    class Diff {
    public:
      class Lines {
      public:
        std::vector<std::pair<int, int>> added;
        std::vector<std::pair<int, int>> modified;
        std::vector<int> removed;
      };
      class Hunk {
      public:
        Hunk(int old_start, int old_size, int new_start, int new_size) : old_lines(old_start, old_size), new_lines(new_start, new_size) {}
        /// Start and size
        std::pair<int, int> old_lines;
        /// Start and size
        std::pair<int, int> new_lines;
      };

    private:
      friend class Repository;
      Diff(const boost::filesystem::path &path, git_repository *repository);
      std::shared_ptr<git_blob> blob = nullptr;
      git_diff_options options;

    public:
      Lines get_lines(const std::string &buffer);
      static std::vector<Hunk> get_hunks(const std::string &old_buffer, const std::string &new_buffer);
      std::string get_details(const std::string &buffer, int line_nr);
    };

    class Status {
    public:
      std::unordered_set<std::string> added;
      std::unordered_set<std::string> modified;
    };

  private:
    friend class Git;
    Repository(const boost::filesystem::path &path);

    std::unique_ptr<git_repository, std::function<void(git_repository *)>> repository;

    boost::filesystem::path work_path;
    sigc::connection monitor_changed_connection;
    Mutex saved_status_mutex;
    Status saved_status GUARDED_BY(saved_status_mutex);
    bool has_saved_status GUARDED_BY(saved_status_mutex) = false;

  public:
    ~Repository();

    Status get_status();
    void clear_saved_status();

    boost::filesystem::path get_work_path() noexcept;
    boost::filesystem::path get_path() noexcept;
    static boost::filesystem::path get_root_path(const boost::filesystem::path &path);

    Diff get_diff(const boost::filesystem::path &path);

    std::string get_branch() noexcept;

    Glib::RefPtr<Gio::FileMonitor> monitor;
  };

private:
  static bool initialized GUARDED_BY(mutex);

  /// Mutex for thread safe operations
  static Mutex mutex;

  static Error error GUARDED_BY(mutex);

  /// Call initialize in public static methods
  static void initialize() noexcept REQUIRES(mutex);

  static boost::filesystem::path path(const char *cpath, boost::optional<size_t> cpath_length = {}) noexcept REQUIRES(mutex);

public:
  static std::shared_ptr<Repository> get_repository(const boost::filesystem::path &path);
};
