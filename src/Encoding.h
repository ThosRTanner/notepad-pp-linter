#pragma once
#include <string>
#include <string_view>

namespace Linter::Encoding
{

int utfOffset(std::string const &utf8, int unicode_offset) noexcept;
std::string convert(std::wstring const &str);
std::wstring convert(std::string_view str);

}    // namespace Linter::Encoding
