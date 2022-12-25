#include "json.hpp"
#include <boost/filesystem.hpp>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

int main() {
#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY);
#endif

  auto tests_path = boost::filesystem::canonical(JUCI_TESTS_PATH);
  auto file_path = boost::filesystem::canonical(tests_path / "language_protocol_test_files" / "main.rs");

  std::string line;
  try {
    // Read initialize and respond
    {
      std::getline(std::cin, line);
      auto size = std::atoi(line.substr(16).c_str());
      std::getline(std::cin, line);
      std::string buffer;
      buffer.resize(size);
      std::cin.read(&buffer[0], size);
      std::stringstream ss(buffer);
      JSON object(ss);
      if(object.string("method") != "initialize")
        return 1;

      std::string result = R"({
  "id": 0,
  "jsonrpc": "2.0",
  "result": {
    "capabilities": {
      "callHierarchyProvider": true,
      "codeActionProvider": {
        "codeActionKinds": [
          "",
          "quickfix",
          "refactor",
          "refactor.extract",
          "refactor.inline",
          "refactor.rewrite"
        ],
        "resolveProvider": true
      },
      "codeLensProvider": {
        "resolveProvider": true
      },
      "completionProvider": {
        "triggerCharacters": [
          ":",
          ".",
          "'"
        ]
      },
      "definitionProvider": true,
      "documentFormattingProvider": true,
      "documentHighlightProvider": true,
      "documentOnTypeFormattingProvider": {
        "firstTriggerCharacter": "=",
        "moreTriggerCharacter": [
          ".",
          ">",
          "{"
        ]
      },
      "documentSymbolProvider": true,
      "experimental": {
        "joinLines": true,
        "onEnter": true,
        "parentModule": true,
        "runnables": {
          "kinds": [
            "cargo"
          ]
        },
        "ssr": true,
        "workspaceSymbolScopeKindFiltering": true
      },
      "foldingRangeProvider": true,
      "hoverProvider": true,
      "implementationProvider": true,
      "referencesProvider": true,
      "renameProvider": {
        "prepareProvider": true
      },
      "selectionRangeProvider": true,
      "semanticTokensProvider": {
        "full": {
          "delta": true
        },
        "legend": {
          "tokenModifiers": [
            "documentation",
            "declaration",
            "definition",
            "static",
            "abstract",
            "deprecated",
            "readonly",
            "constant",
            "controlFlow",
            "injected",
            "mutable",
            "consuming",
            "async",
            "unsafe",
            "attribute",
            "trait",
            "callable",
            "intraDocLink"
          ],
          "tokenTypes": [
            "comment",
            "keyword",
            "string",
            "number",
            "regexp",
            "operator",
            "namespace",
            "type",
            "struct",
            "class",
            "interface",
            "enum",
            "enumMember",
            "typeParameter",
            "function",
            "method",
            "property",
            "macro",
            "variable",
            "parameter",
            "angle",
            "arithmetic",
            "attribute",
            "bitwise",
            "boolean",
            "brace",
            "bracket",
            "builtinType",
            "characterLiteral",
            "colon",
            "comma",
            "comparison",
            "constParameter",
            "dot",
            "escapeSequence",
            "formatSpecifier",
            "generic",
            "label",
            "lifetime",
            "logical",
            "operator",
            "parenthesis",
            "punctuation",
            "selfKeyword",
            "semicolon",
            "typeAlias",
            "union",
            "unresolvedReference"
          ]
        },
        "range": true
      },
      "signatureHelpProvider": {
        "triggerCharacters": [
          "(",
          ","
        ]
      },
      "textDocumentSync": {
        "change": 2,
        "openClose": true,
        "save": {}
      },
      "typeDefinitionProvider": true,
      "workspace": {
        "fileOperations": {
          "willRename": {
            "filters": [
              {
                "pattern": {
                  "glob": "**/*.rs",
                  "matches": "file"
                },
                "scheme": "file"
              },
              {
                "pattern": {
                  "glob": "**",
                  "matches": "folder"
                },
                "scheme": "file"
              }
            ]
          }
        }
      },
      "workspaceSymbolProvider": true
    },
    "serverInfo": {
      "name": "rust-analyzer",
      "version": "3022a2c3a 2021-05-25 dev"
    }
  }
}
)";
      std::cout << "Content-Length: " << result.size() << "\r\n\r\n"
                << result;
    }

    // Read initialized
    {
      std::getline(std::cin, line);
      auto size = std::atoi(line.substr(16).c_str());
      std::getline(std::cin, line);
      std::string buffer;
      buffer.resize(size);
      std::cin.read(&buffer[0], size);
      std::stringstream ss(buffer);
      JSON object(ss);
      if(object.string("method") != "initialized")
        return 1;
    }

    // Read textDocument/didOpen
    {
      std::getline(std::cin, line);
      auto size = std::atoi(line.substr(16).c_str());
      std::getline(std::cin, line);
      std::string buffer;
      buffer.resize(size);
      std::cin.read(&buffer[0], size);
      std::stringstream ss(buffer);
      JSON object(ss);
      if(object.string("method") != "textDocument/didOpen")
        return 1;
    }

    // Read textDocument/didChange
    {
      std::getline(std::cin, line);
      auto size = std::atoi(line.substr(16).c_str());
      std::getline(std::cin, line);
      std::string buffer;
      buffer.resize(size);
      std::cin.read(&buffer[0], size);
      std::stringstream ss(buffer);
      JSON object(ss);
      if(object.string("method") != "textDocument/didChange")
        return 1;
    }

    // Read and write textDocument/formatting
    {
      std::getline(std::cin, line);
      auto size = std::atoi(line.substr(16).c_str());
      std::getline(std::cin, line);
      std::string buffer;
      buffer.resize(size);
      std::cin.read(&buffer[0], size);
      std::stringstream ss(buffer);
      JSON object(ss);
      if(object.string("method") != "textDocument/formatting")
        return 1;

      std::string result = R"({
  "jsonrpc": "2.0",
  "id": 1,
  "result": [
    {
      "range": {
        "start": {
          "line": 0,
          "character": 0
        },
        "end": {
          "line": 0,
          "character": 1
        }
      },
      "newText": ""
    }
  ]
}
)";
      std::cout << "Content-Length: " << result.size() << "\r\n\r\n"
                << result;
    }

    // Read textDocument/didChange
    {
      std::getline(std::cin, line);
      auto size = std::atoi(line.substr(16).c_str());
      std::getline(std::cin, line);
      std::string buffer;
      buffer.resize(size);
      std::cin.read(&buffer[0], size);
      std::stringstream ss(buffer);
      JSON object(ss);
      if(object.string("method") != "textDocument/didChange")
        return 1;
    }

    // Read and write textDocument/definition
    {
      std::getline(std::cin, line);
      auto size = std::atoi(line.substr(16).c_str());
      std::getline(std::cin, line);
      std::string buffer;
      buffer.resize(size);
      std::cin.read(&buffer[0], size);
      std::stringstream ss(buffer);
      JSON object(ss);
      if(object.string("method") != "textDocument/definition")
        return 1;

      std::string result = R"({
  "jsonrpc": "2.0",
  "id": 2,
  "result": [
    {
      "uri": "file://main.rs",
      "range": {
        "start": {
          "line": 0,
          "character": 3
        },
        "end": {
          "line": 0,
          "character": 7
        }
      }
    }
  ]
}
)";
      std::cout << "Content-Length: " << result.size() << "\r\n\r\n"
                << result;
    }

    // Read and write textDocument/typeDefinition
    {
      std::getline(std::cin, line);
      auto size = std::atoi(line.substr(16).c_str());
      std::getline(std::cin, line);
      std::string buffer;
      buffer.resize(size);
      std::cin.read(&buffer[0], size);
      std::stringstream ss(buffer);
      JSON object(ss);
      if(object.string("method") != "textDocument/typeDefinition")
        return 1;

      std::string result = R"({
  "jsonrpc": "2.0",
  "id": 3,
  "result": [
    {
      "uri": "file://main.rs",
      "range": {
        "start": {
          "line": 0,
          "character": 4
        },
        "end": {
          "line": 0,
          "character": 7
        }
      }
    }
  ]
}
)";
      std::cout << "Content-Length: " << result.size() << "\r\n\r\n"
                << result;
    }

    // Read and write textDocument/implementation
    {
      std::getline(std::cin, line);
      auto size = std::atoi(line.substr(16).c_str());
      std::getline(std::cin, line);
      std::string buffer;
      buffer.resize(size);
      std::cin.read(&buffer[0], size);
      std::stringstream ss(buffer);
      JSON object(ss);
      if(object.string("method") != "textDocument/implementation")
        return 1;

      std::string result = R"({
  "jsonrpc": "2.0",
  "id": 4,
  "result": [
    {
      "uri": "file://main.rs",
      "range": {
        "start": {
          "line": 0,
          "character": 0
        },
        "end": {
          "line": 0,
          "character": 1
        }
      }
    },
    {
      "uri": "file://main.rs",
      "range": {
        "start": {
          "line": 1,
          "character": 0
        },
        "end": {
          "line": 1,
          "character": 1
        }
      }
    }
  ]
}
)";
      std::cout << "Content-Length: " << result.size() << "\r\n\r\n"
                << result;
    }

    // Read and write textDocument/references
    {
      std::getline(std::cin, line);
      auto size = std::atoi(line.substr(16).c_str());
      std::getline(std::cin, line);
      std::string buffer;
      buffer.resize(size);
      std::cin.read(&buffer[0], size);
      std::stringstream ss(buffer);
      JSON object(ss);
      if(object.string("method") != "textDocument/references")
        return 1;

      std::string result = R"({
  "jsonrpc": "2.0",
  "id": 5,
  "result": [
    {
      "uri": "file://)" + JSON::escape_string(file_path.string()) +
                           R"(",
      "range": {
        "start": {
          "line": 2,
          "character": 19
        },
        "end": {
          "line": 2,
          "character": 20
        }
      }
    },
    {
      "uri": "file://)" + JSON::escape_string(file_path.string()) +
                           R"(",
      "range": {
        "start": {
          "line": 1,
          "character": 8
        },
        "end": {
          "line": 1,
          "character": 9
        }
      }
    }
  ]
}
)";
      std::cout << "Content-Length: " << result.size() << "\r\n\r\n"
                << result;
    }

    // Read and write textDocument/references
    {
      std::getline(std::cin, line);
      auto size = std::atoi(line.substr(16).c_str());
      std::getline(std::cin, line);
      std::string buffer;
      buffer.resize(size);
      std::cin.read(&buffer[0], size);
      std::stringstream ss(buffer);
      JSON object(ss);
      if(object.string("method") != "textDocument/documentSymbol")
        return 1;

      std::string result = R"({
  "jsonrpc": "2.0",
  "id": 6,
  "result": [
    {
      "name": "main",
      "kind": 12,
      "tags": "",
      "deprecated": false,
      "location": {
        "uri": "file://main.rs",
        "range": {
          "start": {
            "line": 0,
            "character": 0
          },
          "end": {
            "line": 3,
            "character": 1
          }
        }
      }
    }
  ]
}
)";
      std::cout << "Content-Length: " << result.size() << "\r\n\r\n"
                << result;
    }

    // Read textDocument/didClose
    {
      std::getline(std::cin, line);
      auto size = std::atoi(line.substr(16).c_str());
      std::getline(std::cin, line);
      std::string buffer;
      buffer.resize(size);
      std::cin.read(&buffer[0], size);
      std::stringstream ss(buffer);
      JSON object(ss);
      if(object.string("method") != "textDocument/didClose")
        return 1;
    }

    // Read shutdown and respond
    {
      std::getline(std::cin, line);
      auto size = std::atoi(line.substr(16).c_str());
      std::getline(std::cin, line);
      std::string buffer;
      buffer.resize(size);
      std::cin.read(&buffer[0], size);
      std::stringstream ss(buffer);
      JSON object(ss);
      if(object.string("method") != "shutdown")
        return 1;

      std::string result = R"({
  "jsonrpc": "2.0",
  "id": 7,
  "result": {}
})";
      std::cout << "Content-Length: " << result.size() << "\r\n\r\n"
                << result;
    }

    // Read exit
    {
      std::getline(std::cin, line);
      auto size = std::atoi(line.substr(16).c_str());
      std::getline(std::cin, line);
      std::string buffer;
      buffer.resize(size);
      std::cin.read(&buffer[0], size);
      std::stringstream ss(buffer);
      JSON object(ss);
      if(object.string("method") != "exit")
        return 1;
    }
  }
  catch(const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 2;
  }
}
