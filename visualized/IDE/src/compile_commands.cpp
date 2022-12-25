#include "compile_commands.hpp"
#include "clangmm.hpp"
#include "config.hpp"
#include "filesystem.hpp"
#include "json.hpp"
#include "terminal.hpp"
#include "utility.hpp"
#include <algorithm>
#include <regex>

CompileCommands::FindSystemIncludePaths::FindSystemIncludePaths() {
  std::stringstream stdin_stream, stdout_stream;
  stdin_stream << "int main() {}";
  exit_status = Terminal::get().process(stdin_stream, stdout_stream, "clang++ -v -x c++ -E 2>&1 -");
  if(exit_status != 0)
    return;
  std::string line;
  while(std::getline(stdout_stream, line)) {
    if(starts_with(line, "#include <...> search starts here:")) {
      while(std::getline(stdout_stream, line)) {
        if(!line.empty() && line[0] == ' ') {
#ifdef _WIN32
          if(line.back() == '\r')
            line.pop_back();
#endif
          auto end = line.find(" (framework directory)", 1);
          if(end == std::string::npos)
            include_paths.emplace_back(line.substr(1, end));
          else
            framework_paths.emplace_back(line.substr(1, end));
        }
        else
          return;
      }
      return;
    }
  }
}

std::vector<std::string> CompileCommands::Command::parameter_values(const std::string &parameter_name) const {
  std::vector<std::string> parameter_values;

  bool found_argument = false;
  for(auto &parameter : parameters) {
    if(found_argument) {
      parameter_values.emplace_back(parameter);
      found_argument = false;
    }
    else if(parameter == parameter_name)
      found_argument = true;
  }

  return parameter_values;
}

CompileCommands::CompileCommands(const boost::filesystem::path &build_path) {
  try {
    JSON compile_commands(build_path / "compile_commands.json");
    for(auto &command : compile_commands.array()) {
      boost::filesystem::path directory = command.string("directory");
      auto parameters_str = command.string("command");
      boost::filesystem::path file = command.string("file");

      std::vector<std::string> parameters;
      bool backslash = false;
      bool single_quote = false;
      bool double_quote = false;
      size_t parameter_start_pos = std::string::npos;
      size_t parameter_size = 0;
      auto add_parameter = [&parameters, &parameters_str, &parameter_start_pos, &parameter_size] {
        auto parameter = parameters_str.substr(parameter_start_pos, parameter_size);
        // Remove escaping
        for(size_t c = 0; c < parameter.size() - 1; ++c) {
          if(parameter[c] == '\\')
            parameter.replace(c, 2, std::string() + parameter[c + 1]);
        }
        parameters.emplace_back(std::move(parameter));
      };
      for(size_t c = 0; c < parameters_str.size(); ++c) {
        if(backslash)
          backslash = false;
        else if(parameters_str[c] == '\\')
          backslash = true;
        else if((parameters_str[c] == ' ' || parameters_str[c] == '\t') && !backslash && !single_quote && !double_quote) {
          if(parameter_start_pos != std::string::npos) {
            add_parameter();
            parameter_start_pos = std::string::npos;
            parameter_size = 0;
          }
          continue;
        }
        else if(parameters_str[c] == '\'' && !backslash && !double_quote) {
          single_quote = !single_quote;
          continue;
        }
        else if(parameters_str[c] == '\"' && !backslash && !single_quote) {
          double_quote = !double_quote;
          continue;
        }

        if(parameter_start_pos == std::string::npos)
          parameter_start_pos = c;
        ++parameter_size;
      }
      if(parameter_start_pos != std::string::npos)
        add_parameter();

      commands.emplace_back(Command{std::move(directory), std::move(parameters), filesystem::get_absolute_path(file, build_path)});
    }
  }
  catch(...) {
  }
}

std::vector<std::string> CompileCommands::get_arguments(const boost::filesystem::path &build_path, const boost::filesystem::path &file_path) {
  std::string default_std_argument = "-std=c++1y";

  auto extension = file_path.extension().string();
  bool is_header = CompileCommands::is_header(file_path) || extension.empty(); // Include std C++ headers that are without extensions

  // If header file, use source file flags if they are in the same folder
  std::vector<boost::filesystem::path> file_paths;
  if(is_header && !extension.empty()) {
    auto parent_path = file_path.parent_path();
    CompileCommands compile_commands(build_path);
    for(auto &command : compile_commands.commands) {
      if(command.file.parent_path() == parent_path)
        file_paths.emplace_back(command.file);
    }
  }

  if(file_paths.empty())
    file_paths.emplace_back(file_path);

  std::vector<std::string> arguments;
  if(!build_path.empty()) {
    clangmm::CompilationDatabase db(build_path.string());
    if(db) {
      for(auto &file_path : file_paths) {
        clangmm::CompileCommands compile_commands(file_path.string(), db);
        auto commands = compile_commands.get_commands();
        for(auto &command : commands) {
          auto cmd_arguments = command.get_arguments();
          bool ignore_next = false;
          for(size_t c = 1; c + 1 < cmd_arguments.size(); c++) { // Exclude first and last argument
            if(ignore_next)
              ignore_next = false;
            else if(cmd_arguments[c] == "-o" ||
                    cmd_arguments[c] == "-x" ||                          // Remove language arguments since some tools add languages not understood by clang
                    (is_header && cmd_arguments[c] == "-include-pch") || // Header files should not use precompiled headers
                    cmd_arguments[c] == "-MF") {                         // Exclude dependency file generation
              ignore_next = true;
            }
            else if(cmd_arguments[c] == "-c") {
            }
            else if(cmd_arguments[c] == "--") // End of command options
              break;
            else
              arguments.emplace_back(cmd_arguments[c]);
          }
        }
      }
    }
    else
      arguments.emplace_back(default_std_argument);
  }
  else
    arguments.emplace_back(default_std_argument);

  static FindSystemIncludePaths system_include_paths;
  if(system_include_paths) {
    for(auto &path : system_include_paths.include_paths)
      arguments.emplace_back("-I" + path);
    for(auto &path : system_include_paths.framework_paths)
      arguments.emplace_back("-F" + path);
#ifdef _WIN32
    auto clang_version_string = clangmm::to_string(clang_getClangVersion());
    const static std::regex clang_version_regex(R"(^[A-Za-z ]+([0-9.]+).*$)", std::regex::optimize);
    std::smatch sm;
    if(std::regex_match(clang_version_string, sm, clang_version_regex)) {
      auto clang_version = sm[1].str();
      auto env_msystem_prefix = std::getenv("MSYSTEM_PREFIX");
      if(env_msystem_prefix)
        arguments.emplace_back("-I" + (boost::filesystem::path(env_msystem_prefix) / "lib/clang" / clang_version / "include").string());
    }
#endif
  }
  else {
    auto clang_version_string = clangmm::to_string(clang_getClangVersion());
    const static std::regex clang_version_regex(R"(^[A-Za-z ]+([0-9.]+).*$)", std::regex::optimize);
    std::smatch sm;
    if(std::regex_match(clang_version_string, sm, clang_version_regex)) {
      auto clang_version = sm[1].str();
      arguments.emplace_back("-I/usr/lib/clang/" + clang_version + "/include");
      arguments.emplace_back("-I/usr/lib64/clang/" + clang_version + "/include"); // For Fedora
#if defined(__APPLE__)
      // Missing include and framework folders for MacOS:
      arguments.emplace_back("-I/usr/local/opt/llvm/include/c++/v1");
      arguments.emplace_back("-I/usr/local/opt/llvm/lib/clang/" + clang_version + "/include");
      arguments.emplace_back("-I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include");
      arguments.emplace_back("-F/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks");
#endif
#ifdef _WIN32
      auto env_msystem_prefix = std::getenv("MSYSTEM_PREFIX");
      if(env_msystem_prefix)
        arguments.emplace_back("-I" + (boost::filesystem::path(env_msystem_prefix) / "lib/clang" / clang_version / "include").string());
#endif
    }
  }

  // Do not add -fretain-comments-from-system-headers if pch is used, since the pch was most likely made without this flag
  if(std::none_of(arguments.begin(), arguments.end(), [](const std::string &argument) { return argument == "-include-pch"; }))
    arguments.emplace_back("-fretain-comments-from-system-headers");

  if(is_header) {
    arguments.emplace_back("-Wno-pragma-once-outside-header");
    arguments.emplace_back("-Wno-pragma-system-header-outside-header");
    arguments.emplace_back("-Wno-include-next-outside-header");
  }

  if(extension == ".cu" || extension == ".cuh") {
    arguments.emplace_back("-xcuda");
    arguments.emplace_back("-D__CUDACC__");
    arguments.emplace_back("-include");
    arguments.emplace_back("cuda_runtime.h");
    arguments.emplace_back("-ferror-limit=1000"); // CUDA headers redeclares some std functions
  }
  else if(extension == ".cl") {
    arguments.emplace_back("-xcl");
    arguments.emplace_back("-cl-std=CL2.0");
    arguments.emplace_back("-Xclang");
    arguments.emplace_back("-finclude-default-header");
    arguments.emplace_back("-Wno-gcc-compat");
  }
  else if(is_header)
    arguments.emplace_back("-xc++");

  if(!build_path.empty()) {
    arguments.emplace_back("-working-directory");
    arguments.emplace_back(build_path.string());
  }

  if(Config::get().source.clang_tidy_enable) {
    arguments.emplace_back("-Xclang");
    arguments.emplace_back("-add-plugin");
    arguments.emplace_back("-Xclang");
    arguments.emplace_back("clang-tidy");
    if(!Config::get().source.clang_tidy_checks.empty()) {
      arguments.emplace_back("-Xclang");
      arguments.emplace_back("-plugin-arg-clang-tidy");
      arguments.emplace_back("-Xclang");
      arguments.emplace_back("-checks=" + Config::get().source.clang_tidy_checks);
    }
  }

  return arguments;
}

bool CompileCommands::is_header(const boost::filesystem::path &path) {
  auto ext = path.extension();
  if(ext == ".h" ||                                                                     // c headers
     ext == ".hh" || ext == ".hp" || ext == ".hpp" || ext == ".h++" || ext == ".tcc" || // c++ headers
     ext == ".cuh")                                                                     // CUDA headers
    return true;
  else
    return false;
}

bool CompileCommands::is_source(const boost::filesystem::path &path) {
  auto ext = path.extension();
  if(ext == ".c" ||                                                                    // c sources
     ext == ".cpp" || ext == ".cxx" || ext == ".cc" || ext == ".C" || ext == ".c++" || // c++ sources
     ext == ".cu" ||                                                                   // CUDA sources
     ext == ".cl")                                                                     // OpenCL sources
    return true;
  else
    return false;
}
