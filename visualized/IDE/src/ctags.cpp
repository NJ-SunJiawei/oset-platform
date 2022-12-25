#include "ctags.hpp"
#include "config.hpp"
#include "dialog.hpp"
#include "filesystem.hpp"
#include "project_build.hpp"
#include "terminal.hpp"
#include "utility.hpp"
#include <climits>
#include <vector>

Ctags::Ctags(const boost::filesystem::path &path, bool enable_scope, bool enable_kind, const std::string &languages) : enable_scope(enable_scope), enable_kind(enable_kind) {
  if(path.empty())
    return;
  // TODO: When universal ctags is available on all platforms (Ubuntu 20.04 LTS does), add: --pattern-length-limit=0
  auto options = " --sort=foldcase -I \"override noexcept\" -f -" + (!languages.empty() ? " --languages=" + languages : std::string());
  std::string fields(" --fields=n");
  if(enable_scope)
    fields += 's';
  if(enable_kind)
    fields += 'K';
  std::string command;
  boost::system::error_code ec;
  if(boost::filesystem::is_directory(path, ec)) {
    auto build = Project::Build::create(path);
    std::string exclude;
    for(auto &exclude_folder : build->get_exclude_folders())
      exclude += " --exclude=\"" + exclude_folder + "/*\" --exclude=\"*/" + exclude_folder + "/*\"";
    if(!build->project_path.empty())
      project_path = build->project_path;
    else
      project_path = path;
    command = Config::get().project.ctags_command + options + fields + exclude + " -R *";
  }
  else {
    project_path = path.parent_path();
    command = Config::get().project.ctags_command + options + fields + " --c-kinds=+p --c++-kinds=+p " + filesystem::escape_argument(path.string());
  }

  TinyProcessLib::Process process(
      command, project_path.string(),
      [this](const char *output, size_t length) {
        this->output.write(output, length);
      },
      [](const char *bytes, size_t n) {
        Terminal::get().async_print(std::string(bytes, n), true);
      });

  int exit_status;
  size_t count = 0;
  while(!process.try_get_exit_status(exit_status)) {
    ++count;
    if(count > 1000)
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  if(!process.try_get_exit_status(exit_status)) {
    bool canceled = false;
    Dialog::Message message("Please wait until ctags command completes", [&canceled] {
      canceled = true;
    });
    bool killed = false;
    while(!process.try_get_exit_status(exit_status)) {
      if(canceled && !killed) {
        process.kill();
        killed = true;
      }
      while(Gtk::Main::events_pending())
        Gtk::Main::iteration();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    message.hide();
    if(killed)
      output = std::stringstream();
  }
}

Ctags::operator bool() {
  output.seekg(0, std::ios::end);
  if(output.tellg() == 0)
    return false;
  output.seekg(0, std::ios::beg);
  return true;
}

Ctags::Location Ctags::get_location(const std::string &line_, bool add_markup, bool symbol_ends_with_open_parenthesis) const {
  Location location;

#ifdef _WIN32
  auto line = line_;
  if(!line.empty() && line.back() == '\r')
    line.pop_back();
#else
  auto &line = line_;
#endif
  auto symbol_end = line.find('\t');
  if(symbol_end == std::string::npos)
    return location;
  location.symbol = line.substr(0, symbol_end);
  if(9 < location.symbol.size() && location.symbol[8] == ' ' && starts_with(location.symbol, "operator")) {
    auto &chr = location.symbol[9];
    if(!((chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z') || (chr >= '0' && chr <= '9') || chr == '_' || chr == '$' || static_cast<unsigned char>(chr) >= 128))
      location.symbol.erase(8, 1);
  }

  auto file_start = symbol_end + 1;
  if(file_start >= line.size())
    return location;
  auto file_end = line.find('\t', file_start);
  if(file_end == std::string::npos)
    return location;
  location.file_path = line.substr(file_start, file_end - file_start);

  auto source_start = file_end + 3;
  if(source_start >= line.size())
    return location;
  location.index = 0;
  while(source_start < line.size() && (line[source_start] == ' ' || line[source_start] == '\t')) {
    ++source_start;
    ++location.index;
  }
  auto source_end = line.find("/;\"\t", source_start);
  if(source_end == std::string::npos)
    return location;

  // Unescape source
  if(source_end > source_start) {
    auto end = source_end - (line[source_end - 1] == '$' ? 1 : 0);
    location.source.reserve(end - source_start);
    bool escaped = false;
    for(auto i = source_start; i < end; ++i) {
      if(!escaped && line[i] == '\\') {
        escaped = true;
        continue;
      }
      escaped = false;
      location.source += line[i];
    }
  }

  size_t line_start;
  if(enable_kind) {
    auto kind_start = source_end + 4;
    if(kind_start >= line.size())
      return location;
    auto kind_end = line.find('\t', kind_start);
    if(kind_end == std::string::npos)
      return location;
    location.kind = line.substr(kind_start, kind_end - kind_start);
    line_start = kind_start + location.kind.size() + 6;
  }
  else
    line_start = source_end + 9;

  if(line_start >= line.size())
    return location;
  auto line_end = line.find('\t', line_start);
  size_t line_size = line_end == std::string::npos ? std::string::npos : line_end - line_start;
  try {
    location.line = std::stoul(line.substr(line_start, line_size)) - 1;
  }
  catch(...) {
    location.line = 0;
  }

  if(enable_scope && line_end != std::string::npos && line_end + 1 < line.size()) {
    auto scope_start = line.find(':', line_end + 1);
    if(scope_start != std::string::npos && scope_start + 1 < line.size())
      location.scope = line.substr(scope_start + 1);
  }

  if(!location.symbol.empty()) {
    if(add_markup) {
      location.source = Glib::Markup::escape_text(location.source);
      std::string symbol = Glib::Markup::escape_text(location.symbol);
      auto symbol_size = symbol.size();
      if(symbol_ends_with_open_parenthesis)
        symbol += '(';
      bool first = true;
      bool escaped = false;
      for(size_t i = 0; i < location.source.size(); i++) {
        if(!escaped) {
          if(starts_with(location.source, i, symbol)) {
            location.source.insert(i + symbol_size, "</b>");
            location.source.insert(i, "<b>");
            i += 7 + symbol_size - 1;
            first = false;
          }
          else {
            if(location.source[i] == '&') {
              escaped = true;
              i += 2; // Minimum character entities: &lt; and &gt;
            }
            if(first)
              location.index++;
          }
        }
        else if(location.source[i] == ';')
          escaped = false;
      }
    }
    else {
      auto pos = location.source.find(location.symbol);
      if(pos != std::string::npos)
        location.index += pos;
    }
  }

  return location;
}

// Split up a type into its various significant parts
std::vector<std::string> Ctags::get_type_parts(const std::string &type) {
  std::vector<std::string> parts;
  size_t text_start = std::string::npos;
  for(size_t c = 0; c < type.size(); ++c) {
    auto &chr = type[c];
    if((chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z') || (chr >= '0' && chr <= '9') || chr == '_' || chr == '$' || static_cast<unsigned char>(chr) >= 128 || chr == '~') {
      if(text_start == std::string::npos)
        text_start = c;
    }
    else {
      if(text_start != std::string::npos) {
        parts.emplace_back(type.substr(text_start, c - text_start));
        text_start = std::string::npos;
      }
      if(chr == '*' || chr == '&')
        parts.emplace_back(std::string() + chr);
      else if(chr == '{')
        break;
    }
  }
  return parts;
}

std::vector<Ctags::Location> Ctags::get_locations(const boost::filesystem::path &path, const std::string &name, const std::string &type, const std::string &languages) {
  Ctags ctags(path, true, false, languages);
  if(!ctags)
    return {};

  // insert name into type
  size_t c = 0;
  size_t bracket_count = 0;
  for(; c < type.size(); ++c) {
    if(type[c] == '<')
      ++bracket_count;
    else if(type[c] == '>')
      --bracket_count;
    else if(bracket_count == 0 && type[c] == '(')
      break;
  }
  auto full_type = type;
  full_type.insert(c, name);

  auto parts = get_type_parts(full_type);

  std::string line;
  long best_score = LONG_MIN;
  std::vector<Location> best_locations;
  while(std::getline(ctags.output, line)) {
    if(line.size() > 2048)
      continue;
    auto location = ctags.get_location(line);
    if(!location.scope.empty()) {
      if(location.scope + "::" + location.symbol != name)
        continue;
    }
    else if(location.symbol != name)
      continue;

    location.file_path = ctags.project_path / location.file_path;

    auto source_parts = get_type_parts(location.source);

    // Find match score
    long score = 0;
    size_t source_index = 0;
    for(auto &part : parts) {
      bool found = false;
      for(auto c = source_index; c < source_parts.size(); ++c) {
        if(part == source_parts[c]) {
          source_index = c + 1;
          ++score;
          found = true;
          break;
        }
      }
      if(!found)
        --score;
    }
    size_t index = 0;
    for(auto &source_part : source_parts) {
      bool found = false;
      for(auto c = index; c < parts.size(); ++c) {
        if(source_part == parts[c]) {
          index = c + 1;
          ++score;
          found = true;
          break;
        }
      }
      if(!found)
        --score;
    }

    if(score > best_score) {
      best_score = score;
      best_locations.clear();
      best_locations.emplace_back(location);
    }
    else if(score == best_score)
      best_locations.emplace_back(location);
  }

  return best_locations;
}
