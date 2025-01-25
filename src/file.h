#pragma once
#include <string>
#include <utility>

namespace Linter
{

class File
{
  public:
    File(std::wstring const &filename, std::wstring const &directory);

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
    std::wstring filename_;
    std::wstring directory_;
    std::wstring file_;
};

}    // namespace Linter
