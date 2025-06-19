#pragma once
#include <string>

class Encoding
{
  public:
    static int utfOffset(std::string const &utf8, int unicode_offset) noexcept;
    static std::string convert(std::wstring const &str) noexcept;
    static std::wstring convert(std::string const &utf8) noexcept;
};
