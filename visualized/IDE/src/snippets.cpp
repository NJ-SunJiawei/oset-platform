#include "snippets.hpp"
#include "config.hpp"
#include "filesystem.hpp"
#include "json.hpp"
#include "terminal.hpp"

void Snippets::load() {
  auto snippets_file = Config::get().home_juci_path / "snippets.json";

  boost::system::error_code ec;
  if(!boost::filesystem::exists(snippets_file, ec))
    filesystem::write(snippets_file, R"({
  "^markdown$": [
    {
      "key": "<primary>1",
      "prefix": "code_block",
      "body": "```${1:language}\n${2:code}\n```\n",
      "description": "Insert code block"
    }
  ]
}
)");

  snippets.clear();
  try {
    JSON languages(snippets_file);
    for(auto &language : languages.children()) {
      snippets.emplace_back(std::regex(language.first), std::vector<Snippet>());
      for(auto &snippet : language.second.array()) {
        auto key_string = snippet.string_or("key", "");
        guint key = 0;
        GdkModifierType modifier = static_cast<GdkModifierType>(0);
        if(!key_string.empty()) {
          gtk_accelerator_parse(key_string.c_str(), &key, &modifier);
          if(key == 0 && modifier == 0)
            Terminal::get().async_print("\e[31mError\e[m: could not parse key string: " + key_string + "\n", true);
        }
        snippets.back().second.emplace_back(Snippet{snippet.string_or("prefix", ""), key, modifier, snippet.string("body"), snippet.string_or("description", "")});
      }
    }
  }
  catch(const std::exception &e) {
    Terminal::get().async_print(std::string("\e[31mError\e[m: ") + e.what() + "\n", true);
  }
}
