#include "source_language_protocol.hpp"
#include "directories.hpp"
#include "filesystem.hpp"
#include "info.hpp"
#include "notebook.hpp"
#include "project.hpp"
#include "selection_dialog.hpp"
#include "terminal.hpp"
#ifdef JUCI_ENABLE_DEBUG
#include "debug_lldb.hpp"
#endif
#include "config.hpp"
#include "json.hpp"
#include "menu.hpp"
#include "utility.hpp"
#include <future>
#include <limits>
#include <regex>
#include <unordered_map>

const std::string type_coverage_message = "Un-type checked code. Consider adding type annotations.";

LanguageProtocol::Position::Position(const JSON &position) {
  try {
    line = position.integer("line");
    character = position.integer("character");
  }
  catch(...) {
    // Workaround for buggy rls
    line = std::min(position.integer("line"), std::numeric_limits<long long>::max());
    character = std::min(position.integer("character"), std::numeric_limits<long long>::max());
  }
}
LanguageProtocol::Range::Range(const JSON &range) : start(range.object("start")), end(range.object("end")) {}

LanguageProtocol::Location::Location(const JSON &location, std::string file_) : file(file_.empty() ? filesystem::get_path_from_uri(location.string("uri")).string() : std::move(file_)), range(location.object("range")) {
}

LanguageProtocol::Documentation::Documentation(const boost::optional<JSON> &documentation) {
  if(!documentation)
    return;
  if(auto child = documentation->object_optional()) {
    value = child->string_or("value", "");
    kind = child->string_or("kind", "");
  }
  else
    value = documentation->string_or("");
}

LanguageProtocol::Diagnostic::RelatedInformation::RelatedInformation(const JSON &related_information) : message(related_information.string("message")), location(related_information.object("location")) {}

LanguageProtocol::Diagnostic::Diagnostic(JSON &&diagnostic) : message(diagnostic.string("message")), range(diagnostic.object("range")), severity(diagnostic.integer_or("severity", 0)), code(diagnostic.string_or("code", "")) {
  for(auto &related_information : diagnostic.array_or_empty("relatedInformation"))
    related_informations.emplace_back(related_information);
  object = std::make_shared<JSON>(JSON::make_owner(std::move(diagnostic)));
}

LanguageProtocol::TextEdit::TextEdit(const JSON &text_edit, std::string new_text_) : range(text_edit.object("range")), new_text(new_text_.empty() ? text_edit.string("newText") : std::move(new_text_)) {}

LanguageProtocol::TextDocumentEdit::TextDocumentEdit(const JSON &text_document_edit) : file(filesystem::get_path_from_uri(text_document_edit.object("textDocument").string("uri")).string()) {
  for(auto &text_edit : text_document_edit.array_or_empty("edits"))
    text_edits.emplace_back(text_edit);
}

LanguageProtocol::TextDocumentEdit::TextDocumentEdit(std::string file, std::vector<TextEdit> text_edits) : file(std::move(file)), text_edits(std::move(text_edits)) {}

LanguageProtocol::WorkspaceEdit::WorkspaceEdit(const JSON &workspace_edit, boost::filesystem::path file_path) {
  boost::filesystem::path project_path;
  auto build = Project::Build::create(file_path);
  if(!build->project_path.empty())
    project_path = build->project_path;
  else
    project_path = file_path.parent_path();
  try {
    if(auto changes = workspace_edit.children_optional("changes")) {
      for(auto &change : *changes) {
        auto file = filesystem::get_path_from_uri(change.first);
        if(filesystem::file_in_path(file, project_path)) {
          std::vector<LanguageProtocol::TextEdit> text_edits;
          for(auto &text_edit : change.second.array())
            text_edits.emplace_back(text_edit);
          document_changes.emplace_back(TextDocumentEdit(file.string(), std::move(text_edits)));
        }
      }
    }
    else {
      auto document_changes = workspace_edit.array_or_empty("documentChanges");
      if(document_changes.empty()) {
        if(auto child = workspace_edit.object_optional("documentChanges"))
          document_changes.emplace_back(std::move(*child));
      }
      for(auto &document_change : document_changes) {
        if(auto kind = document_change.string_optional("kind")) {
          if(*kind == "rename") {
            auto old_path = filesystem::get_path_from_uri(document_change.string("oldUri"));
            auto new_path = filesystem::get_path_from_uri(document_change.string("newUri"));
            if(filesystem::file_in_path(old_path, project_path) && filesystem::file_in_path(new_path, project_path))
              this->document_changes.emplace_back(RenameFile{std::move(old_path), std::move(new_path)});
          }
        }
        else {
          LanguageProtocol::TextDocumentEdit document_edit(document_change);
          if(filesystem::file_in_path(document_edit.file, project_path))
            this->document_changes.emplace_back(TextDocumentEdit(std::move(document_edit.file), std::move(document_edit.text_edits)));
        }
      }
    }
  }
  catch(...) {
    document_changes.clear();
  }
}

LanguageProtocol::Client::Client(boost::filesystem::path root_path_, std::string language_id_, const std::string &language_server) : root_path(std::move(root_path_)), language_id(std::move(language_id_)) {
  process = std::make_unique<TinyProcessLib::Process>(
      language_server, root_path.string(),
      [this](const char *bytes, size_t n) {
        server_message_stream.write(bytes, n);
        parse_server_message();
      },
      [](const char *bytes, size_t n) {
        std::cerr.write(bytes, n);
      },
      true, TinyProcessLib::Config{1048576});
}

std::shared_ptr<LanguageProtocol::Client> LanguageProtocol::Client::get(const boost::filesystem::path &file_path, const std::string &language_id, const std::string &language_server) {
  boost::filesystem::path root_path;
  auto build = Project::Build::create(file_path);
  if(!build->project_path.empty())
    root_path = build->project_path;
  else
    root_path = file_path.parent_path();

  auto cache_id = root_path.string() + '|' + language_id;

  static Mutex mutex;
  static std::unordered_map<std::string, std::weak_ptr<Client>> cache GUARDED_BY(mutex);

  LockGuard lock(mutex);
  auto it = cache.find(cache_id);
  if(it == cache.end())
    it = cache.emplace(cache_id, std::weak_ptr<Client>()).first;
  auto instance = it->second.lock();
  if(!instance)
    it->second = instance = std::shared_ptr<Client>(new Client(root_path, language_id, language_server), [](Client *client_ptr) {
      client_ptr->dispatcher = nullptr; // Dispatcher must be destroyed in main thread

      std::thread delete_thread([client_ptr] { // Delete client in the background
        delete client_ptr;
      });
      delete_thread.detach();
    });
  return instance;
}

LanguageProtocol::Client::~Client() {
  std::promise<void> result_processed;
  write_request(nullptr, "shutdown", "", [this, &result_processed](JSON &&result, bool error) {
    if(!error)
      this->write_notification("exit");
    result_processed.set_value();
  });
  result_processed.get_future().get();

  LockGuard lock(timeout_threads_mutex);
  for(auto &thread : timeout_threads)
    thread.join();

  int exit_status = -1;
  for(size_t c = 0; c < 10; ++c) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if(process->try_get_exit_status(exit_status))
      break;
  }
  if(exit_status == -1) {
    process->kill();
    exit_status = process->get_exit_status();
  }

  if(on_exit_status)
    on_exit_status(exit_status);
  if(Config::get().log.language_server) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "Language server exit status: " << exit_status << std::endl;
  }
}

void LanguageProtocol::Client::add(Source::LanguageProtocolView *view) {
  if(view) {
    LockGuard lock(views_mutex);
    views.emplace(view);
  }
}

boost::optional<LanguageProtocol::Capabilities> LanguageProtocol::Client::get_capabilities() {
  LockGuard lock(initialize_mutex);
  if(initialized)
    return capabilities;
  return {};
}

LanguageProtocol::Capabilities LanguageProtocol::Client::initialize() {
  LockGuard lock(initialize_mutex);

  if(initialized)
    return capabilities;

  std::promise<void> result_processed;
  TinyProcessLib::Process::id_type process_id;
  {
    LockGuard lock(read_write_mutex);
    process_id = process->get_id();
  }
  write_request(
      nullptr, "initialize", "\"processId\":" + std::to_string(process_id) + ",\"rootPath\":\"" + JSON::escape_string(root_path.string()) + "\",\"rootUri\":\"" + JSON::escape_string(filesystem::get_uri_from_path(root_path)) + R"(","capabilities": {
  "workspace": {
    "symbol": { "dynamicRegistration": false },
    "executeCommand": { "dynamicRegistration": false },
    "workspaceFolders": true
  },
  "textDocument": {
    "synchronization": { "dynamicRegistration": false, "didSave": true },
    "completion": {
      "dynamicRegistration": false,
      "completionItem": {
        "snippetSupport": true,
        "documentationFormat": ["markdown", "plaintext"],
        "resolveSupport": {
          "properties": ["documentation", "detail"]
        }
      }
    },
    "hover": {
      "dynamicRegistration": false,
      "contentFormat": ["markdown", "plaintext"]
    },
    "signatureHelp": {
      "dynamicRegistration": false,
      "signatureInformation": {
        "documentationFormat": ["markdown", "plaintext"]
      }
    },
    "definition": { "dynamicRegistration": false },
    "typeDefinition": { "dynamicRegistration": false },
    "implementation": { "dynamicRegistration": false },
    "references": { "dynamicRegistration": false },
    "documentHighlight": { "dynamicRegistration": false },
    "documentSymbol": { "dynamicRegistration": false },
    "codeAction": {
      "dynamicRegistration": false,
      "resolveSupport": {
        "properties": ["edit"]
      },
      "codeActionLiteralSupport": {
        "codeActionKind": {
          "valueSet": [
            "",
            "quickfix",
            "refactor",
            "refactor.extract",
            "refactor.inline",
            "refactor.rewrite",
            "source",
            "source.organizeImports"
          ]
        }
      }
    },
    "formatting": { "dynamicRegistration": false },
    "rangeFormatting": { "dynamicRegistration": false },
    "rename": { "dynamicRegistration": false },
    "publishDiagnostics": { "relatedInformation":true }
  }
},
"initializationOptions": {
  "checkOnSave": { "enable": true }
},
"trace": "off")",
      [this, &result_processed](JSON &&result, bool error) {
        if(!error) {
          if(auto object = result.object_optional("capabilities")) {
            if(auto child = object->child_optional("textDocumentSync")) {
              if(auto integer = child->integer_optional())
                capabilities.text_document_sync = static_cast<LanguageProtocol::Capabilities::TextDocumentSync>(*integer);
              else
                capabilities.text_document_sync = static_cast<LanguageProtocol::Capabilities::TextDocumentSync>(child->integer_or("change", 0));
            }
            if(auto child = object->child_optional("completionProvider")) {
              capabilities.completion = true;
              capabilities.completion_resolve = child->boolean_or("resolveProvider", false);
            }
            /// Some server capabilities are reported as either boolean or objects
            auto boolean_or_object = [&object](const std::string &provider) {
              if(auto child = object->child_optional(provider))
                return child->boolean_or(true);
              return false;
            };
            capabilities.hover = boolean_or_object("hoverProvider");
            capabilities.signature_help = static_cast<bool>(object->child_optional("signatureHelpProvider"));
            capabilities.definition = boolean_or_object("definitionProvider");
            capabilities.type_definition = boolean_or_object("typeDefinitionProvider");
            capabilities.implementation = boolean_or_object("implementationProvider");
            capabilities.references = boolean_or_object("referencesProvider");
            capabilities.document_highlight = boolean_or_object("documentHighlightProvider");
            capabilities.document_symbol = boolean_or_object("documentSymbolProvider");
            if(auto child = object->child_optional("codeActionProvider")) {
              capabilities.code_action = child->boolean_or(true); // Can be object as well
              capabilities.code_action_resolve = child->boolean_or("resolveProvider", false);
            }
            capabilities.document_formatting = boolean_or_object("documentFormattingProvider");
            capabilities.document_range_formatting = boolean_or_object("documentRangeFormattingProvider");
            capabilities.rename = boolean_or_object("renameProvider");
            capabilities.execute_command = static_cast<bool>(object->child_optional("executeCommandProvider"));
            capabilities.type_coverage = static_cast<bool>(object->child_optional("typeCoverageProvider"));
            capabilities.workspace_symbol = boolean_or_object("workspaceSymbolProvider");
            if(auto workspace = object->object_optional("workspace")) {
              if(auto workspace_folders = workspace->object_optional("workspaceFolders")) {
                if(auto change_notifications = workspace_folders->child_optional("changeNotifications"))
                  capabilities.workspace_folders = change_notifications->boolean_or(true); // Can be string as well
              }
            }
          }

          // See https://clangd.llvm.org/extensions.html#utf-8-offsets for documentation on offsetEncoding
          // Disabled for now since rust-analyzer does not seem to support utf-8 byte offsets from clients
          // capabilities.use_line_index = result.get<std::string>("offsetEncoding", "") == "utf-8";

          write_notification("initialized");
          if(capabilities.workspace_folders)
            write_notification("workspace/didChangeWorkspaceFolders", "\"event\":{\"added\":[{\"uri\": \"" + JSON::escape_string(filesystem::get_uri_from_path(root_path)) + "\",\"name\":\"" + JSON::escape_string(root_path.filename().string()) + "\"}],\"removed\":[]}");
        }
        result_processed.set_value();
      });
  result_processed.get_future().get();

  initialized = true;
  return capabilities;
}

void LanguageProtocol::Client::remove(Source::LanguageProtocolView *view) {
  {
    LockGuard lock(views_mutex);
    auto it = views.find(view);
    if(it != views.end())
      views.erase(it);
  }
  LockGuard lock(read_write_mutex);
  for(auto it = handlers.begin(); it != handlers.end();) {
    if(it->second.first == view) {
      auto function = std::move(it->second.second);
      it = handlers.erase(it);
      lock.unlock();
      function({}, true);
      lock.lock();
    }
    else
      it++;
  }
}

void LanguageProtocol::Client::parse_server_message() {
  if(!header_read) {
    std::string line;
    while(!header_read && std::getline(server_message_stream, line)) {
      if(!line.empty() && line != "\r") {
        if(starts_with(line, "Content-Length: ")) {
          try {
            server_message_size = static_cast<size_t>(std::stoul(line.substr(16)));
          }
          catch(...) {
          }
        }
      }
      else if(server_message_size) {
        server_message_content_pos = server_message_stream.tellg();
        *server_message_size += server_message_content_pos;
        header_read = true;
      }
    }
  }

  if(header_read) {
    server_message_stream.seekg(0, std::ios::end);
    size_t read_size = server_message_stream.tellg();
    if(read_size >= *server_message_size) {
      std::stringstream tmp;
      if(read_size > *server_message_size) {
        server_message_stream.seekg(*server_message_size, std::ios::beg);
        server_message_stream.seekp(*server_message_size, std::ios::beg);
        for(size_t c = *server_message_size; c < read_size; ++c) {
          tmp.put(server_message_stream.get());
          server_message_stream.put(' ');
        }
      }

      try {
        server_message_stream.seekg(server_message_content_pos, std::ios::beg);
        JSON object(server_message_stream);

        if(Config::get().log.language_server) {
          std::lock_guard<std::mutex> lock(log_mutex);
          std::cout << "language server: " << std::setw(2) << object << '\n';
        }

        {
          LockGuard lock(read_write_mutex);
          if(auto result = object.child_optional("result")) {
            auto it = handlers.find(object.integer("id", JSON::ParseOptions::accept_string));
            if(it != handlers.end()) {
              auto function = std::move(it->second.second);
              handlers.erase(it);
              lock.unlock();
              function(JSON::make_owner(std::move(*result)), false);
              lock.lock();
            }
          }
          else if(auto error = object.child_optional("error")) {
            if(!Config::get().log.language_server) {
              std::lock_guard<std::mutex> lock(log_mutex);
              std::cerr << std::setw(2) << object << '\n';
            }
            auto it = handlers.find(object.integer("id", JSON::ParseOptions::accept_string));
            if(it != handlers.end()) {
              auto function = std::move(it->second.second);
              handlers.erase(it);
              lock.unlock();
              function(JSON::make_owner(std::move(*error)), true);
              lock.lock();
            }
          }
          else if(auto method = object.string_optional("method")) {
            if(auto params = object.object_optional("params")) {
              lock.unlock();
              if(auto id = object.child_optional("id")) {
                if(auto integer = id->integer_optional())
                  handle_server_request(*integer, *method, JSON::make_owner(std::move(*params)));
                else
                  handle_server_request(id->string(), *method, JSON::make_owner(std::move(*params)));
              }
              else
                handle_server_notification(*method, JSON::make_owner(std::move(*params)));
              lock.lock();
            }
          }
        }
      }
      catch(const std::exception &e) {
        auto server_message_stream_str = server_message_stream.str();
        Terminal::get().async_print(std::string("\e[31mError\e[m: failed to parse message from language server: ") + e.what() + " in:\n" + server_message_stream_str.substr(server_message_content_pos < server_message_stream_str.size() ? server_message_content_pos : 0) + "\n", true);
      }
      catch(...) {
        Terminal::get().async_print("\e[31mError\e[m: failed to parse message from language server\n", true);
      }

      server_message_stream = std::stringstream();
      server_message_size.reset();
      header_read = false;

      tmp.seekg(0, std::ios::end);
      if(tmp.tellg() > 0) {
        tmp.seekg(0, std::ios::beg);
        server_message_stream = std::move(tmp);
        parse_server_message();
      }
    }
  }
}

void LanguageProtocol::Client::write_request(Source::LanguageProtocolView *view, const std::string &method, const std::string &params, std::function<void(JSON &&result, bool error)> &&function) {
  LockGuard lock(read_write_mutex);
  if(function) {
    handlers.emplace(message_id, std::make_pair(view, std::move(function)));

    auto message_id = this->message_id;
    LockGuard lock(timeout_threads_mutex);
    timeout_threads.emplace_back([this, message_id] {
      for(size_t c = 0; c < 40; ++c) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500) * (language_id == "julia" ? 100 : 1));
        LockGuard lock(read_write_mutex);
        auto id_it = handlers.find(message_id);
        if(id_it == handlers.end())
          return;
      }
      LockGuard lock(read_write_mutex);
      auto id_it = handlers.find(message_id);
      if(id_it != handlers.end()) {
        Terminal::get().async_print("\e[33mWarning\e[m: request to language server timed out. If you suspect the server has crashed, please close and reopen all project source files.\n", true);
        auto function = std::move(id_it->second.second);
        handlers.erase(id_it);
        lock.unlock();
        function({}, true);
        lock.lock();
      }
    });
  }
  std::string content("{\"jsonrpc\":\"2.0\",\"id\":" + std::to_string(message_id++) + ",\"method\":\"" + method + "\"" + (params.empty() ? "" : ",\"params\":{" + params + '}') + '}');
  if(Config::get().log.language_server) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "Language client: " << std::setw(2) << JSON(content) << std::endl;
  }
  if(!process->write("Content-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content)) {
    Terminal::get().async_print("\e[31mError\e[m: could not write to language server. Please close and reopen all project files.\n", true);
    auto id_it = handlers.find(message_id - 1);
    if(id_it != handlers.end()) {
      auto function = std::move(id_it->second.second);
      handlers.erase(id_it);
      lock.unlock();
      function({}, true);
      lock.lock();
    }
  }
}

void LanguageProtocol::Client::write_response(const boost::variant<size_t, std::string> &id, const std::string &result) {
  LockGuard lock(read_write_mutex);
  auto integer = boost::get<size_t>(&id);
  std::string content("{\"jsonrpc\":\"2.0\",\"id\":" + (integer ? std::to_string(*integer) : '"' + boost::get<std::string>(id) + '"') + ",\"result\":" + result + "}");
  if(Config::get().log.language_server) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "Language client: " << std::setw(2) << JSON(content) << std::endl;
  }
  process->write("Content-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content);
}

void LanguageProtocol::Client::write_notification(const std::string &method, const std::string &params) {
  LockGuard lock(read_write_mutex);
  std::string content("{\"jsonrpc\":\"2.0\",\"method\":\"" + method + "\",\"params\":{" + params + "}}");
  if(Config::get().log.language_server) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << "Language client: " << std::setw(2) << JSON(content) << std::endl;
  }
  process->write("Content-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content);
}

void LanguageProtocol::Client::handle_server_notification(const std::string &method, JSON &&params) {
  if(method == "textDocument/publishDiagnostics") {
    std::vector<Diagnostic> diagnostics;
    auto file = filesystem::get_path_from_uri(params.string_or("uri", ""));
    if(!file.empty()) {
      for(auto &child : params.array_or_empty("diagnostics")) {
        try {
          diagnostics.emplace_back(std::move(child));
        }
        catch(...) {
        }
      }
      LockGuard lock(views_mutex);
      for(auto view : views) {
        if(file == view->file_path) {
          view->update_diagnostics_async(std::move(diagnostics));
          break;
        }
      }
    }
  }
  else if(method == "window/logMessage") {
    if(language_id == "python" && !pyright) {
      if(auto message = params.string_optional("message")) {
        if(starts_with(*message, "Pyright language server"))
          pyright = true;
      }
    }
  }
}

void LanguageProtocol::Client::handle_server_request(const boost::variant<size_t, std::string> &id, const std::string &method, JSON &&params) {
  if(method == "workspace/applyEdit") {
    std::promise<void> result_processed;
    bool applied = true;
    dispatcher->post([&result_processed, &applied, params = std::make_shared<JSON>(std::move(params))] {
      ScopeGuard guard({[&result_processed] {
        result_processed.set_value();
      }});
      if(auto current_view = dynamic_cast<Source::LanguageProtocolView *>(Notebook::get().get_current_view())) {
        LanguageProtocol::WorkspaceEdit workspace_edit;
        try {
          workspace_edit = LanguageProtocol::WorkspaceEdit(params->object("edit"), current_view->file_path);
        }
        catch(...) {
          applied = false;
          return;
        }

        for(auto &document_change : workspace_edit.document_changes) {
          if(auto edit = boost::get<TextDocumentEdit>(&document_change)) {
            Source::View *view = nullptr;
            for(auto &e : Source::View::views) {
              if(e->file_path == edit->file) {
                view = e;
                break;
              }
            }
            if(!view) {
              if(!Notebook::get().open(edit->file)) {
                applied = false;
                return;
              }
              view = Notebook::get().get_current_view();
            }

            auto buffer = view->get_buffer();
            buffer->begin_user_action();

            auto end_iter = buffer->end();
            // If entire buffer is replaced
            if(edit->text_edits.size() == 1 &&
               edit->text_edits[0].range.start.line == 0 && edit->text_edits[0].range.start.character == 0 &&
               (edit->text_edits[0].range.end.line > end_iter.get_line() ||
                (edit->text_edits[0].range.end.line == end_iter.get_line() && edit->text_edits[0].range.end.character >= current_view->get_line_pos(end_iter)))) {
              view->replace_text(edit->text_edits[0].new_text);
            }
            else {
              for(auto text_edit_it = edit->text_edits.rbegin(); text_edit_it != edit->text_edits.rend(); ++text_edit_it) {
                auto start_iter = view->get_iter_at_line_pos(text_edit_it->range.start.line, text_edit_it->range.start.character);
                auto end_iter = view->get_iter_at_line_pos(text_edit_it->range.end.line, text_edit_it->range.end.character);
                if(view != current_view)
                  view->get_buffer()->place_cursor(start_iter);
                buffer->erase(start_iter, end_iter);
                start_iter = view->get_iter_at_line_pos(text_edit_it->range.start.line, text_edit_it->range.start.character);
                buffer->insert(start_iter, text_edit_it->new_text);
              }
            }

            buffer->end_user_action();
          }
        }

        if(current_view)
          Notebook::get().open(current_view);
      }
    });
    result_processed.get_future().get();
    write_response(id, std::string("{\"applied\":") + (applied ? "true" : "false") + '}');
  }
  else if(method == "workspace/workspaceFolders")
    write_response(id, "[{\"uri\": \"" + JSON::escape_string(filesystem::get_uri_from_path(root_path)) + "\",\"name\":\"" + JSON::escape_string(root_path.filename().string()) + "\"}]");
  else if(method == "client/registerCapability") {
    try {
      for(auto &registration : params.array("registrations")) {
        if(registration.string("method") == "workspace/didChangeWorkspaceFolders") {
          write_notification("workspace/didChangeWorkspaceFolders", "\"event\":{\"added\":[{\"uri\": \"" + JSON::escape_string(filesystem::get_uri_from_path(root_path)) + "\",\"name\":\"" + JSON::escape_string(root_path.filename().string()) + "\"}],\"removed\":[]}");
          if(pyright) {
            dispatcher->post([this] {
              LockGuard lock(views_mutex);
              for(auto &view : views) {
                view->write_notification("textDocument/didClose");
                view->write_did_open_notification();
              }
            });
          }
        }
      }
    }
    catch(...) {
    }
    write_response(id, "{}");
  }
  else
    write_response(id, "{}"); // TODO: write error instead on unsupported methods
}

Source::LanguageProtocolView::LanguageProtocolView(const boost::filesystem::path &file_path, const Glib::RefPtr<Gsv::Language> &language, std::string language_id_, std::string language_server_)
    : Source::BaseView(file_path, language), Source::View(file_path, language), uri(filesystem::get_uri_from_path(file_path)), uri_escaped(JSON::escape_string(uri)), language_id(std::move(language_id_)), language_server(std::move(language_server_)), client(LanguageProtocol::Client::get(file_path, language_id, language_server)) {
  initialize();
}

void Source::LanguageProtocolView::initialize() {
  status_diagnostics = std::make_tuple(0, 0, 0);
  if(update_status_diagnostics)
    update_status_diagnostics(this);

  status_state = "initializing...";
  if(update_status_state)
    update_status_state(this);

  set_editable(false);

  auto init = [this](const LanguageProtocol::Capabilities &capabilities) {
    this->capabilities = capabilities;
    set_editable(true);

    write_did_open_notification();

    if(!initialized) {
      setup_signals();
      setup_autocomplete();
      setup_navigation_and_refactoring();
      Menu::get().toggle_menu_items();
    }

    if(status_state == "initializing...") {
      status_state = "";
      if(update_status_state)
        update_status_state(this);
    }

    update_type_coverage();

    initialized = true;
  };

  client->add(this);
  if(auto capabilities = client->get_capabilities())
    init(*capabilities);
  else {
    initialize_thread = std::thread([this, init] {
      auto capabilities = client->initialize();
      dispatcher.post([init, capabilities] {
        init(capabilities);
      });
    });
  }
}

Source::LanguageProtocolView::~LanguageProtocolView() {
  autocomplete_delayed_show_arguments_connection.disconnect();
  resolve_completion_item_connection.disconnect();
  update_type_coverage_connection.disconnect();

  if(initialize_thread.joinable())
    initialize_thread.join();

  if(autocomplete) {
    autocomplete->state = Autocomplete::State::idle;
    if(autocomplete->thread.joinable())
      autocomplete->thread.join();
  }

  write_notification("textDocument/didClose");
  thread_pool.shutdown(true);
  client->remove(this);
}

int Source::LanguageProtocolView::get_line_pos(const Gtk::TextIter &iter) {
  if(capabilities.use_line_index)
    return iter.get_line_index();
  return utf16_code_unit_count(get_line(iter), 0, iter.get_line_index());
}

int Source::LanguageProtocolView::get_line_pos(int line, int line_index) {
  if(capabilities.use_line_index)
    return line_index;
  return utf16_code_unit_count(get_line(line), 0, line_index);
}

Gtk::TextIter Source::LanguageProtocolView::get_iter_at_line_pos(int line, int pos) {
  if(capabilities.use_line_index)
    return get_iter_at_line_index(line, pos);
  return get_iter_at_line_index(line, utf16_code_units_byte_count(get_line(line), pos));
}

std::pair<std::string, std::string> Source::LanguageProtocolView::make_position(int line, int character) {
  return {"position", "{\"line\":" + std::to_string(line) + ",\"character\":" + std::to_string(character) + "}"};
}

std::pair<std::string, std::string> Source::LanguageProtocolView::make_range(const std::pair<int, int> &start, const std::pair<int, int> &end) {
  return {"range", "{\"start\":{\"line\":" + std::to_string(start.first) + ",\"character\":" + std::to_string(start.second) + "},\"end\":{\"line\":" + std::to_string(end.first) + ",\"character\":" + std::to_string(end.second) + "}}"};
}

std::string Source::LanguageProtocolView::to_string(const std::pair<std::string, std::string> &param) {
  std::string result;
  result.reserve(param.first.size() + param.second.size() + 3);
  result += '"';
  result += param.first;
  result += "\":";
  result += param.second;
  return result;
}

std::string Source::LanguageProtocolView::to_string(const std::vector<std::pair<std::string, std::string>> &params) {
  size_t size = params.empty() ? 0 : params.size() - 1;
  for(auto &param : params)
    size += param.first.size() + param.second.size() + 3;

  std::string result;
  result.reserve(size);
  for(auto &param : params) {
    if(!result.empty())
      result += ',';
    result += '"';
    result += param.first;
    result += "\":";
    result += param.second;
  }
  return result;
}

void Source::LanguageProtocolView::write_request(const std::string &method, const std::string &params, std::function<void(JSON &&result, bool)> &&function) {
  client->write_request(this, method, "\"textDocument\":{\"uri\":\"" + uri_escaped + "\"}" + (params.empty() ? "" : "," + params), std::move(function));
}

void Source::LanguageProtocolView::write_notification(const std::string &method) {
  client->write_notification(method, "\"textDocument\":{\"uri\":\"" + uri_escaped + "\"}");
}

void Source::LanguageProtocolView::write_did_open_notification() {
  document_version = 1;
  client->write_notification("textDocument/didOpen", "\"textDocument\":{\"uri\":\"" + uri_escaped + "\",\"version\":" + std::to_string(document_version++) + ",\"languageId\":\"" + language_id + "\",\"text\":\"" + JSON::escape_string(get_buffer()->get_text().raw()) + "\"}");
}

void Source::LanguageProtocolView::write_did_change_notification(const std::vector<std::pair<std::string, std::string>> &params) {
  client->write_notification("textDocument/didChange", "\"textDocument\":{\"uri\":\"" + uri_escaped + "\",\"version\":" + std::to_string(document_version++) + (params.empty() ? "}" : "}," + to_string(params)));
}

void Source::LanguageProtocolView::rename(const boost::filesystem::path &path) {
  while(!initialized) {
    while(Gtk::Main::events_pending())
      Gtk::Main::iteration();
  }
  write_notification("textDocument/didClose");

  while(thread_pool.unprocessed()) // TODO: does this include tasks in progress?
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  client->remove(this);
  dispatcher.reset();

  Source::DiffView::rename(path);
  uri = filesystem::get_uri_from_path(path);
  uri_escaped = JSON::escape_string(uri);

  client->add(this);
  write_did_open_notification();
}

bool Source::LanguageProtocolView::save() {
  if(!Source::View::save())
    return false;

  write_notification("textDocument/didSave");

  update_type_coverage();

  return true;
}

void Source::LanguageProtocolView::setup_navigation_and_refactoring() {
  if(capabilities.document_formatting && !(format_style && is_js /* Use Prettier instead */)) {
    format_style = [this](bool continue_without_style_file, bool ignore_selection) {
      if(!continue_without_style_file) {
        bool has_style_file = false;
        auto search_path = file_path.parent_path();
        auto style_file = '.' + language_id + "-format";

        boost::system::error_code ec;
        while(true) {
          if(boost::filesystem::exists(search_path / style_file, ec)) {
            has_style_file = true;
            break;
          }
          if(search_path == search_path.root_directory())
            break;
          search_path = search_path.parent_path();
        }

        if(!has_style_file) {
          if(language_id == "rust") {
            auto search_path = file_path.parent_path();
            while(true) {
              if(boost::filesystem::exists(search_path / "rustfmt.toml", ec) ||
                 boost::filesystem::exists(search_path / ".rustfmt.toml", ec)) {
                has_style_file = true;
                break;
              }
              if(search_path == search_path.root_directory())
                break;
              search_path = search_path.parent_path();
            }
          }
          else if(language_id == "python") {
            auto search_path = file_path.parent_path();
            while(true) {
              if(boost::filesystem::exists(search_path / ".style.yapf", ec)) {
                has_style_file = true;
                break;
              }
              if(search_path == search_path.root_directory())
                break;
              search_path = search_path.parent_path();
            }
          }
        }

        if(!has_style_file)
          return;
      }

      std::vector<LanguageProtocol::TextEdit> text_edits;
      std::promise<void> result_processed;

      std::string method;
      std::vector<std::pair<std::string, std::string>> params = {{"options", '{' + to_string({{"tabSize", std::to_string(tab_size)}, {"insertSpaces", tab_char == ' ' ? "true" : "false"}}) + '}'}};
      if(!ignore_selection && get_buffer()->get_has_selection() && capabilities.document_range_formatting) {
        method = "textDocument/rangeFormatting";
        Gtk::TextIter start, end;
        get_buffer()->get_selection_bounds(start, end);
        params.emplace_back(make_range({start.get_line(), get_line_pos(start)}, {end.get_line(), get_line_pos(end)}));
      }
      else
        method = "textDocument/formatting";

      write_request(method, to_string(params), [&text_edits, &result_processed](JSON &&result, bool error) {
        if(!error) {
          for(auto &edit : result.array_or_empty()) {
            try {
              text_edits.emplace_back(edit);
            }
            catch(...) {
            }
          }
        }
        result_processed.set_value();
      });
      result_processed.get_future().get();

      auto end_iter = get_buffer()->end();
      // If entire buffer is replaced:
      if(text_edits.size() == 1 &&
         text_edits[0].range.start.line == 0 && text_edits[0].range.start.character == 0 &&
         (text_edits[0].range.end.line > end_iter.get_line() ||
          (text_edits[0].range.end.line == end_iter.get_line() && text_edits[0].range.end.character >= get_line_pos(end_iter)))) {
        replace_text(text_edits[0].new_text);
      }
      else {
        get_buffer()->begin_user_action();
        for(auto it = text_edits.rbegin(); it != text_edits.rend(); ++it) {
          auto start = get_iter_at_line_pos(it->range.start.line, it->range.start.character);
          auto end = get_iter_at_line_pos(it->range.end.line, it->range.end.character);
          get_buffer()->erase(start, end);
          start = get_iter_at_line_pos(it->range.start.line, it->range.start.character);
          get_buffer()->insert(start, it->new_text);
        }
        get_buffer()->end_user_action();
      }
    };
  }

  if(capabilities.definition) {
    get_declaration_locations = [this]() {
      auto offsets = get_declarations(get_buffer()->get_insert()->get_iter());
      if(offsets.empty())
        Info::get().print("No declaration found");
      return offsets;
    };

    // Assumes that capabilities.definition is available when capabilities.implementation is supported
    if(capabilities.implementation) {
      get_declaration_or_implementation_locations = [this]() {
        // Try implementation locations first
        auto iter = get_buffer()->get_insert()->get_iter();
        auto offsets = get_implementations(iter);
        auto token_iters = get_token_iters(iter);
        bool is_implementation = false;
        for(auto &offset : offsets) {
          if(offset.file_path == file_path && get_iter_at_line_pos(offset.line, offset.index) == token_iters.first) {
            is_implementation = true;
            break;
          }
        }
        if(offsets.empty() || is_implementation) {
          auto offsets = get_declaration_locations();
          if(!offsets.empty())
            return offsets;
        }
        return offsets;
      };
    }
    else {
      get_declaration_or_implementation_locations = [this]() {
        return get_declaration_locations();
      };
    }
  }
  if(capabilities.type_definition) {
    get_type_declaration_locations = [this]() {
      auto offsets = get_type_declarations(get_buffer()->get_insert()->get_iter());
      if(offsets.empty())
        Info::get().print("No type declaration found");
      return offsets;
    };
  }
  if(capabilities.implementation) {
    get_implementation_locations = [this]() {
      auto offsets = get_implementations(get_buffer()->get_insert()->get_iter());
      if(offsets.empty())
        Info::get().print("No implementation found");
      return offsets;
    };
  }

  if(capabilities.references || capabilities.document_highlight) {
    get_usages = [this] {
      auto iter = get_buffer()->get_insert()->get_iter();
      std::set<LanguageProtocol::Location> locations;
      std::promise<void> result_processed;

      std::string method;
      if(capabilities.references)
        method = "textDocument/references";
      else
        method = "textDocument/documentHighlight";

      write_request(method, to_string({make_position(iter.get_line(), get_line_pos(iter)), {"context", "{\"includeDeclaration\":true}"}}), [this, &locations, &result_processed](JSON &&result, bool error) {
        if(!error) {
          try {
            for(auto &location : result.array_or_empty())
              locations.emplace(location, !capabilities.references ? file_path.string() : std::string());
          }
          catch(...) {
            locations.clear();
          }
        }
        result_processed.set_value();
      });
      result_processed.get_future().get();

      auto embolden_token = [this](std::string &line, int token_start_pos, int token_end_pos) {
        // Markup token as bold
        size_t pos = 0;
        while((pos = line.find('&', pos)) != std::string::npos) {
          size_t pos2 = line.find(';', pos + 2);
          if(static_cast<size_t>(token_start_pos) > pos) {
            token_start_pos += pos2 - pos;
            token_end_pos += pos2 - pos;
          }
          else if(static_cast<size_t>(token_end_pos) > pos)
            token_end_pos += pos2 - pos;
          else
            break;
          pos = pos2 + 1;
        }

        if(!capabilities.use_line_index) {
          auto code_units_diff = token_end_pos - token_start_pos;
          token_start_pos = utf16_code_units_byte_count(line, token_start_pos);
          token_end_pos = token_start_pos + utf16_code_units_byte_count(line, code_units_diff, token_start_pos);
        }

        if(static_cast<size_t>(token_start_pos) > line.size())
          token_start_pos = line.size();
        if(static_cast<size_t>(token_end_pos) > line.size())
          token_end_pos = line.size();
        if(token_start_pos < token_end_pos) {
          line.insert(token_end_pos, "</b>");
          line.insert(token_start_pos, "<b>");
        }

        size_t start_pos = 0;
        while(start_pos < line.size() && (line[start_pos] == ' ' || line[start_pos] == '\t'))
          ++start_pos;
        if(start_pos > 0)
          line.erase(0, start_pos);
      };

      std::unordered_map<std::string, std::vector<std::string>> file_lines;
      std::vector<std::pair<Offset, std::string>> usages;
      auto c = static_cast<size_t>(-1);
      for(auto &location : locations) {
        ++c;
        usages.emplace_back(Offset(location.range.start.line, location.range.start.character, location.file), std::string());
        auto &usage = usages.back();
        Source::View *view = nullptr;
        for(auto &e : views) {
          if(e->file_path == location.file) {
            view = e;
            break;
          }
        }
        if(view) {
          if(location.range.start.line < view->get_buffer()->get_line_count()) {
            auto start = view->get_buffer()->get_iter_at_line(location.range.start.line);
            auto end = start;
            end.forward_to_line_end();
            usage.second = Glib::Markup::escape_text(view->get_buffer()->get_text(start, end));
            embolden_token(usage.second, location.range.start.character, location.range.end.character);
          }
        }
        else {
          auto it = file_lines.find(location.file);
          if(it == file_lines.end()) {
            std::ifstream ifs(location.file);
            if(ifs) {
              std::vector<std::string> lines;
              std::string line;
              while(std::getline(ifs, line)) {
                if(!line.empty() && line.back() == '\r')
                  line.pop_back();
                lines.emplace_back(line);
              }
              auto pair = file_lines.emplace(location.file, lines);
              it = pair.first;
            }
            else {
              auto pair = file_lines.emplace(location.file, std::vector<std::string>());
              it = pair.first;
            }
          }

          if(static_cast<size_t>(location.range.start.line) < it->second.size()) {
            usage.second = Glib::Markup::escape_text(it->second[location.range.start.line]);
            embolden_token(usage.second, location.range.start.character, location.range.end.character);
          }
        }
      }

      if(locations.empty())
        Info::get().print("No symbol found at current cursor location");

      return usages;
    };
  }

  get_token_spelling = [this] {
    auto spelling = get_token(get_buffer()->get_insert()->get_iter());
    if(spelling.empty())
      Info::get().print("No valid symbol found at current cursor location");
    return spelling;
  };

  if(capabilities.rename || capabilities.document_highlight) {
    rename_similar_tokens = [this](const std::string &text) {
      auto previous_text = get_token(get_buffer()->get_insert()->get_iter());
      if(previous_text.empty())
        return;

      auto iter = get_buffer()->get_insert()->get_iter();
      LanguageProtocol::WorkspaceEdit workspace_edit;
      std::promise<void> result_processed;
      if(capabilities.rename) {
        write_request("textDocument/rename", to_string({make_position(iter.get_line(), get_line_pos(iter)), {"newName", '"' + text + '"'}}), [this, &workspace_edit, &result_processed](JSON &&result, bool error) {
          if(!error) {
            workspace_edit = LanguageProtocol::WorkspaceEdit(result, file_path);
          }
          result_processed.set_value();
        });
      }
      else {
        write_request("textDocument/documentHighlight", to_string({make_position(iter.get_line(), get_line_pos(iter)), {"context", "{\"includeDeclaration\":true}"}}), [this, &workspace_edit, &text, &result_processed](JSON &&result, bool error) {
          if(!error) {
            try {
              std::vector<LanguageProtocol::TextEdit> edits;
              for(auto &edit : result.array_or_empty())
                edits.emplace_back(edit, text);
              workspace_edit.document_changes.emplace_back(LanguageProtocol::TextDocumentEdit(file_path.string(), std::move(edits)));
            }
            catch(...) {
              workspace_edit.document_changes.clear();
            }
          }
          result_processed.set_value();
        });
      }
      result_processed.get_future().get();

      if(workspace_edit.document_changes.empty())
        return;

      Terminal::get().print("Renamed ");
      Terminal::get().print(previous_text, true);
      Terminal::get().print(" to ");
      Terminal::get().print(text, true);
      Terminal::get().print(" at:\n");

      auto current_view = Notebook::get().get_current_view();

      std::set<Source::View *> views_to_be_saved;
      std::set<Source::View *> views_to_be_closed;

      bool update_directories = false;
      for(auto &document_change : workspace_edit.document_changes) {
        if(auto edit = boost::get<LanguageProtocol::TextDocumentEdit>(&document_change)) {
          Source::View *view = nullptr;
          for(auto &e : views) {
            if(e->file_path == edit->file) {
              view = e;
              break;
            }
          }
          if(!view) {
            if(!Notebook::get().open(edit->file))
              return;
            view = Notebook::get().get_current_view();
            views_to_be_closed.emplace(view);
          }
          views_to_be_saved.emplace(view);

          auto buffer = view->get_buffer();
          buffer->begin_user_action();

          auto end_iter = buffer->end();
          // If entire buffer is replaced
          if(edit->text_edits.size() == 1 &&
             edit->text_edits[0].range.start.line == 0 && edit->text_edits[0].range.start.character == 0 &&
             (edit->text_edits[0].range.end.line > end_iter.get_line() ||
              (edit->text_edits[0].range.end.line == end_iter.get_line() && edit->text_edits[0].range.end.character >= get_line_pos(end_iter)))) {
            view->replace_text(edit->text_edits[0].new_text);

            Terminal::get().print(filesystem::get_short_path(view->file_path).string() + ":1:1\n");
          }
          else {
            struct TerminalOutput {
              std::string prefix;
              std::string new_text;
              std::string postfix;
            };
            std::list<TerminalOutput> terminal_output_list;

            for(auto text_edit_it = edit->text_edits.rbegin(); text_edit_it != edit->text_edits.rend(); ++text_edit_it) {
              auto start_iter = view->get_iter_at_line_pos(text_edit_it->range.start.line, text_edit_it->range.start.character);
              auto end_iter = view->get_iter_at_line_pos(text_edit_it->range.end.line, text_edit_it->range.end.character);
              if(view != current_view)
                view->get_buffer()->place_cursor(start_iter);
              buffer->erase(start_iter, end_iter);
              start_iter = view->get_iter_at_line_pos(text_edit_it->range.start.line, text_edit_it->range.start.character);

              auto start_line_iter = buffer->get_iter_at_line(start_iter.get_line());
              while((*start_line_iter == ' ' || *start_line_iter == '\t') && start_line_iter < start_iter && start_line_iter.forward_char()) {
              }
              auto end_line_iter = start_iter;
              while(!end_line_iter.ends_line() && end_line_iter.forward_char()) {
              }
              terminal_output_list.emplace_front(TerminalOutput{filesystem::get_short_path(view->file_path).string() + ':' + std::to_string(text_edit_it->range.start.line + 1) + ':' + std::to_string(text_edit_it->range.start.character + 1) + ": " + buffer->get_text(start_line_iter, start_iter),
                                                                text_edit_it->new_text,
                                                                buffer->get_text(start_iter, end_line_iter) + '\n'});

              buffer->insert(start_iter, text_edit_it->new_text);
            }

            for(auto &output : terminal_output_list) {
              Terminal::get().print(output.prefix);
              Terminal::get().print(output.new_text, true);
              Terminal::get().print(output.postfix);
            }
          }

          buffer->end_user_action();
        }
        else if(auto rename_file = boost::get<LanguageProtocol::RenameFile>(&document_change)) {
          Source::View *view = nullptr;
          for(auto &e : views) {
            if(e->file_path == rename_file->old_path) {
              view = e;
              break;
            }
          }
          if(!view) {
            if(!Notebook::get().open(rename_file->old_path))
              return;
            view = Notebook::get().get_current_view();
            views_to_be_closed.emplace(view);
          }
          boost::system::error_code ec;
          boost::filesystem::rename(rename_file->old_path, rename_file->new_path, ec);
          if(ec) {
            Terminal::get().print("\e[31mError\e[m: could not rename file: " + ec.message() + '\n', true);
            return;
          }
          view->rename(rename_file->new_path);
          update_directories = true;
        }
      }

      for(auto &view : views_to_be_saved)
        view->save();

      if(update_directories)
        Directories::get().update();

      if(current_view)
        Notebook::get().open(current_view);

      for(auto &view : views_to_be_closed)
        Notebook::get().close(view);
    };
  }

  if(capabilities.document_symbol) {
    get_methods = [this]() {
      std::vector<std::pair<Offset, std::string>> methods;

      std::promise<void> result_processed;
      write_request("textDocument/documentSymbol", {}, [&result_processed, &methods](JSON &&result, bool error) {
        if(!error) {
          std::function<void(const JSON &symbols, const std::string &container)> parse_result = [&methods, &parse_result](const JSON &symbols, const std::string &container) {
            for(auto &symbol : symbols.array_or_empty()) {
              try {
                auto name = symbol.string("name");
                auto kind = symbol.integer("kind");
                if(kind == 6 || kind == 9 || kind == 12) {
                  std::unique_ptr<LanguageProtocol::Range> range;
                  std::string prefix;
                  if(auto location_object = symbol.object_optional("location")) {
                    LanguageProtocol::Location location(*location_object);
                    range = std::make_unique<LanguageProtocol::Range>(location.range);
                    std::string container = symbol.string_or("containerName", "");
                    if(!container.empty())
                      prefix = container;
                  }
                  else {
                    range = std::make_unique<LanguageProtocol::Range>(symbol.object("range"));
                    if(!container.empty())
                      prefix = container;
                  }
                  methods.emplace_back(Offset(range->start.line, range->start.character), (!prefix.empty() ? Glib::Markup::escape_text(prefix) + ':' : "") + std::to_string(range->start.line + 1) + ": " + "<b>" + Glib::Markup::escape_text(name) + "</b>");
                }
                if(auto children = symbol.child_optional("children"))
                  parse_result(*children, (!container.empty() ? container + "::" : "") + name);
              }
              catch(...) {
              }
            }
          };
          parse_result(result, "");
        }
        result_processed.set_value();
      });
      result_processed.get_future().get();

      std::sort(methods.begin(), methods.end(), [](const std::pair<Offset, std::string> &a, const std::pair<Offset, std::string> &b) {
        return a.first < b.first;
      });
      return methods;
    };
  }

  goto_next_diagnostic = [this]() {
    place_cursor_at_next_diagnostic();
  };

  get_fix_its = [this]() {
    if(!fix_its.empty())
      return fix_its;

    if(!capabilities.code_action) {
      Info::get().print("No fix-its found in current buffer");
      return std::vector<FixIt>{};
    }

    // Fetch code actions if no fix-its are available
    std::promise<void> result_processed;
    Gtk::TextIter start, end;
    get_buffer()->get_selection_bounds(start, end);
    std::vector<std::pair<std::string, std::shared_ptr<JSON>>> results;
    write_request("textDocument/codeAction", to_string({make_range({start.get_line(), get_line_pos(start)}, {end.get_line(), get_line_pos(end)}), {"context", "{\"diagnostics\":[]}"}}), [&result_processed, &results](JSON &&result, bool error) {
      if(!error) {
        for(auto &code_action : result.array_or_empty()) {
          auto title = code_action.string_or("title", "");
          if(!title.empty())
            results.emplace_back(title, std::make_shared<JSON>(JSON::make_owner(std::move(code_action))));
        }
      }
      result_processed.set_value();
    });
    result_processed.get_future().get();

    if(results.empty()) {
      Info::get().print("No code actions found at cursor");
      return std::vector<FixIt>{};
    }

    SelectionDialog::create(this, true, true);
    for(auto &result : results)
      SelectionDialog::get()->add_row(result.first);
    SelectionDialog::get()->on_select = [this, results = std::move(results)](unsigned int index, const std::string &text, bool hide_window) {
      if(index >= results.size())
        return;

      auto edit = results[index].second->object_optional("edit");
      if(capabilities.code_action_resolve && !edit) {
        std::promise<void> result_processed;
        std::stringstream ss;
        bool first = true;
        for(auto &child : results[index].second->children_or_empty()) {
          ss << (!first ? ",\"" : "\"") << JSON::escape_string(child.first) << "\":" << child.second;
          first = false;
        }
        write_request("codeAction/resolve", ss.str(), [&result_processed, &edit](JSON &&result, bool error) {
          if(!error) {
            auto object = result.object_optional("edit");
            if(object)
              edit = JSON::make_owner(std::move(*object));
          }
          result_processed.set_value();
        });
        result_processed.get_future().get();
      }

      if(edit) {
        LanguageProtocol::WorkspaceEdit workspace_edit;
        try {
          workspace_edit = LanguageProtocol::WorkspaceEdit(*edit, file_path);
        }
        catch(...) {
          return;
        }

        auto current_view = Notebook::get().get_current_view();

        for(auto &document_change : workspace_edit.document_changes) {
          if(auto edit = boost::get<LanguageProtocol::TextDocumentEdit>(&document_change)) {
            Source::View *view = nullptr;
            for(auto &e : views) {
              if(e->file_path == edit->file) {
                view = e;
                break;
              }
            }
            if(!view) {
              if(!Notebook::get().open(edit->file))
                return;
              view = Notebook::get().get_current_view();
            }

            auto buffer = view->get_buffer();
            buffer->begin_user_action();

            auto end_iter = buffer->end();
            // If entire buffer is replaced
            if(edit->text_edits.size() == 1 &&
               edit->text_edits[0].range.start.line == 0 && edit->text_edits[0].range.start.character == 0 &&
               (edit->text_edits[0].range.end.line > end_iter.get_line() ||
                (edit->text_edits[0].range.end.line == end_iter.get_line() && edit->text_edits[0].range.end.character >= get_line_pos(end_iter)))) {
              view->replace_text(edit->text_edits[0].new_text);
            }
            else {
              for(auto text_edit_it = edit->text_edits.rbegin(); text_edit_it != edit->text_edits.rend(); ++text_edit_it) {
                auto start_iter = view->get_iter_at_line_pos(text_edit_it->range.start.line, text_edit_it->range.start.character);
                auto end_iter = view->get_iter_at_line_pos(text_edit_it->range.end.line, text_edit_it->range.end.character);
                if(view != current_view)
                  view->get_buffer()->place_cursor(start_iter);
                buffer->erase(start_iter, end_iter);
                start_iter = view->get_iter_at_line_pos(text_edit_it->range.start.line, text_edit_it->range.start.character);
                buffer->insert(start_iter, text_edit_it->new_text);
              }
            }

            buffer->end_user_action();
          }
        }

        if(current_view)
          Notebook::get().open(current_view);
      }

      if(capabilities.execute_command) {
        auto command = results[index].second->object_optional("command");
        if(command) {
          std::stringstream ss;
          bool first = true;
          for(auto &child : command->children_or_empty()) {
            ss << (!first ? ",\"" : "\"") << JSON::escape_string(child.first) << "\":" << child.second;
            first = false;
          }
          write_request("workspace/executeCommand", ss.str(), [](JSON &&result, bool error) {
          });
        }
      }
    };
    hide_tooltips();
    SelectionDialog::get()->show();

    return std::vector<FixIt>{};
  };
}

void Source::LanguageProtocolView::setup_signals() {
  if(capabilities.text_document_sync == LanguageProtocol::Capabilities::TextDocumentSync::incremental) {
    get_buffer()->signal_insert().connect(
        [this](const Gtk::TextIter &start, const Glib::ustring &text, int bytes) {
          std::pair<int, int> location = {start.get_line(), get_line_pos(start)};
          write_did_change_notification({{"contentChanges", "[{" + to_string({make_range(location, location), {"text", '"' + JSON::escape_string(text.raw()) + '"'}}) + "}]"}});
        },
        false);

    get_buffer()->signal_erase().connect(
        [this](const Gtk::TextIter &start, const Gtk::TextIter &end) {
          write_did_change_notification({{"contentChanges", "[{" + to_string({make_range({start.get_line(), get_line_pos(start)}, {end.get_line(), get_line_pos(end)}), {"text", "\"\""}}) + "}]"}});
        },
        false);
  }
  else if(capabilities.text_document_sync == LanguageProtocol::Capabilities::TextDocumentSync::full) {
    get_buffer()->signal_changed().connect([this]() {
      write_did_change_notification({{"contentChanges", "[{" + to_string({"text", '"' + JSON::escape_string(get_buffer()->get_text().raw()) + '"'}) + "}]"}});
    });
  }
}

void Source::LanguageProtocolView::setup_autocomplete() {
  autocomplete = std::make_unique<Autocomplete>(this, interactive_completion, last_keyval, false, true);

  if(!capabilities.completion)
    return;

  non_interactive_completion = [this] {
    if(CompletionDialog::get() && CompletionDialog::get()->is_visible())
      return;
    autocomplete->run();
  };

  autocomplete->reparse = [this] {
    autocomplete_rows.clear();
  };

  if(capabilities.signature_help) {
    // Activate argument completions
    get_buffer()->signal_changed().connect(
        [this] {
          autocomplete_possibly_no_arguments = false;
          if(!interactive_completion)
            return;
          if(CompletionDialog::get() && CompletionDialog::get()->is_visible())
            return;
          if(!has_focus())
            return;
          if(autocomplete_show_arguments)
            autocomplete->stop();
          autocomplete_show_arguments = false;
          autocomplete_delayed_show_arguments_connection.disconnect();
          autocomplete_delayed_show_arguments_connection = Glib::signal_timeout().connect(
              [this]() {
                if(get_buffer()->get_has_selection())
                  return false;
                if(CompletionDialog::get() && CompletionDialog::get()->is_visible())
                  return false;
                if(!has_focus())
                  return false;
                if(is_possible_argument()) {
                  autocomplete->stop();
                  autocomplete->run();
                }
                return false;
              },
              500);
        },
        false);

    // Remove argument completions
    signal_key_press_event().connect(
        [this](GdkEventKey *event) {
          if(autocomplete_show_arguments && CompletionDialog::get() && CompletionDialog::get()->is_visible() &&
             event->keyval != GDK_KEY_Down && event->keyval != GDK_KEY_Up &&
             event->keyval != GDK_KEY_Return && event->keyval != GDK_KEY_KP_Enter &&
             event->keyval != GDK_KEY_ISO_Left_Tab && event->keyval != GDK_KEY_Tab &&
             (event->keyval < GDK_KEY_Shift_L || event->keyval > GDK_KEY_Hyper_R)) {
            get_buffer()->erase(CompletionDialog::get()->start_mark->get_iter(), get_buffer()->get_insert()->get_iter());
            CompletionDialog::get()->hide();
          }
          return false;
        },
        false);
  }

  autocomplete->is_restart_key = [this](guint keyval) {
    auto iter = get_buffer()->get_insert()->get_iter();
    iter.backward_chars(2);
    if(keyval == '.' || (keyval == ':' && *iter == ':'))
      return true;
    return false;
  };

  std::function<bool(Gtk::TextIter)> is_possible_xml_attribute = [](Gtk::TextIter) { return false; };
  if(is_js) {
    autocomplete->is_restart_key = [](guint keyval) {
      if(keyval == '.' || keyval == ' ')
        return true;
      return false;
    };

    is_possible_xml_attribute = [this](Gtk::TextIter iter) {
      return (*iter == ' ' || iter.ends_line() || *iter == '/' || (*iter == '>' && iter.backward_char())) && find_open_symbol_backward(iter, iter, '<', '>');
    };
  }

  autocomplete->run_check = [this, is_possible_xml_attribute]() {
    auto prefix_start = get_buffer()->get_insert()->get_iter();
    auto prefix_end = prefix_start;

    auto prev = prefix_start;
    prev.backward_char();
    if(!is_code_iter(prev))
      return false;

    size_t count = 0;
    while(prefix_start.backward_char() && is_token_char(*prefix_start))
      ++count;

    autocomplete_enable_snippets = false;
    autocomplete_show_arguments = false;

    if(prefix_start != prefix_end && !is_token_char(*prefix_start))
      prefix_start.forward_char();

    prev = prefix_start;
    prev.backward_char();
    auto prevprev = prev;
    if(*prev == '.') {
      auto iter = prev;
      bool starts_with_num = false;
      size_t count = 0;
      while(iter.backward_char() && is_token_char(*iter)) {
        ++count;
        starts_with_num = Glib::Unicode::isdigit(*iter);
      }
      if((count >= 1 || *iter == ')' || *iter == ']' || *iter == '"' || *iter == '\'' || *iter == '?') && !starts_with_num) {
        {
          LockGuard lock(autocomplete->prefix_mutex);
          autocomplete->prefix = get_buffer()->get_text(prefix_start, prefix_end);
        }
        return true;
      }
    }
    else if((prevprev.backward_char() && *prevprev == ':' && *prev == ':')) {
      {
        LockGuard lock(autocomplete->prefix_mutex);
        autocomplete->prefix = get_buffer()->get_text(prefix_start, prefix_end);
      }
      return true;
    }
    else if(count >= 3) { // part of symbol
      {
        LockGuard lock(autocomplete->prefix_mutex);
        autocomplete->prefix = get_buffer()->get_text(prefix_start, prefix_end);
      }
      autocomplete_enable_snippets = true;
      return true;
    }
    if(is_possible_argument()) {
      autocomplete_show_arguments = true;
      LockGuard lock(autocomplete->prefix_mutex);
      autocomplete->prefix = "";
      return true;
    }
    if(is_possible_xml_attribute(prefix_start)) {
      LockGuard lock(autocomplete->prefix_mutex);
      autocomplete->prefix = "";
      return true;
    }
    if(!interactive_completion) {
      {
        LockGuard lock(autocomplete->prefix_mutex);
        autocomplete->prefix = get_buffer()->get_text(prefix_start, prefix_end);
      }
      auto prevprev = prev;
      autocomplete_enable_snippets = !(*prev == '.' || (prevprev.backward_char() && *prevprev == ':' && *prev == ':'));
      return true;
    }

    return false;
  };

  autocomplete->before_add_rows = [this] {
    status_state = "autocomplete...";
    if(update_status_state)
      update_status_state(this);
  };

  autocomplete->after_add_rows = [this] {
    status_state = "";
    if(update_status_state)
      update_status_state(this);
  };

  autocomplete->add_rows = [this](std::string &buffer, int line, int line_index) {
    if(autocomplete->state == Autocomplete::State::starting) {
      autocomplete_rows.clear();
      std::promise<void> result_processed;
      if(autocomplete_show_arguments) {
        if(!capabilities.signature_help)
          return true;
        dispatcher.post([this, line, line_index, &result_processed] {
          // Find current parameter number and previously used named parameters
          unsigned current_parameter_position = 0;
          auto named_parameter_symbol = get_named_parameter_symbol();
          std::set<std::string> used_named_parameters;
          auto iter = get_buffer()->get_insert()->get_iter();
          int para_count = 0;
          int square_count = 0;
          int angle_count = 0;
          int curly_count = 0;
          while(iter.backward_char() && backward_to_code(iter)) {
            if(para_count == 0 && square_count == 0 && angle_count == 0 && curly_count == 0) {
              if(named_parameter_symbol && (*iter == ',' || *iter == '(')) {
                auto next = iter;
                if(next.forward_char() && forward_to_code(next)) {
                  auto pair = get_token_iters(next);
                  if(pair.first != pair.second) {
                    auto symbol = pair.second;
                    if(forward_to_code(symbol) && *symbol == static_cast<unsigned char>(*named_parameter_symbol))
                      used_named_parameters.emplace(get_buffer()->get_text(pair.first, pair.second));
                  }
                }
              }
              if(*iter == ',')
                ++current_parameter_position;
              else if(*iter == '(')
                break;
            }
            if(*iter == '(')
              ++para_count;
            else if(*iter == ')')
              --para_count;
            else if(*iter == '[')
              ++square_count;
            else if(*iter == ']')
              --square_count;
            else if(*iter == '<')
              ++angle_count;
            else if(*iter == '>')
              --angle_count;
            else if(*iter == '{')
              ++curly_count;
            else if(*iter == '}')
              --curly_count;
          }
          bool using_named_parameters = named_parameter_symbol && !(current_parameter_position > 0 && used_named_parameters.empty());

          write_request("textDocument/signatureHelp", to_string({make_position(line, get_line_pos(line, line_index))}), [this, &result_processed, current_parameter_position, using_named_parameters, used_named_parameters = std::move(used_named_parameters)](JSON &&result, bool error) {
            if(!error) {
              for(auto &signature : result.array_or_empty("signatures")) {
                unsigned parameter_position = 0;
                for(auto &parameter : signature.array_or_empty("parameters")) {
                  if(parameter_position == current_parameter_position || using_named_parameters) {
                    auto label = parameter.string_or("label", "");
                    auto insert = label;
                    if(!using_named_parameters || used_named_parameters.find(get_token(insert)) == used_named_parameters.end()) {
                      autocomplete->rows.emplace_back(std::move(label));
                      autocomplete_rows.emplace_back(AutocompleteRow{std::move(insert), {}, LanguageProtocol::Documentation(parameter.child_optional("documentation")), {}, {}});
                    }
                  }
                  parameter_position++;
                }
              }
              if(autocomplete_rows.empty()) {
                dispatcher.post([this] {
                  // Move cursor forward if no arguments in completed function and if cursor is still inside ()
                  if(autocomplete_possibly_no_arguments) {
                    auto iter = get_buffer()->get_insert()->get_iter();
                    auto prev = iter;
                    if(prev.backward_char() && *prev == '(' && *iter == ')' && iter.forward_char())
                      get_buffer()->place_cursor(iter);
                  }
                });
              }
            }
            result_processed.set_value();
          });
        });
      }
      else {
        dispatcher.post([this, line, line_index, &result_processed] {
          write_request("textDocument/completion", to_string({make_position(line, get_line_pos(line, line_index))}), [this, &result_processed](JSON &&result, bool error) {
            if(!error) {
              bool is_incomplete = result.boolean_or("isIncomplete", false);
              auto items = result.array_or_empty();
              if(items.empty())
                items = result.array_or_empty("items");
              std::string prefix;
              {
                LockGuard lock(autocomplete->prefix_mutex);
                prefix = autocomplete->prefix;
              }
              for(auto &item : items) {
                auto label = item.string_or("label", "");
                if(starts_with(label, prefix)) {
                  auto detail = item.string_or("detail", "");
                  LanguageProtocol::Documentation documentation(item.child_optional("documentation"));

                  std::vector<LanguageProtocol::TextEdit> additional_text_edits;
                  try {
                    for(auto &text_edit : item.array_or_empty("additionalTextEdits"))
                      additional_text_edits.emplace_back(text_edit);
                  }
                  catch(...) {
                    additional_text_edits.clear();
                  }

                  auto insert = item.string_or("insertText", "");
                  if(insert.empty()) {
                    if(auto text_edit = item.object_optional("textEdit"))
                      insert = text_edit->string_or("newText", "");
                  }
                  if(insert.empty())
                    insert = label;
                  if(!insert.empty()) {
                    auto kind = item.integer_or("kind", 0);
                    if(kind >= 2 && kind <= 4 && insert.find('(') == std::string::npos) // If kind is method, function or constructor, but parentheses are missing
                      insert += "(${1:})";

                    std::shared_ptr<JSON> item_object;
                    if(detail.empty() && documentation.value.empty() && (is_incomplete || is_js || (language_id == "python" && !client->pyright))) // Workaround for typescript-language-server (is_js) and python-lsp-server
                      item_object = std::make_shared<JSON>(JSON::make_owner(std::move(item)));

                    autocomplete->rows.emplace_back(std::move(label));
                    autocomplete_rows.emplace_back(AutocompleteRow{std::move(insert), std::move(detail), std::move(documentation), std::move(item_object), std::move(additional_text_edits)});
                  }
                }
              }

              if(autocomplete_enable_snippets) {
                LockGuard lock(snippets_mutex);
                if(snippets) {
                  for(auto &snippet : *snippets) {
                    if(starts_with(snippet.prefix, prefix)) {
                      autocomplete->rows.emplace_back(snippet.prefix);
                      autocomplete_rows.emplace_back(AutocompleteRow{snippet.body, {}, LanguageProtocol::Documentation(snippet.description), {}, {}});
                    }
                  }
                }
              }
            }
            result_processed.set_value();
          });
        });
      }
      result_processed.get_future().get();
    }
    return true;
  };

  autocomplete->on_show = [this] {
    hide_tooltips();
  };

  autocomplete->on_hide = [this] {
    autocomplete_rows.clear();
    ++set_tooltip_count;
  };

  autocomplete->on_select = [this](unsigned int index, const std::string &text, bool hide_window) {
    auto insert = hide_window ? autocomplete_rows[index].insert : text;

    get_buffer()->erase(CompletionDialog::get()->start_mark->get_iter(), get_buffer()->get_insert()->get_iter());

    // Do not insert function/template parameters if they already exist
    {
      auto iter = get_buffer()->get_insert()->get_iter();
      if(*iter == '(' || *iter == '<') {
        auto bracket_pos = insert.find(*iter);
        if(bracket_pos != std::string::npos)
          insert.erase(bracket_pos);
      }
    }

    // Do not instert ?. after ., instead replace . with ?.
    if(1 < insert.size() && insert[0] == '?' && insert[1] == '.') {
      auto iter = get_buffer()->get_insert()->get_iter();
      auto prev = iter;
      if(prev.backward_char() && *prev == '.') {
        get_buffer()->erase(prev, iter);
      }
    }

    if(hide_window) {
      if(autocomplete_show_arguments) {
        if(auto symbol = get_named_parameter_symbol()) { // Do not select named parameters in for instance Python
          auto named_parameter = get_token(insert);
          if(!named_parameter.empty()) {
            get_buffer()->insert(CompletionDialog::get()->start_mark->get_iter(), named_parameter + *symbol);
            return;
          }
        }
        get_buffer()->insert(CompletionDialog::get()->start_mark->get_iter(), insert);
        int start_offset = CompletionDialog::get()->start_mark->get_iter().get_offset();
        int end_offset = CompletionDialog::get()->start_mark->get_iter().get_offset() + insert.size();
        get_buffer()->select_range(get_buffer()->get_iter_at_offset(start_offset), get_buffer()->get_iter_at_offset(end_offset));
        return;
      }

      auto additional_text_edits = std::move(autocomplete_rows[index].additional_text_edits); // autocomplete_rows will be cleared after insert_snippet (see autocomplete->on_hide)

      get_buffer()->begin_user_action();
      insert_snippet(CompletionDialog::get()->start_mark->get_iter(), insert);
      for(auto it = additional_text_edits.rbegin(); it != additional_text_edits.rend(); ++it) {
        auto start = get_iter_at_line_pos(it->range.start.line, it->range.start.character);
        auto end = get_iter_at_line_pos(it->range.end.line, it->range.end.character);
        get_buffer()->erase(start, end);
        start = get_iter_at_line_pos(it->range.start.line, it->range.start.character);
        get_buffer()->insert(start, it->new_text);
      }
      get_buffer()->end_user_action();

      auto iter = get_buffer()->get_insert()->get_iter();
      if(*iter == ')' && iter.backward_char() && *iter == '(') { // If no arguments, try signatureHelp
        last_keyval = '(';
        if(is_js) // Workaround for typescript-language-server
          autocomplete_possibly_no_arguments = true;
        autocomplete->run();
      }
    }
    else
      get_buffer()->insert(CompletionDialog::get()->start_mark->get_iter(), insert);
  };

  autocomplete->set_tooltip_buffer = [this](unsigned int index) -> std::function<void(Tooltip & tooltip)> {
    size_t last_count = ++set_tooltip_count;
    auto &autocomplete_row = autocomplete_rows[index];

    const static auto insert_documentation = [](Source::LanguageProtocolView *view, Tooltip &tooltip, const std::string &detail, const LanguageProtocol::Documentation &documentation) {
      if(view->language_id == "python" && !view->client->pyright) // pylsp might support markdown in the future
        tooltip.insert_docstring(documentation.value);
      else {
        if(!detail.empty()) {
          tooltip.insert_code(detail, view->language);
          tooltip.remove_trailing_newlines();
        }
        if(!documentation.value.empty()) {
          if(tooltip.buffer->size() > 0)
            tooltip.buffer->insert_at_cursor("\n\n");
          if(documentation.kind == "plaintext" || documentation.kind.empty())
            tooltip.insert_with_links_tagged(documentation.value);
          else if(documentation.kind == "markdown")
            tooltip.insert_markdown(documentation.value);
          else
            tooltip.insert_code(documentation.value, documentation.kind);
        }
      }
    };

    if(capabilities.completion_resolve && autocomplete_row.detail.empty() && autocomplete_row.documentation.value.empty() && autocomplete_row.item_object) {
      // Prevent several completionItem/resolve requests when no completion item is found from written source (for instance when autocompleting in python : math.___).
      // This seems to be a bug in Gtk::TreeView::signal_cursor_changed.
      resolve_completion_item_connection.disconnect();
      resolve_completion_item_connection = Glib::signal_timeout().connect(
          [this, last_count, item_object = autocomplete_row.item_object] {
            if(last_count != set_tooltip_count)
              return false;
            std::stringstream ss;
            bool first = true;
            for(auto &child : item_object->children_or_empty()) {
              ss << (!first ? ",\"" : "\"") << JSON::escape_string(child.first) << "\":" << child.second;
              first = false;
            }
            write_request("completionItem/resolve", ss.str(), [this, last_count](JSON &&result, bool error) {
              if(!error) {
                if(last_count != set_tooltip_count)
                  return;
                auto detail = result.string_or("detail", "");
                auto documentation = LanguageProtocol::Documentation(result.child_optional("documentation"));
                if(detail.empty() && documentation.value.empty())
                  return;
                dispatcher.post([this, last_count, detail = std::move(detail), documentation = std::move(documentation)] {
                  if(last_count != set_tooltip_count)
                    return;
                  autocomplete->tooltips.clear();
                  auto iter = CompletionDialog::get()->start_mark->get_iter();
                  autocomplete->tooltips.emplace_back(this, iter, iter, [this, detail = std::move(detail), documentation = std::move(documentation)](Tooltip &tooltip) {
                    insert_documentation(this, tooltip, detail, documentation);
                  });
                  autocomplete->tooltips.show(true);
                });
              }
            });
            return false;
          },
          0);
      return nullptr;
    }
    if(autocomplete_row.detail.empty() && autocomplete_row.documentation.value.empty())
      return nullptr;

    return [this, &autocomplete_row](Tooltip &tooltip) mutable {
      insert_documentation(this, tooltip, autocomplete_row.detail, autocomplete_row.documentation);
    };
  };

  autocomplete->clear_tooltips = [this] {
    resolve_completion_item_connection.disconnect();
    ++set_tooltip_count;
    autocomplete->tooltips.hide();
  };
}

void Source::LanguageProtocolView::update_diagnostics_async(std::vector<LanguageProtocol::Diagnostic> &&diagnostics) {
  size_t last_count = ++update_diagnostics_async_count;
  if(capabilities.code_action && !diagnostics.empty()) {
    dispatcher.post([this, diagnostics = std::move(diagnostics), last_count]() mutable {
      if(last_count != update_diagnostics_async_count)
        return;
      std::stringstream ss;
      bool first = true;
      for(auto &diagnostic : diagnostics) {
        if(!first)
          ss << ',';
        ss << *diagnostic.object;
        first = false;
      }
      std::pair<std::string, std::string> range;
      if(diagnostics.size() == 1) // Use diagnostic range if only one diagnostic, otherwise use whole buffer
        range = make_range({diagnostics[0].range.start.line, diagnostics[0].range.start.character}, {diagnostics[0].range.end.line, diagnostics[0].range.end.character});
      else {
        auto start = get_buffer()->begin();
        auto end = get_buffer()->end();
        range = make_range({start.get_line(), get_line_pos(start)}, {end.get_line(), get_line_pos(end)});
      }
      std::vector<std::pair<std::string, std::string>> params = {range, {"context", '{' + to_string({{"diagnostics", '[' + ss.str() + ']'}, {"only", "[\"quickfix\"]"}}) + '}'}};
      thread_pool.push([this, diagnostics = std::move(diagnostics), params = std::move(params), last_count]() mutable {
        if(last_count != update_diagnostics_async_count)
          return;
        std::promise<void> result_processed;
        write_request("textDocument/codeAction", to_string(params), [this, &result_processed, &diagnostics, last_count](JSON &&result, bool error) {
          if(!error && last_count == update_diagnostics_async_count) {
            try {
              for(auto &code_action : result.array_or_empty()) {
                auto kind = code_action.string_or("kind", "");
                if(kind == "quickfix" || kind.empty()) { // Workaround for typescript-language-server (kind.empty())
                  auto title = code_action.string("title");
                  std::vector<LanguageProtocol::Diagnostic> quickfix_diagnostics;
                  for(auto &diagnostic : code_action.array_or_empty("diagnostics"))
                    quickfix_diagnostics.emplace_back(std::move(diagnostic));
                  auto edit = code_action.object_optional("edit");
                  if(!edit) {
                    if(auto arguments = code_action.array_optional("arguments")) {
                      if(!arguments->empty())
                        edit = std::move((*arguments)[0]);
                    }
                  }
                  if(edit) {
                    LanguageProtocol::WorkspaceEdit workspace_edit(*edit, file_path);
                    for(auto &document_change : workspace_edit.document_changes) {
                      if(auto edit = boost::get<LanguageProtocol::TextDocumentEdit>(&document_change)) {
                        for(auto &text_edit : edit->text_edits) {
                          if(!quickfix_diagnostics.empty()) {
                            for(auto &diagnostic : diagnostics) {
                              for(auto &quickfix_diagnostic : quickfix_diagnostics) {
                                if(diagnostic.message == quickfix_diagnostic.message && diagnostic.range == quickfix_diagnostic.range) {
                                  auto pair = diagnostic.quickfixes.emplace(title, std::set<Source::FixIt>{});
                                  pair.first->second.emplace(
                                      text_edit.new_text,
                                      edit->file,
                                      std::make_pair<Offset, Offset>(Offset(text_edit.range.start.line, text_edit.range.start.character),
                                                                     Offset(text_edit.range.end.line, text_edit.range.end.character)));
                                  break;
                                }
                              }
                            }
                          }
                          else { // Workaround for language server that does not report quickfix diagnostics
                            for(auto &diagnostic : diagnostics) {
                              if(text_edit.range.start.line == diagnostic.range.start.line) {
                                auto pair = diagnostic.quickfixes.emplace(title, std::set<Source::FixIt>{});
                                pair.first->second.emplace(
                                    text_edit.new_text,
                                    edit->file,
                                    std::make_pair<Offset, Offset>(Offset(text_edit.range.start.line, text_edit.range.start.character),
                                                                   Offset(text_edit.range.end.line, text_edit.range.end.character)));
                                break;
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            catch(...) {
            }
          }
          result_processed.set_value();
        });
        result_processed.get_future().get();
        dispatcher.post([this, diagnostics = std::move(diagnostics), last_count]() mutable {
          if(last_count == update_diagnostics_async_count) {
            if(capabilities.type_coverage)
              last_diagnostics = diagnostics;
            update_diagnostics(std::move(diagnostics));
          }
        });
      });
    });
  }
  else {
    dispatcher.post([this, diagnostics = std::move(diagnostics), last_count]() mutable {
      if(last_count == update_diagnostics_async_count) {
        if(capabilities.type_coverage)
          last_diagnostics = diagnostics;
        update_diagnostics(std::move(diagnostics));
      }
    });
  }
}

void Source::LanguageProtocolView::update_diagnostics(std::vector<LanguageProtocol::Diagnostic> diagnostics) {
  diagnostic_offsets.clear();
  diagnostic_tooltips.clear();
  fix_its.clear();
  get_buffer()->remove_tag_by_name("def:warning_underline", get_buffer()->begin(), get_buffer()->end());
  get_buffer()->remove_tag_by_name("def:error_underline", get_buffer()->begin(), get_buffer()->end());
  num_warnings = 0;
  num_errors = 0;
  num_fix_its = 0;

  for(auto &diagnostic : diagnostics) {
    auto start = get_iter_at_line_pos(diagnostic.range.start.line, diagnostic.range.start.character);
    auto end = get_iter_at_line_pos(diagnostic.range.end.line, diagnostic.range.end.character);

    if(start == end) {
      if(!end.ends_line())
        end.forward_char();
      else
        while(start.ends_line() && start.backward_char()) { // Move start so that diagnostic underline is visible
        }
    }

    bool error = false;
    if(diagnostic.severity >= 2)
      num_warnings++;
    else {
      num_errors++;
      error = true;
    }
    num_fix_its += diagnostic.quickfixes.size();

    for(auto &quickfix : diagnostic.quickfixes)
      fix_its.insert(fix_its.end(), quickfix.second.begin(), quickfix.second.end());

    add_diagnostic_tooltip(start, end, error, [this, diagnostic = std::move(diagnostic)](Tooltip &tooltip) {
      if(language_id == "python" && !client->pyright) { // pylsp might support markdown in the future
        tooltip.insert_with_links_tagged(diagnostic.message);
        return;
      }
      tooltip.insert_markdown(diagnostic.message);

      if(!diagnostic.related_informations.empty()) {
        auto link_tag = tooltip.buffer->get_tag_table()->lookup("link");
        for(size_t i = 0; i < diagnostic.related_informations.size(); ++i) {
          auto link = filesystem::get_relative_path(diagnostic.related_informations[i].location.file, file_path.parent_path()).string();
          link += ':' + std::to_string(diagnostic.related_informations[i].location.range.start.line + 1);
          link += ':' + std::to_string(diagnostic.related_informations[i].location.range.start.character + 1);

          if(i == 0)
            tooltip.buffer->insert_at_cursor("\n\n");
          else
            tooltip.buffer->insert_at_cursor("\n");
          tooltip.insert_markdown(diagnostic.related_informations[i].message);
          tooltip.buffer->insert_at_cursor(": ");
          tooltip.buffer->insert_with_tag(tooltip.buffer->get_insert()->get_iter(), link, link_tag);
        }
      }

      if(!diagnostic.quickfixes.empty()) {
        if(diagnostic.quickfixes.size() == 1)
          tooltip.buffer->insert_at_cursor("\n\nFix-it:");
        else
          tooltip.buffer->insert_at_cursor("\n\nFix-its:");
        for(auto &quickfix : diagnostic.quickfixes) {
          tooltip.buffer->insert_at_cursor("\n");
          tooltip.insert_markdown(quickfix.first);
        }
      }
    });
  }

  for(auto &mark : type_coverage_marks) {
    add_diagnostic_tooltip(mark.first->get_iter(), mark.second->get_iter(), false, [](Tooltip &tooltip) {
      tooltip.buffer->insert_at_cursor(type_coverage_message);
    });
    num_warnings++;
  }

  status_diagnostics = std::make_tuple(num_warnings, num_errors, num_fix_its);
  if(update_status_diagnostics)
    update_status_diagnostics(this);
}

void Source::LanguageProtocolView::show_type_tooltips(const Gdk::Rectangle &rectangle) {
  if(!capabilities.hover)
    return;

  Gtk::TextIter iter;
  int location_x, location_y;
  window_to_buffer_coords(Gtk::TextWindowType::TEXT_WINDOW_TEXT, rectangle.get_x(), rectangle.get_y(), location_x, location_y);
  location_x += (rectangle.get_width() - 1) / 2;
  get_iter_at_location(iter, location_x, location_y);
  Gdk::Rectangle iter_rectangle;
  get_iter_location(iter, iter_rectangle);
  if(iter.ends_line() && location_x > iter_rectangle.get_x())
    return;

  auto offset = iter.get_offset();

  static int request_count = 0;
  request_count++;
  auto current_request = request_count;
  write_request("textDocument/hover", to_string({make_position(iter.get_line(), get_line_pos(iter))}), [this, offset, current_request](JSON &&result, bool error) {
    if(!error) {
      // hover result structure vary significantly from the different language servers
      struct Content {
        std::string value;
        std::string kind;
      };
      std::list<Content> contents;
      auto contents_pt = result.child_optional("contents");
      if(!contents_pt)
        return;
      if(auto string = contents_pt->string_optional())
        contents.emplace_back(Content{std::move(*string), "markdown"});
      else if(auto object = contents_pt->object_optional()) {
        auto value = object->string_or("value", "");
        if(!value.empty()) {
          auto kind = object->string_or("kind", "");
          if(kind.empty())
            kind = object->string_or("language", "");
          contents.emplace_front(Content{std::move(value), std::move(kind)});
        }
      }
      else if(auto array = contents_pt->array_optional()) {
        bool first_value_in_object = true;
        for(auto &object_or_string : *array) {
          if(auto object = object_or_string.object_optional()) {
            auto value = object_or_string.string_or("value", "");
            if(!value.empty()) {
              auto kind = object_or_string.string_or("kind", "");
              if(kind.empty())
                kind = object_or_string.string_or("language", "");
              if(first_value_in_object) // Place first value, which most likely is type information, to front (workaround for flow-bin's language server)
                contents.emplace_front(Content{std::move(value), std::move(kind)});
              else
                contents.emplace_back(Content{std::move(value), std::move(kind)});
              first_value_in_object = false;
            }
          }
          else if(auto string = object_or_string.string_optional()) {
            if(!string->empty())
              contents.emplace_back(Content{std::move(*string), "markdown"});
          }
        }
      }
      if(!contents.empty()) {
        dispatcher.post([this, offset, contents = std::move(contents), current_request]() mutable {
          if(current_request != request_count)
            return;
          if(Notebook::get().get_current_view() != this)
            return;
          if(offset >= get_buffer()->get_char_count())
            return;
          type_tooltips.clear();

          auto token_iters = get_token_iters(get_buffer()->get_iter_at_offset(offset));
          type_tooltips.emplace_back(this, token_iters.first, token_iters.second, [this, offset, contents = std::move(contents)](Tooltip &tooltip) mutable {
            bool first = true;
            if(language_id == "python" && !client->pyright) { // pylsp might support markdown in the future
              for(auto &content : contents) {
                if(!first)
                  tooltip.buffer->insert_at_cursor("\n\n");
                first = false;
                if(content.kind == "python")
                  tooltip.insert_code(content.value, content.kind);
                else
                  tooltip.insert_docstring(content.value);
              }
            }
            else {
              for(auto &content : contents) {
                if(!first)
                  tooltip.buffer->insert_at_cursor("\n\n");
                first = false;
                if(content.kind == "plaintext" || content.kind.empty())
                  tooltip.insert_with_links_tagged(content.value);
                else if(content.kind == "markdown")
                  tooltip.insert_markdown(content.value);
                else
                  tooltip.insert_code(content.value, content.kind);
                tooltip.remove_trailing_newlines();
              }
            }

#ifdef JUCI_ENABLE_DEBUG
            if(language_id == "rust" && capabilities.definition) {
              if(Debug::LLDB::get().is_stopped()) {
                Glib::ustring value_type = "Value";

                auto token_iters = get_token_iters(get_buffer()->get_iter_at_offset(offset));

                auto variable = get_buffer()->get_text(token_iters.first, token_iters.second);
                Glib::ustring debug_value;
                auto offsets = get_declarations(token_iters.first);
                for(size_t i = 0; i < offsets.size() && debug_value.empty(); ++i)
                  debug_value = Debug::LLDB::get().get_value(variable, offsets[i].file_path, offsets[i].line + 1, offsets[i].index + 1);
                if(debug_value.empty()) {
                  debug_value = Debug::LLDB::get().get_return_value(file_path, token_iters.first.get_line() + 1, token_iters.first.get_line_index() + 1);
                  if(!debug_value.empty())
                    value_type = "Return value";
                }
                if(debug_value.empty()) {
                  auto end = token_iters.second;
                  while((end.ends_line() || *end == ' ' || *end == '\t') && end.forward_char()) {
                  }
                  if(*end != '(') {
                    auto iter = token_iters.first;
                    auto start = iter;
                    while(iter.backward_char()) {
                      if(*iter == '.') {
                        while(iter.backward_char() && (*iter == ' ' || *iter == '\t' || iter.ends_line())) {
                        }
                      }
                      if(!is_token_char(*iter))
                        break;
                      start = iter;
                    }
                    if(is_token_char(*start))
                      debug_value = Debug::LLDB::get().get_value(get_buffer()->get_text(start, token_iters.second));
                  }
                }
                if(!debug_value.empty()) {
                  size_t pos = debug_value.find(" = ");
                  if(pos != Glib::ustring::npos) {
                    Glib::ustring::iterator iter;
                    while(!debug_value.validate(iter)) {
                      auto next_char_iter = iter;
                      next_char_iter++;
                      debug_value.replace(iter, next_char_iter, "?");
                    }
                    tooltip.buffer->insert_at_cursor("\n\n" + value_type + ":\n");
                    tooltip.insert_code(debug_value.substr(pos + 3, debug_value.size() - (pos + 3) - 1));
                  }
                }
              }
            }
#endif
          });
          type_tooltips.show();
        });
      }
    }
  });
}

void Source::LanguageProtocolView::apply_similar_symbol_tag() {
  if(!capabilities.document_highlight)
    return;

  auto iter = get_buffer()->get_insert()->get_iter();
  static int request_count = 0;
  request_count++;
  auto current_request = request_count;
  write_request("textDocument/documentHighlight", to_string({make_position(iter.get_line(), get_line_pos(iter)), {"context", "{\"includeDeclaration\":true}"}}), [this, current_request](JSON &&result, bool error) {
    if(!error) {
      std::vector<LanguageProtocol::Range> ranges;
      for(auto &location : result.array_or_empty()) {
        try {
          ranges.emplace_back(location.object("range"));
        }
        catch(...) {
        }
      }
      dispatcher.post([this, ranges = std::move(ranges), current_request] {
        if(current_request != request_count || !similar_symbol_tag_applied)
          return;
        get_buffer()->remove_tag(similar_symbol_tag, get_buffer()->begin(), get_buffer()->end());
        for(auto &range : ranges) {
          auto start = get_iter_at_line_pos(range.start.line, range.start.character);
          auto end = get_iter_at_line_pos(range.end.line, range.end.character);
          get_buffer()->apply_tag(similar_symbol_tag, start, end);
        }
      });
    }
  });
}

void Source::LanguageProtocolView::apply_clickable_tag(const Gtk::TextIter &iter) {
  static int request_count = 0;
  request_count++;
  auto current_request = request_count;
  write_request("textDocument/definition", to_string({make_position(iter.get_line(), get_line_pos(iter))}), [this, current_request, line = iter.get_line(), line_offset = iter.get_line_offset()](JSON &&result, bool error) {
    if(!error) {
      if(result.array_optional() || result.object_optional()) {
        dispatcher.post([this, current_request, line, line_offset] {
          if(current_request != request_count || !clickable_tag_applied)
            return;
          get_buffer()->remove_tag(clickable_tag, get_buffer()->begin(), get_buffer()->end());
          auto range = get_token_iters(get_iter_at_line_offset(line, line_offset));
          get_buffer()->apply_tag(clickable_tag, range.first, range.second);
        });
      }
    }
  });
}

std::vector<Source::Offset> Source::LanguageProtocolView::get_declarations(const Gtk::TextIter &iter) {
  auto offsets = std::make_shared<std::vector<Offset>>();
  std::promise<void> result_processed;
  write_request("textDocument/definition", to_string({make_position(iter.get_line(), get_line_pos(iter))}), [offsets, &result_processed](JSON &&result, bool error) {
    if(!error) {
      auto locations = result.array_or_empty();
      if(locations.empty()) {
        if(auto object = result.object_optional())
          locations.emplace_back(std::move(*object));
      }
      for(auto &location_object : locations) {
        try {
          LanguageProtocol::Location location(location_object);
          offsets->emplace_back(location.range.start.line, location.range.start.character, std::move(location.file));
        }
        catch(...) {
        }
      }
    }
    result_processed.set_value();
  });
  result_processed.get_future().get();
  return *offsets;
}

std::vector<Source::Offset> Source::LanguageProtocolView::get_type_declarations(const Gtk::TextIter &iter) {
  auto offsets = std::make_shared<std::vector<Offset>>();
  std::promise<void> result_processed;
  write_request("textDocument/typeDefinition", to_string({make_position(iter.get_line(), get_line_pos(iter))}), [offsets, &result_processed](JSON &&result, bool error) {
    if(!error) {
      auto locations = result.array_or_empty();
      if(locations.empty()) {
        if(auto object = result.object_optional())
          locations.emplace_back(std::move(*object));
      }
      for(auto &location_object : locations) {
        try {
          LanguageProtocol::Location location(location_object);
          offsets->emplace_back(location.range.start.line, location.range.start.character, std::move(location.file));
        }
        catch(...) {
        }
      }
    }
    result_processed.set_value();
  });
  result_processed.get_future().get();
  return *offsets;
}

std::vector<Source::Offset> Source::LanguageProtocolView::get_implementations(const Gtk::TextIter &iter) {
  auto offsets = std::make_shared<std::vector<Offset>>();
  std::promise<void> result_processed;
  write_request("textDocument/implementation", to_string({make_position(iter.get_line(), get_line_pos(iter))}), [offsets, &result_processed](JSON &&result, bool error) {
    if(!error) {
      auto locations = result.array_or_empty();
      if(locations.empty()) {
        if(auto object = result.object_optional())
          locations.emplace_back(std::move(*object));
      }
      for(auto &location_object : locations) {
        try {
          LanguageProtocol::Location location(location_object);
          offsets->emplace_back(location.range.start.line, location.range.start.character, std::move(location.file));
        }
        catch(...) {
        }
      }
    }
    result_processed.set_value();
  });
  result_processed.get_future().get();
  return *offsets;
}

boost::optional<char> Source::LanguageProtocolView::get_named_parameter_symbol() {
  if(language_id == "python") // TODO: add more languages that supports named parameters
    return '=';
  return {};
}

void Source::LanguageProtocolView::update_type_coverage() {
  if(capabilities.type_coverage) {
    write_request("textDocument/typeCoverage", {}, [this](JSON &&result, bool error) {
      if(error) {
        if(update_type_coverage_retries > 0) { // Retry typeCoverage request, since these requests can fail while waiting for language server to start
          dispatcher.post([this] {
            update_type_coverage_connection.disconnect();
            update_type_coverage_connection = Glib::signal_timeout().connect(
                [this]() {
                  --update_type_coverage_retries;
                  update_type_coverage();
                  return false;
                },
                1000);
          });
        }
        return;
      }
      update_type_coverage_retries = 0;

      std::vector<LanguageProtocol::Range> ranges;
      for(auto &uncovered_range : result.array_or_empty("uncoveredRanges")) {
        try {
          ranges.emplace_back(uncovered_range.object("range"));
        }
        catch(...) {
        }
      }

      dispatcher.post([this, ranges = std::move(ranges)] {
        type_coverage_marks.clear();
        for(auto &range : ranges) {
          auto start = get_iter_at_line_pos(range.start.line, range.start.character);
          auto end = get_iter_at_line_pos(range.end.line, range.end.character);
          type_coverage_marks.emplace_back(start, end);
        }

        update_diagnostics(last_diagnostics);
      });
    });
  }
}
