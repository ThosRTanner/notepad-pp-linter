#pragma once

// Fixme do we extract the command substructure somewhere?
#include "Settings.h"

#include <filesystem>
#include <string>
#include <utility>

namespace Linter
{

class File_Holder
{
  public:
    explicit File_Holder(std::filesystem::path const &);

    File_Holder(File_Holder const &) = delete;
    File_Holder(File_Holder &&) = delete;
    File_Holder &operator=(File_Holder const &) = delete;
    File_Holder &operator=(File_Holder &&) = delete;

    ~File_Holder();

    std::pair<std::string, std::string> exec(
        Settings::Linter::Command const &, std::string const &text
    );

    void write(std::string const &data);

  private:
    void setup_environment() const;

    std::wstring expand_arg_values(std::wstring) const;

    std::wstring expand_variables(std::wstring const &) const;

    std::filesystem::path path_;
    std::filesystem::path temp_file_;
};

}    // namespace Linter
