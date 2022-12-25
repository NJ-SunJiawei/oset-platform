#include "commands.hpp"
#include "config.hpp"
#include "filesystem.hpp"
#include "json.hpp"
#include "terminal.hpp"

void Commands::load() {
  auto commands_file = Config::get().home_juci_path / "commands.json";

  boost::system::error_code ec;
  if(!boost::filesystem::exists(commands_file, ec))
    filesystem::write(commands_file, R"([
  {
    "key": "<primary><shift>1",
    "path_comment": "Regular expression for which paths this command should apply",
    "path": "^.*\\.json$",
    "compile_comment": "Add compile command if a compilation step is needed prior to the run command. <path_match> is set to the matching file or directory, and <working_directory> is set to the project directory if found or the matching file's directory.",
    "compile": "",
    "run_comment": "<path_match> is set to the matching file or directory, and <working_directory> is set to the project directory if found or the matching file's directory",
    "run": "echo <path_match> && echo <working_directory>",
    "debug_comment": "Whether or not this command should run through debugger",
    "debug": false,
    "debug_remote_host": ""
  }
]
)");

  commands.clear();
  try {
    JSON commands_json(commands_file);
    for(auto &command : commands_json.array()) {
      auto key_string = command.string("key");
      guint key = 0;
      GdkModifierType modifier = static_cast<GdkModifierType>(0);
      if(!key_string.empty()) {
        gtk_accelerator_parse(key_string.c_str(), &key, &modifier);
        if(key == 0 && modifier == 0)
          Terminal::get().async_print("\e[31mError\e[m: could not parse key string: " + key_string + "\n", true);
      }
      auto path = command.string_or("path", "");
      boost::optional<std::regex> regex;
      if(!path.empty())
        regex = std::regex(path, std::regex::optimize);
      commands.emplace_back(Command{key, modifier, std::move(regex),
                                    command.string_or("compile", ""), command.string("run"),
                                    command.boolean_or("debug", false), command.string_or("debug_remote_host", "")});
    }
  }
  catch(const std::exception &e) {
    Terminal::get().async_print(std::string("\e[31mError\e[m: ") + e.what() + "\n", true);
  }
}
