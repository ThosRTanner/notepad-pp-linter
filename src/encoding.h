#pragma once
#include <string>

class Encoding
{
  public:
    static int utfOffset(const std::string & utf8, int unicode_offset) noexcept;
};
