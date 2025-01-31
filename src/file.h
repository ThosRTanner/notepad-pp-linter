#pragma once

#include <filesystem>
#include <string>
#include <utility>

namespace Linter
{

class File
{
  public:
    explicit File(std::filesystem::path const &);

    File(File const &) = delete;
    File(File &&) = delete;
    File &operator=(File const &) = delete;
    File &operator=(File &&) = delete;

    ~File();

    std::pair<std::string, std::string> exec(
        std::wstring command_line, std::string const *str
    );

    void write(std::string const &data);

  private:
    std::filesystem::path path_;
    std::filesystem::path temp_file_;
};

}    // namespace Linter
