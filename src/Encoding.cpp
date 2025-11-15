#include "Encoding.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace Linter::Encoding
{

int utfOffset(std::string const &utf8, int unicode_offset) noexcept
{
    int result = 0;
    std::string::const_iterator iter = utf8.begin();
    std::string::const_iterator const end = utf8.end();
    while (unicode_offset > 0 && iter != end)
    {
        if ((*iter & 0xC0) == 0xC0 && unicode_offset == 1)
        {
            break;
        }
        if ((*iter & 0x80) == 0 || (*iter & 0xC0) == 0x80)
        {
            unicode_offset -= 1;
        }
        iter += 1;
        if (iter != end && *iter != 0x0D && *iter != 0x0A)
        {
            result += 1;
        }
    }

    return result;
}

std::string convert(std::wstring const &str)
{
    // The casts here are safe...
#pragma warning(push)
#pragma warning(disable : 26472)
    std::string result;
    for (wchar_t const chr : str)
    {
        if (chr <= 0x7F)
        {
            // 1-byte UTF-8
            result += static_cast<char>(chr);
        }
        else if (chr <= 0x7FF)
        {
            // 2-byte UTF-8
            result += static_cast<char>(0xC0 | ((chr >> 6) & 0x1F));
            result += static_cast<char>(0x80 | (chr & 0x3F));
        }
        else if (chr <= 0xFFFF)
        {
            // 3-byte UTF-8
            result += static_cast<char>(0xE0 | ((chr >> 12) & 0x0F));
            result += static_cast<char>(0x80 | ((chr >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (chr & 0x3F));
        }
        else
        {
            // Out of range for UTF-8 (non-BMP), replace with '?'
            result += '?';
        }
    }
#pragma warning(pop)
    return result;
}

std::wstring convert(std::string_view const str)
{
    // The array references are safe, but we could possibly rewrite this
    // using iterators insead of indices.
    // The static casts are safe according to clang tidy, but not windows.
#pragma warning(push)
#pragma warning(disable : 26446 26472)
    std::wstring result;
    std::size_t pos = 0;
    while (pos < str.length())
    {
        auto const ch1 = static_cast<unsigned char>(str[pos]);
        if (ch1 < 0x80)
        {
            // 1-byte ASCII
            result += ch1;
            pos += 1;
        }
        else if ((ch1 & 0xE0) == 0xC0 && pos + 1 < str.length())
        {
            // 2-byte sequence
            auto const ch2 = static_cast<unsigned char>(str[pos + 1]);
            if ((ch2 & 0xC0) == 0x80)
            {
                result += static_cast<wchar_t>(((ch1 & 0x1F) << 6) | (ch2 & 0x3F));
                pos += 2;
            }
            else
            {
                result += L'?';
                pos += 1;
            }
        }
        else if ((ch1 & 0xF0) == 0xE0 && pos + 2 < str.length())
        {
            // 3-byte sequence
            auto const ch2 = static_cast<unsigned char>(str[pos + 1]);
            auto const ch3 = static_cast<unsigned char>(str[pos + 2]);
            if ((ch2 & 0xC0) == 0x80 && (ch3 & 0xC0) == 0x80)
            {
                result += static_cast<wchar_t>(
                    ((ch1 & 0x0F) << 12) | ((ch2 & 0x3F) << 6) | (ch3 & 0x3F)
                );
                pos += 3;
            }
            else
            {
                result += L'?';
                pos += 1;
            }
        }
        else
        {
            // Invalid or unsupported sequence (including 4+ byte UTF-8)
            result += L'?';
            pos += 1;
        }
    }
#pragma warning(pop)
    return result;
}

}    // namespace Linter::Encoding
