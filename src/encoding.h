#pragma once
#include <string>

class Encoding
{
  public:
    static int utfOffset(std::string const &utf8, int unicode_offset) noexcept;
};
