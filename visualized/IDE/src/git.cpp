#include "git.hpp"
#include <cstring>
#include <unordered_map>

bool Git::initialized = false;
Mutex Git::mutex;
Git::Error Git::error;

std::string Git::Error::message() noexcept {
#if LIBGIT2_VER_MAJOR > 0 || (LIBGIT2_VER_MAJOR == 0 && LIBGIT2_VER_MINOR >= 28)
  auto last_error = git_error_last();
#else
  auto last_error = giterr_last();
#endif
  if(!last_error)
    return std::string();
  else
    return last_error->message;
}

Git::Repository::Diff::Diff(const boost::filesystem::path &path, git_repository *repository) {
  blob = std::shared_ptr<git_blob>(nullptr, [](git_blob *blob) {
    if(blob)
      git_blob_free(blob);
  });
  auto spec = "HEAD:" + path.generic_string();
  LockGuard lock(mutex);
  error.code = git_revparse_single(reinterpret_cast<git_object **>(&blob), repository, spec.c_str());
  if(error)
    throw std::runtime_error(error.message());

  git_diff_init_options(&options, GIT_DIFF_OPTIONS_VERSION);
  options.context_lines = 0;
}

Git::Repository::Diff::Lines Git::Repository::Diff::get_lines(const std::string &buffer) {
  Lines lines;
  LockGuard lock(mutex);
  error.code = git_diff_blob_to_buffer(
      blob.get(), nullptr, buffer.c_str(), buffer.size(), nullptr, &options, nullptr, nullptr, [](const git_diff_delta *delta, const git_diff_hunk *hunk, void *payload) {
        // Based on https://github.com/atom/git-diff/blob/master/lib/git-diff-view.coffee
        auto lines = static_cast<Lines *>(payload);
        auto start = hunk->new_start - 1;
        auto end = hunk->new_start + hunk->new_lines - 1;
        if(hunk->old_lines == 0 && hunk->new_lines > 0)
          lines->added.emplace_back(start, end);
        else if(hunk->new_lines == 0 && hunk->old_lines > 0)
          lines->removed.emplace_back(start);
        else
          lines->modified.emplace_back(start, end);
        return 0;
      },
      nullptr, &lines);
  if(error)
    throw std::runtime_error(error.message());
  return lines;
}

std::vector<Git::Repository::Diff::Hunk> Git::Repository::Diff::get_hunks(const std::string &old_buffer, const std::string &new_buffer) {
  std::vector<Git::Repository::Diff::Hunk> hunks;
  LockGuard lock(mutex);
  initialize();
  git_diff_options options;
  git_diff_init_options(&options, GIT_DIFF_OPTIONS_VERSION);
  options.context_lines = 0;
  error.code = git_diff_buffers(
      old_buffer.c_str(), old_buffer.size(), nullptr, new_buffer.c_str(), new_buffer.size(), nullptr, &options, nullptr, nullptr, [](const git_diff_delta *delta, const git_diff_hunk *hunk, void *payload) {
        auto hunks = static_cast<std::vector<Git::Repository::Diff::Hunk> *>(payload);
        hunks->emplace_back(hunk->old_start, hunk->old_lines, hunk->new_start, hunk->new_lines);
        return 0;
      },
      nullptr, &hunks);
  if(error)
    throw std::runtime_error(error.message());
  return hunks;
}

std::string Git::Repository::Diff::get_details(const std::string &buffer, int line_nr) {
  std::pair<std::string, int> details;
  details.second = line_nr;
  LockGuard lock(mutex);
  error.code = git_diff_blob_to_buffer(
      blob.get(), nullptr, buffer.c_str(), buffer.size(), nullptr, &options, nullptr, nullptr, nullptr, [](const git_diff_delta *delta, const git_diff_hunk *hunk, const git_diff_line *line, void *payload) {
        auto details = static_cast<std::pair<std::string, int> *>(payload);
        auto line_nr = details->second;
        auto start = hunk->new_start - 1;
        auto end = hunk->new_start + hunk->new_lines - 1;
        if(line_nr == start || (line_nr >= start && line_nr < end)) {
          if(details->first.empty())
            details->first += std::string(hunk->header, hunk->header_len);
          details->first += line->origin + std::string(line->content, line->content_len);
        }
        return 0;
      },
      &details);
  if(error)
    throw std::runtime_error(error.message());
  return details.first;
}

Git::Repository::Repository(const boost::filesystem::path &path) {
  git_repository *repository_ptr;
  {
    LockGuard lock(mutex);
    error.code = git_repository_open_ext(&repository_ptr, path.generic_string().c_str(), 0, nullptr);
    if(error)
      throw std::runtime_error(error.message());
  }
  repository = std::unique_ptr<git_repository, std::function<void(git_repository *)>>(repository_ptr, [](git_repository *ptr) {
    git_repository_free(ptr);
  });

  work_path = get_work_path();
  if(work_path.empty())
    throw std::runtime_error("Could not find work path");

  auto git_directory = Gio::File::create_for_path(get_path().string());
  monitor = git_directory->monitor_directory(Gio::FileMonitorFlags::FILE_MONITOR_WATCH_MOVES);
  monitor_changed_connection = monitor->signal_changed().connect(
      [this](const Glib::RefPtr<Gio::File> &file,
             const Glib::RefPtr<Gio::File> &,
             Gio::FileMonitorEvent monitor_event) {
        if(monitor_event != Gio::FileMonitorEvent::FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
          this->clear_saved_status();
        }
      },
      false);
}

Git::Repository::~Repository() {
  monitor_changed_connection.disconnect();
}

Git::Repository::Status Git::Repository::get_status() {
  {
    LockGuard lock(saved_status_mutex);
    if(has_saved_status)
      return saved_status;
  }

  struct Data {
    const boost::filesystem::path &work_path;
    Status status = {};
  };
  Data data{work_path};
  {
    LockGuard lock(mutex);
    error.code = git_status_foreach(
        repository.get(), [](const char *path, unsigned int status_flags, void *payload) {
          auto data = static_cast<Data *>(payload);
          bool new_ = false;
          bool modified = false;
          if((status_flags & (GIT_STATUS_INDEX_NEW | GIT_STATUS_WT_NEW)) > 0)
            new_ = true;
          else if((status_flags & (GIT_STATUS_INDEX_MODIFIED | GIT_STATUS_WT_MODIFIED)) > 0)
            modified = true;
          boost::filesystem::path rel_path(path);
          do {
            if(new_)
              data->status.added.emplace((data->work_path / rel_path).generic_string());
            else if(modified)
              data->status.modified.emplace((data->work_path / rel_path).generic_string());
            rel_path = rel_path.parent_path();
          } while(!rel_path.empty());

          return 0;
        },
        &data);

    if(error)
      throw std::runtime_error(error.message());
  }

  LockGuard lock(saved_status_mutex);
  saved_status = std::move(data.status);
  has_saved_status = true;
  return saved_status;
}

void Git::Repository::clear_saved_status() {
  LockGuard lock(saved_status_mutex);
  saved_status = {};
  has_saved_status = false;
}

boost::filesystem::path Git::Repository::get_work_path() noexcept {
  LockGuard lock(mutex);
  return Git::path(git_repository_workdir(repository.get()));
}

boost::filesystem::path Git::Repository::get_path() noexcept {
  LockGuard lock(mutex);
  return Git::path(git_repository_path(repository.get()));
}

boost::filesystem::path Git::Repository::get_root_path(const boost::filesystem::path &path) {
  git_buf root = {nullptr, 0, 0};
  LockGuard lock(mutex);
  initialize();
  error.code = git_repository_discover(&root, path.generic_string().c_str(), 0, nullptr);
  if(error)
    throw std::runtime_error(error.message());
  auto root_path = Git::path(root.ptr, root.size);
#if LIBGIT2_VER_MAJOR > 0 || (LIBGIT2_VER_MAJOR == 0 && LIBGIT2_VER_MINOR >= 28)
  git_buf_dispose(&root);
#else
  git_buf_free(&root);
#endif
  return root_path;
}

Git::Repository::Diff Git::Repository::get_diff(const boost::filesystem::path &path) {
  return Diff(path, repository.get());
}

std::string Git::Repository::get_branch() noexcept {
  std::string branch;
  git_reference *reference;
  LockGuard lock(mutex);
  error.code = git_repository_head(&reference, repository.get());
  if(!error) {
    if(auto reference_name_cstr = git_reference_name(reference)) {
      std::string reference_name(reference_name_cstr);
      size_t pos;
      if((pos = reference_name.rfind('/')) != std::string::npos) {
        if(pos + 1 < reference_name.size())
          branch = reference_name.substr(pos + 1);
      }
      else if((pos = reference_name.rfind('\\')) != std::string::npos) {
        if(pos + 1 < reference_name.size())
          branch = reference_name.substr(pos + 1);
      }
    }
    git_reference_free(reference);
  }
  return branch;
}

void Git::initialize() noexcept {
  if(!initialized) {
    git_libgit2_init();
    initialized = true;
  }
}

std::shared_ptr<Git::Repository> Git::get_repository(const boost::filesystem::path &path) {
  auto root_path = Repository::get_root_path(path).generic_string();

  static Mutex mutex;
  static std::unordered_map<std::string, std::weak_ptr<Git::Repository>> cache GUARDED_BY(mutex);

  LockGuard lock(mutex);
  auto it = cache.find(root_path);
  if(it == cache.end())
    it = cache.emplace(root_path, std::weak_ptr<Git::Repository>()).first;
  auto instance = it->second.lock();
  if(!instance)
    it->second = instance = std::shared_ptr<Repository>(new Repository(root_path));
  return instance;
}

boost::filesystem::path Git::path(const char *cpath, boost::optional<size_t> cpath_length_) noexcept {
  if(!cpath)
    return boost::filesystem::path();
  auto cpath_length = cpath_length_.value_or(strlen(cpath));
  if(cpath_length > 0 && (cpath[cpath_length - 1] == '/' || cpath[cpath_length - 1] == '\\'))
    return std::string(cpath, cpath_length - 1);
  else
    return std::string(cpath, cpath_length);
}
