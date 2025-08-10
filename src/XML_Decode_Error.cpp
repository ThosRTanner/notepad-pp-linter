#include "XML_Decode_Error.h"

#include "Encoding.h"

// IWYU says you don't need this. But you do...
#include <comutil.h>    // IWYU pragma: keep
#include <msxml.h>
#include <wtypes.h>

#include <cstdio>
#include <iomanip>
#include <ios>
#include <sstream>

namespace Linter
{

XML_Decode_Error::XML_Decode_Error(IXMLDOMParseError &error)
{
    // Note that this constructor does allocations. Sorry.

    // Get the error line and column for later.
    error.get_line(&line_);
    error.get_linepos(&column_);

    // We use the rest of the information to construct the what message.
    BSTR url;
    error.get_url(&url);

    std::wostringstream msg;
    msg << "Invalid xml in "
        << (url == nullptr ? L"temporary linter output file" : url)
        << " at line " << line_ << " col " << column_;

    BSTR text;
    error.get_srcText(&text);
    if (text != nullptr)
    {
        msg << " (near " << text << ")";
    }

    long code;
    error.get_errorCode(&code);

    BSTR reason;
    error.get_reason(&reason);

    msg << ": code 0x" << std::hex << std::setw(8) << std::setfill(L'0') << code
        << " " << reason;

    std::snprintf(
        &what_string_[0],
        sizeof(what_string_),
        "%s",
        Encoding::convert(msg.str()).c_str()
    );
}

XML_Decode_Error::XML_Decode_Error(XML_Decode_Error const &) noexcept = default;

XML_Decode_Error::XML_Decode_Error(XML_Decode_Error &&) noexcept = default;

XML_Decode_Error &XML_Decode_Error::operator=(XML_Decode_Error const
                                                  &) noexcept = default;

XML_Decode_Error &XML_Decode_Error::operator=(XML_Decode_Error &&) noexcept =
    default;

XML_Decode_Error::~XML_Decode_Error() = default;

char const *XML_Decode_Error::what() const noexcept
{
    return &what_string_[0];
}

}    // namespace Linter
