#pragma once
#include "autocomplete.hpp"
#include "clangmm.hpp"
#include "dispatcher.hpp"
#include "mutex.hpp"
#include "source.hpp"
#include "terminal.hpp"
#include <atomic>
#include <map>
#include <set>
#include <thread>

namespace Source {
  class ClangViewParse : public View {
  protected:
    enum class ParseState {
      processing,
      restarting,
      stop
    };
    enum class ParseProcessState {
      idle,
      starting,
      preprocessing,
      processing,
      postprocessing
    };

  public:
    ClangViewParse(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language);

    void rename(const boost::filesystem::path &path) override;
    bool save() override;
    void configure() override;

    void soft_reparse(bool delayed = false) override;

  protected:
    Dispatcher dispatcher;
    void parse_initialize();
    std::unique_ptr<clangmm::TranslationUnit> clang_tu;
    std::unique_ptr<clangmm::Tokens> clang_tokens;
    std::vector<std::pair<clangmm::Offset, clangmm::Offset>> clang_tokens_offsets;
    sigc::connection delayed_reparse_connection;
    sigc::connection delayed_full_reparse_connection;

    void show_type_tooltips(const Gdk::Rectangle &rectangle) override;

    Mutex parse_mutex;
    std::thread parse_thread;
    std::atomic<ParseState> parse_state;
    std::atomic<ParseProcessState> parse_process_state;

    CXCompletionString selected_completion_string = nullptr;

  private:
    Glib::ustring parse_thread_buffer GUARDED_BY(parse_mutex);

    static const std::map<int, std::string> &clang_types();
    void update_syntax() REQUIRES(parse_mutex);
    std::map<int, Glib::RefPtr<Gtk::TextTag>> syntax_tags;

    void update_diagnostics() REQUIRES(parse_mutex);
    std::vector<clangmm::Diagnostic> clang_diagnostics GUARDED_BY(parse_mutex);

    /// Removes for instance ::__1:: and ::__cxx11:: from type
    void remove_internal_namespaces(std::string &type);
  };

  class ClangViewAutocomplete : public virtual ClangViewParse {
  public:
    ClangViewAutocomplete(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language);

  protected:
    Autocomplete autocomplete;
    std::unique_ptr<clangmm::CodeCompleteResults> code_complete_results;
    std::vector<CXCompletionString> completion_strings;
    sigc::connection delayed_show_arguments_connection;

    std::atomic<bool> enable_snippets = {false};
    std::map<size_t, std::string> snippet_inserts;
    std::map<size_t, std::string> snippet_comments;

  private:
    std::atomic<bool> show_parameters = {false};

    const std::unordered_map<std::string, std::string> &autocomplete_manipulators_map();
  };

  class ClangViewRefactor : public virtual ClangViewParse {
    class Identifier {
    public:
      Identifier(std::string spelling_, const clangmm::Cursor &cursor)
          : kind(cursor.get_kind()), spelling(std::move(spelling_)), usr_extended(cursor.get_usr_extended()), cursor(cursor) {}
      Identifier() : kind(static_cast<clangmm::Cursor::Kind>(0)) {}

      operator bool() const { return static_cast<int>(kind) != 0; }
      bool operator==(const Identifier &rhs) const { return spelling == rhs.spelling && usr_extended == rhs.usr_extended; }
      bool operator!=(const Identifier &rhs) const { return !(*this == rhs); }
      bool operator<(const Identifier &rhs) const { return spelling < rhs.spelling || (spelling == rhs.spelling && usr_extended < rhs.usr_extended); }
      clangmm::Cursor::Kind kind;
      std::string spelling;
      std::string usr_extended;
      clangmm::Cursor cursor;
    };

  public:
    ClangViewRefactor(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language);

  protected:
    void apply_similar_symbol_tag() override;
    void apply_clickable_tag(const Gtk::TextIter &iter) override;

  private:
    Identifier get_identifier();
    bool wait_parsing();
  };

  class ClangView : public ClangViewAutocomplete, public ClangViewRefactor {
  public:
    ClangView(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language);

    void full_reparse() override;
    void async_delete();

  private:
    Glib::Dispatcher do_delete_object;
    std::thread delete_thread;
    std::thread full_reparse_thread;
    bool full_reparse_running = false;
  };
} // namespace Source
