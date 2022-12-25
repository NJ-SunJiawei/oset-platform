#pragma once
#include "cmake.hpp"
#include "meson.hpp"
#include <boost/filesystem.hpp>
#include <memory>

namespace Project {
  class Build {
  public:
    virtual ~Build() {}

    boost::filesystem::path project_path;

    virtual boost::filesystem::path get_default_path();
    virtual bool update_default(bool force = false) { return false; }
    virtual boost::filesystem::path get_debug_path();
    virtual bool update_debug(bool force = false) { return false; }

    virtual std::string get_compile_command() { return std::string(); }
    virtual boost::filesystem::path get_executable(const boost::filesystem::path &path) { return boost::filesystem::path(); }

    /// Returns false if an invalid build is found, true otherwise even when build is not found.
    virtual bool is_valid(const boost::filesystem::path &build_path) const { return true; }

    std::vector<std::string> get_exclude_folders();

    static std::unique_ptr<Build> create(const boost::filesystem::path &path);
  };

  class CMakeBuild : public Build {
    ::CMake cmake;

  public:
    CMakeBuild(const boost::filesystem::path &path);

    bool update_default(bool force = false) override;
    bool update_debug(bool force = false) override;

    std::string get_compile_command() override;
    boost::filesystem::path get_executable(const boost::filesystem::path &path) override;

    bool is_valid(const boost::filesystem::path &build_path) const override;
  };

  class MesonBuild : public Build {
    Meson meson;

  public:
    MesonBuild(const boost::filesystem::path &path);

    bool update_default(bool force = false) override;
    bool update_debug(bool force = false) override;

    std::string get_compile_command() override;
    boost::filesystem::path get_executable(const boost::filesystem::path &path) override;

    bool is_valid(const boost::filesystem::path &build_path) const override;
  };

  class CompileCommandsBuild : public Build {
  public:
  };

  class CargoBuild : public Build {
  public:
    boost::filesystem::path get_default_path() override { return project_path / "target" / "debug"; }
    bool update_default(bool force = false) override;
    boost::filesystem::path get_debug_path() override { return get_default_path(); }
    bool update_debug(bool force = false) override;

    std::string get_compile_command() override;
    boost::filesystem::path get_executable(const boost::filesystem::path &path) override;
  };

  class NpmBuild : public Build {
  };

  class PythonMain : public Build {
  };

  class GoBuild : public Build {
  };
} // namespace Project
