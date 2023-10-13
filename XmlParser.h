#pragma once

#include <string>
#include <vector>

class XmlParser
{
  public:
    struct Error
    {
        Error(int line, int column, std::wstring const &message, std::wstring const &path, std::wstring const &tool)
            : m_line(line), m_column(column), m_message(message), m_path(path), m_tool(tool)
        {
        }

        int m_line;
        int m_column;
        std::wstring m_message;
        std::wstring m_path;
        std::wstring m_tool;
    };

    static std::vector<Error> getErrors(std::string const &xml, std::wstring const &path);
};
