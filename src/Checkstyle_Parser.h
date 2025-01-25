#pragma once

#include <string>
#include <vector>

namespace Linter
{

class Checkstyle_Parser
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

    static std::vector<Error> get_errors(std::string const &xml);
};

}    // namespace Linter
