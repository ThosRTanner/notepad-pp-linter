#pragma once

// Fixme do we extract the command substructure somewhere?
#include "Settings.h"

#include <intsafe.h>

#include <filesystem>
#include <string>
#include <tuple>

class Plugin;

namespace Linter
{

class File_Linter
{
  public:
    explicit File_Linter(
        std::filesystem::path const &target,
        std::filesystem::path const &plugin_dir,
        std::filesystem::path const &settings_dir,
        std::vector<Settings::Variable> const &variables,
        std::string const &text
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
        Settings::Command const &, std::string const *const input = nullptr
    ) const;

    std::filesystem::path target_;
    std::filesystem::path plugin_dir_;
    std::filesystem::path settings_dir_;
    std::filesystem::path temp_file_;
    std::vector<Settings::Variable> const &variables_;
    std::string const text_;
    wchar_t const *orig_env_;

    std::vector<std::string> warnings_;

    bool created_temp_file_;
};

}    // namespace Linter
