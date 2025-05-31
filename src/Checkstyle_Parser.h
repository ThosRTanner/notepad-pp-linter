#pragma once

#include <string>
#include <vector>

namespace Linter
{

struct Error_Info;

class Checkstyle_Parser
{
  public:
    static std::vector<Error_Info> get_errors(std::string const &xml);
};

}    // namespace Linter
