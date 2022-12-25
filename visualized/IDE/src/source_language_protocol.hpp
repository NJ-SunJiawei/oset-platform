#pragma once
#include "autocomplete.hpp"
#include "json.hpp"
#include "mutex.hpp"
#include "process.hpp"
#include "source.hpp"
#include <atomic>
#include <boost/optional.hpp>
#include <list>
#include <map>
#include <set>
#include <sstream>

namespace Source {
  class LanguageProtocolView;
}

namespace LanguageProtocol {
  class Position {
  public:
    Position(const JSON &position);
    int line, character;

    bool operator<(const Position &rhs) const {
      return line < rhs.line || (line == rhs.line && character < rhs.character);
    }
    bool operator==(const Position &rhs) const {
      return line == rhs.line && character == rhs.character;
    }
  };

  class Range {
  public:
    Range(const JSON &range);
    Position start, end;

    bool operator<(const Range &rhs) const {
      return start < rhs.start || (start == rhs.start && end < rhs.end);
    }
    bool operator==(const Range &rhs) const {
      return start == rhs.start && end == rhs.end;
    }
  };

  class Location {
  public:
    Location(const JSON &location, std::string file_ = {});
    Location(std::string _file, Range _range) : file(std::move(_file)), range(std::move(_range)) {}
    std::string file;
    Range range;

    bool operator<(const Location &rhs) const {
      return file < rhs.file || (file == rhs.file && range < rhs.range);
    }
    bool operator==(const Location &rhs) const {
      return file == rhs.file && range == rhs.range;
    }
  };

  class Documentation {
  public:
    Documentation(const boost::optional<JSON> &documentation);
    Documentation(std::string value = {}) : value(std::move(value)) {}
    std::string value;
    std::string kind;
  };

  class Diagnostic {
  public:
    class RelatedInformation {
    public:
      RelatedInformation(const JSON &related_information);
      std::string message;
      Location location;
    };

    Diagnostic(JSON &&diagnostic);
    std::string message;
    Range range;
    /// 1: error, 2: warning, 3: information, 4: hint
    int severity;
    std::string code;
    std::vector<RelatedInformation> related_informations;
    std::map<std::string, std::set<Source::FixIt>> quickfixes;
    /// Diagnostic object for textDocument/codeAction on new diagnostics
    std::shared_ptr<JSON> object;
  };

  class TextEdit {
  public:
    TextEdit(const JSON &text_edit, std::string new_text_ = {});
    Range range;
    std::string new_text;
  };

  class TextDocumentEdit {
  public:
    TextDocumentEdit(const JSON &text_document_edit);
    TextDocumentEdit(std::string file, std::vector<TextEdit> text_edits);

    std::string file;
    std::vector<TextEdit> text_edits;
  };

  struct RenameFile {
    boost::filesystem::path old_path;
    boost::filesystem::path new_path;
  };

  class WorkspaceEdit {
  public:
    WorkspaceEdit() = default;
    WorkspaceEdit(const JSON &workspace_edit, boost::filesystem::path file_path);
    std::vector<boost::variant<TextDocumentEdit, RenameFile>> document_changes;
  };

  class Capabilities {
  public:
    enum class TextDocumentSync { none = 0,
                                  full,
                                  incremental };
    TextDocumentSync text_document_sync = TextDocumentSync::none;
    bool hover = false;
    bool completion = false;
    bool completion_resolve = false;
    bool signature_help = false;
    bool definition = false;
    bool type_definition = false;
    bool implementation = false;
    bool references = false;
    bool document_highlight = false;
    bool workspace_symbol = false;
    bool workspace_folders = false;
    bool document_symbol = false;
    bool document_formatting = false;
    bool document_range_formatting = false;
    bool rename = false;
    bool code_action = false;
    bool code_action_resolve = false;
    bool execute_command = false;
    bool type_coverage = false;
    bool use_line_index = false;
  };

  class Client {
    Client(boost::filesystem::path root_path, std::string language_id, const std::string &language_server);
    boost::filesystem::path root_path;
    std::string language_id;

    Capabilities capabilities;

    /// Dispatcher must be destroyed in main thread
    std::unique_ptr<Dispatcher> dispatcher = std::make_unique<Dispatcher>();

    Mutex views_mutex;
    std::set<Source::LanguageProtocolView *> views GUARDED_BY(views_mutex);

    Mutex initialize_mutex;
    bool initialized GUARDED_BY(initialize_mutex) = false;

    Mutex read_write_mutex;
    std::unique_ptr<TinyProcessLib::Process> process GUARDED_BY(read_write_mutex);

    std::stringstream server_message_stream;
    boost::optional<size_t> server_message_size;
    size_t server_message_content_pos;
    bool header_read = false;

    size_t message_id GUARDED_BY(read_write_mutex) = 0;

    std::map<size_t, std::pair<Source::LanguageProtocolView *, std::function<void(JSON &&result, bool error)>>> handlers GUARDED_BY(read_write_mutex);

    Mutex timeout_threads_mutex;
    std::vector<std::thread> timeout_threads GUARDED_BY(timeout_threads_mutex);

    std::mutex log_mutex;

  public:
    static std::shared_ptr<Client> get(const boost::filesystem::path &file_path, const std::string &language_id, const std::string &language_server);

    ~Client();

    void add(Source::LanguageProtocolView *view);
    boost::optional<Capabilities> get_capabilities();
    Capabilities initialize();
    void remove(Source::LanguageProtocolView *view);

    void parse_server_message();
    void write_request(Source::LanguageProtocolView *view, const std::string &method, const std::string &params, std::function<void(JSON &&result, bool)> &&function = nullptr);
    void write_response(const boost::variant<size_t, std::string> &id, const std::string &result);
    void write_notification(const std::string &method, const std::string &params = {});
    void handle_server_notification(const std::string &method, JSON &&params);
    void handle_server_request(const boost::variant<size_t, std::string> &id, const std::string &method, JSON &&params);

    std::function<void(int exit_status)> on_exit_status;

    /// Detecting pyright language server since workarounds need to be applied
    std::atomic<bool> pyright = {false};
  };
} // namespace LanguageProtocol

namespace Source {
  class LanguageProtocolView : public View {
    friend class LanguageProtocol::Client;

  public:
    LanguageProtocolView(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language, std::string language_id_, std::string language_server_);
    void initialize();
    ~LanguageProtocolView() override;

    void rename(const boost::filesystem::path &path) override;
    bool save() override;

    /// Get line offset depending on if utf-8 byte offsets or utf-16 code units are used
    int get_line_pos(const Gtk::TextIter &iter);
    /// Get line offset depending on if utf-8 byte offsets or utf-16 code units are used
    int get_line_pos(int line, int line_index);

  private:
    std::pair<std::string, std::string> make_position(int line, int character);
    std::pair<std::string, std::string> make_range(const std::pair<int, int> &start, const std::pair<int, int> &end);
    std::string to_string(const std::pair<std::string, std::string> &param);
    std::string to_string(const std::vector<std::pair<std::string, std::string>> &params);
    /// Helper method for calling client->write_request
    void write_request(const std::string &method, const std::string &params, std::function<void(JSON &&result, bool)> &&function);
    /// Helper method for calling client->write_notification
    void write_notification(const std::string &method);
    /// Helper method for calling client->write_notification
    void write_did_open_notification();
    /// Helper method for calling client->write_notification
    void write_did_change_notification(const std::vector<std::pair<std::string, std::string>> &params);

    std::atomic<size_t> update_diagnostics_async_count = {0};
    void update_diagnostics(std::vector<LanguageProtocol::Diagnostic> diagnostics);

  public:
    void update_diagnostics_async(std::vector<LanguageProtocol::Diagnostic> &&diagnostics);

    Gtk::TextIter get_iter_at_line_pos(int line, int pos) override;

    std::string uri;
    std::string uri_escaped;

  protected:
    void show_type_tooltips(const Gdk::Rectangle &rectangle) override;
    void apply_similar_symbol_tag() override;
    void apply_clickable_tag(const Gtk::TextIter &iter) override;

  private:
    bool initialized = false;

    std::string language_id;
    std::string language_server;
    LanguageProtocol::Capabilities capabilities;

    std::shared_ptr<LanguageProtocol::Client> client;

    size_t document_version = 1;

    std::thread initialize_thread;
    Dispatcher dispatcher;
    Glib::ThreadPool thread_pool;

    void setup_navigation_and_refactoring();
    void setup_signals();
    void setup_autocomplete();

    void tag_similar_symbols();

    std::vector<Offset> get_declarations(const Gtk::TextIter &iter);
    std::vector<Offset> get_type_declarations(const Gtk::TextIter &iter);
    std::vector<Offset> get_implementations(const Gtk::TextIter &iter);

    std::unique_ptr<Autocomplete> autocomplete;

    struct AutocompleteRow {
      std::string insert;
      std::string detail;
      LanguageProtocol::Documentation documentation;
      /// CompletionItem for completionItem/resolve
      std::shared_ptr<JSON> item_object;
      std::vector<LanguageProtocol::TextEdit> additional_text_edits;
    };
    std::vector<AutocompleteRow> autocomplete_rows;

    std::atomic<bool> autocomplete_enable_snippets = {false};
    bool autocomplete_show_arguments = false;
    sigc::connection autocomplete_delayed_show_arguments_connection;
    /// Workaround for typescript-language-server.
    /// Used to move cursor forward if no arguments in completed function and if cursor is still inside ()
    bool autocomplete_possibly_no_arguments = false;

    /// If language supports named parameters, returns the symbol separating the named parameter and value,
    /// for instance '=' for Python
    boost::optional<char> get_named_parameter_symbol();

    sigc::connection resolve_completion_item_connection;
    /// Used when calling completionItem/resolve. Also increased in autocomplete->on_hide.
    std::atomic<size_t> set_tooltip_count = {0};

    /// Used by update_type_coverage only (capabilities.type_coverage == true)
    std::vector<LanguageProtocol::Diagnostic> last_diagnostics;

    sigc::connection update_type_coverage_connection;
    std::list<std::pair<Mark, Mark>> type_coverage_marks;
    size_t num_warnings = 0, num_errors = 0, num_fix_its = 0;
    void update_type_coverage();
    std::atomic<int> update_type_coverage_retries = {60};
  };
} // namespace Source
