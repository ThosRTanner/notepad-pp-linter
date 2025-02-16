#pragma once

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
        std::wstring command_line, std::string const *str
    );

    void write(std::string const &data);

  private:
    std::filesystem::path path_;
    std::filesystem::path temp_file_;
};

}    // namespace Linter
