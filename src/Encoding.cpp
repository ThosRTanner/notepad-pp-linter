#include "Encoding.h"

#include <string>
#include <string_view>

#include <stddef.h>

namespace Linter
{
namespace Encoding
{

int utfOffset(std::string const &utf8, int unicode_offset) noexcept
{
    int result = 0;
    std::string::const_iterator i = utf8.begin();
    std::string::const_iterator const end = utf8.end();
    while (unicode_offset > 0 && i != end)
    {
        if ((*i & 0xC0) == 0xC0 && unicode_offset == 1)
        {
            break;
        }
        if ((*i & 0x80) == 0 || (*i & 0xC0) == 0x80)
        {
            --unicode_offset;
        }
        i += 1;
        if (i != end && *i != 0x0D && *i != 0x0A)
        {
            ++result;
        }
    }

    return result;
}

std::string convert(std::wstring const &str) noexcept
{
    // The casts here are safe...
#pragma warning(push)
#pragma warning(disable : 26472)
    std::string result;
    for (wchar_t const wc : str)
    {
        if (wc <= 0x7F)
        {
            // 1-byte UTF-8
            result += static_cast<char>(wc);
        }
        else if (wc <= 0x7FF)
        {
            // 2-byte UTF-8
            result += static_cast<char>(0xC0 | ((wc >> 6) & 0x1F));
            result += static_cast<char>(0x80 | (wc & 0x3F));
        }
        else if (wc <= 0xFFFF)
        {
            // 3-byte UTF-8
            result += static_cast<char>(0xE0 | ((wc >> 12) & 0x0F));
            result += static_cast<char>(0x80 | ((wc >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (wc & 0x3F));
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

std::wstring convert(std::string_view const str) noexcept
{
    // The array references are safe, but we could possibly rewrite this
    // using iterators insead of indices.
#pragma warning(push)
#pragma warning(disable : 26446)
    std::wstring result;
    size_t i = 0;
    while (i < str.length())
    {
        unsigned char const c = static_cast<unsigned char>(str[i]);
        if (c < 0x80)
        {
            // 1-byte ASCII
            result += c;
            i += 1;
        }
        else if ((c & 0xE0) == 0xC0 && i + 1 < str.length())
        {
            // 2-byte sequence
            unsigned char const c1 = static_cast<unsigned char>(str[i + 1]);
            if ((c1 & 0xC0) == 0x80)
            {
                result += ((c & 0x1F) << 6) | (c1 & 0x3F);
                i += 2;
            }
            else
            {
                result += L'?';
                i += 1;
            }
        }
        else if ((c & 0xF0) == 0xE0 && i + 2 < str.length())
        {
            // 3-byte sequence
            unsigned char const c1 = static_cast<unsigned char>(str[i + 1]);
            unsigned char const c2 = static_cast<unsigned char>(str[i + 2]);
            if ((c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80)
            {
                result += ((c & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
                i += 3;
            }
            else
            {
                result += L'?';
                i += 1;
            }
        }
        else
        {
            // Invalid or unsupported sequence (including 4+ byte UTF-8)
            result += L'?';
            i += 1;
        }
    }
#pragma warning(pop)
    return result;
}

}    // namespace Encoding
}    // namespace Linter
