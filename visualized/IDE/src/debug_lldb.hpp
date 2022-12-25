#pragma once
#include "mutex.hpp"
#include <boost/filesystem.hpp>
#include <list>
#include <lldb/API/LLDB.h>
#include <thread>
#include <tuple>

namespace Debug {
  class LLDB {
  public:
    class Frame {
    public:
      uint32_t index;
      std::string module_filename;
      boost::filesystem::path file_path;
      std::string function_name;
      int line_nr;
      int line_index;
    };
    class Variable {
    public:
      uint32_t thread_index_id;
      uint32_t frame_index;
      std::string name;
      std::function<std::string()> get_value;
      bool declaration_found;
      boost::filesystem::path file_path;
      int line_nr;
      int line_index;
    };

  private:
    LLDB();
    void destroy_() EXCLUDES(mutex);

    static bool initialized;

  public:
    static LLDB &get() {
      static LLDB instance;
      return instance;
    }

    /// Must be called before application terminates (cannot be placed in destructor sadly)
    static void destroy() {
      if(initialized)
        get().destroy_();
    }

    std::list<std::function<void(const lldb::SBProcess &)>> on_start;
    /// The handlers are not run in the main loop.
    std::list<std::function<void(int exit_status)>> on_exit;
    /// The handlers are not run in the main loop.
    std::list<std::function<void(const lldb::SBEvent &)>> on_event;

    Mutex mutex;

    void start(const std::string &command, const boost::filesystem::path &path = "",
               const std::vector<std::pair<boost::filesystem::path, int>> &breakpoints = {},
               const std::vector<std::string> &startup_commands = {}, const std::string &remote_host = "") EXCLUDES(mutex);
    void continue_debug() EXCLUDES(mutex); // can't use continue as function name
    void stop() EXCLUDES(mutex);
    void kill() EXCLUDES(mutex);
    void step_over() EXCLUDES(mutex);
    void step_into() EXCLUDES(mutex);
    void step_out() EXCLUDES(mutex);
    std::pair<std::string, std::string> run_command(const std::string &command) EXCLUDES(mutex);
    std::vector<Frame> get_backtrace() EXCLUDES(mutex);
    std::vector<Variable> get_variables() EXCLUDES(mutex);
    void select_frame(uint32_t frame_index, uint32_t thread_index_id = 0) EXCLUDES(mutex);

    /// Get value using variable name and location
    std::string get_value(const std::string &variable_name, const boost::filesystem::path &file_path, unsigned int line_nr, unsigned int line_index) EXCLUDES(mutex);
    /// Get value from expression
    std::string get_value(const std::string &expression) EXCLUDES(mutex);
    std::string get_return_value(const boost::filesystem::path &file_path, unsigned int line_nr, unsigned int line_index) EXCLUDES(mutex);

    bool is_invalid() EXCLUDES(mutex);
    bool is_stopped() EXCLUDES(mutex);
    bool is_running() EXCLUDES(mutex);

    void add_breakpoint(const boost::filesystem::path &file_path, int line_nr) EXCLUDES(mutex);
    void remove_breakpoint(const boost::filesystem::path &file_path, int line_nr, int line_count) EXCLUDES(mutex);

    void write(const std::string &buffer) EXCLUDES(mutex);

  private:
    std::tuple<std::vector<std::string>, std::string, std::vector<std::string>> parse_run_arguments(const std::string &command);

    std::unique_ptr<lldb::SBDebugger> debugger GUARDED_BY(mutex);
    std::unique_ptr<lldb::SBListener> listener GUARDED_BY(mutex);
    std::unique_ptr<lldb::SBProcess> process GUARDED_BY(mutex);
    std::thread debug_thread;

    lldb::StateType state GUARDED_BY(mutex);

    size_t buffer_size;
  };
} // namespace Debug
