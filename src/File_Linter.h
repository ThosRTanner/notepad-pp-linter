#pragma once

// Fixme do we extract the command substructure somewhere?
#include "Settings.h"

#include <intsafe.h>

#include <filesystem>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace Linter
{

class Environment_Wrapper;

class File_Linter
{
  public:
    explicit File_Linter(
        std::filesystem::path target,
        std::filesystem::path plugin_dir,
        std::filesystem::path settings_dir,
        std::vector<Settings::Variable> const &variables,
        std::string text
    );

    File_Linter(File_Linter const &) = delete;
    File_Linter(File_Linter &&) = delete;
    File_Linter &operator=(File_Linter const &) = delete;
    File_Linter &operator=(File_Linter &&) = delete;

    ~File_Linter();

    std::tuple<std::wstring, DWORD, std::string, std::string>
    run_linter(Settings::Command const &);

    std::filesystem::path get_temp_file_name() const;

    void ensure_temp_file_exists();

    std::vector<std::string> const &warnings() const noexcept
    {
        return warnings_;
    }

  private:
    void setup_environment();

    static std::wstring expand_variables(std::wstring const &);

    std::tuple<DWORD, std::string, std::string> execute(
        Settings::Command const &, std::string const * input = nullptr
    ) const;

    std::filesystem::path target_;
    std::filesystem::path plugin_dir_;
    std::filesystem::path settings_dir_;
    std::filesystem::path temp_file_;
    std::vector<Settings::Variable> const &variables_;
    std::string const text_;
    std::unique_ptr<Environment_Wrapper> env_;

    std::vector<std::string> warnings_;

    bool created_temp_file_{false};
};

}    // namespace Linter
