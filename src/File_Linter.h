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
        std::string const &text
    );

    File_Linter(File_Linter const &) = delete;
    File_Linter(File_Linter &&) = delete;
    File_Linter &operator=(File_Linter const &) = delete;
    File_Linter &operator=(File_Linter &&) = delete;

    ~File_Linter();

    std::tuple<std::wstring, DWORD, std::string, std::string> run_linter(
        Settings::Linter::Command const &);

    std::filesystem::path get_temp_file_name() const;

    void ensure_temp_file_exists();
    
  private:
    void setup_environment() const;

    std::wstring expand_arg_values(std::wstring) const;

    std::wstring expand_variables(std::wstring const &) const;

    std::filesystem::path target_;
    std::filesystem::path plugin_dir_;
    std::filesystem::path settings_dir_;
    std::filesystem::path temp_file_;
    std::string const &text_;

    bool created_temp_file_;
};

}    // namespace Linter
