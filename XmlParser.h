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
    };

    static std::vector<Error> getErrors(const std::string &xml);
};
