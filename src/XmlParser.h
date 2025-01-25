#pragma once

#include <string>
#include <vector>

class XmlParser
{
  public:
    struct Error
    {
        int line_;
        int column_;
        std::wstring message_;
        std::wstring severity_;
        std::wstring tool_;
    };

    static std::vector<Error> getErrors(std::string const &xml);
};
