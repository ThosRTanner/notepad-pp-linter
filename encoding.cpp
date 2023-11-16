#include "stdafx.h"
#include "encoding.h"

int Encoding::utfOffset(const std::string & utf8, int unicodeOffset) noexcept
{
    int result = 0;
    std::string::const_iterator i = utf8.begin(), end = utf8.end();
    while (unicodeOffset > 0 && i != end)
    {
        if ((*i & 0xC0) == 0xC0 && unicodeOffset == 1)
        {
            break;
        }
        if ((*i & 0x80) == 0 || (*i & 0xC0) == 0x80)
        {
            --unicodeOffset;
        }
        ++i;
        if (i != end && *i != 0x0D && *i != 0x0A)
        {
            ++result;
        }
    }

    return result;
}
