#pragma once

#include <string>
#include <vector>

class XmlParser
{
  public:
    struct Error
    {
        int m_line;
        int m_column;
        std::wstring m_message;
        std::wstring m_severity;
        std::wstring m_tool;
    };

    static std::vector<Error> getErrors(std::string const &xml);
};
