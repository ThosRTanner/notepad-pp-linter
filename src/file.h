#pragma once
#include <string>
#include <utility>

class File
{
  public:
    File(const std::wstring &fileName, const std::wstring &directory);

    File(File const &) = delete;
    File(File &&) = delete;
    File &operator=(File const &) = delete;
    File &operator=(File &&) = delete;

    ~File();
    std::pair<std::string, std::string> exec(std::wstring commandLine, std::string const *str);
    void write(const std::string &data);

  private:
    std::wstring m_fileName;
    std::wstring m_directory;
    std::wstring m_file;
};
