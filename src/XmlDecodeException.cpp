#include "XmlDecodeException.h"

#include <comutil.h>
#include <msxml.h>
#include <wtypes.h>

#include <cstdio>
#include <iomanip>
#include <ios>
#include <sstream>

namespace Linter
{

XmlDecodeException::XmlDecodeException(IXMLDOMParseError &error)
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
        static_cast<char *>(static_cast<bstr_t>(msg.str().c_str()))
    );
}

XmlDecodeException::XmlDecodeException(XmlDecodeException const &) noexcept =
    default;

XmlDecodeException::XmlDecodeException(XmlDecodeException &&) noexcept =
    default;

XmlDecodeException &XmlDecodeException::operator=(XmlDecodeException const
                                                      &) noexcept = default;

XmlDecodeException &XmlDecodeException::operator=(XmlDecodeException
                                                      &&) noexcept = default;

XmlDecodeException::~XmlDecodeException() = default;

char const *XmlDecodeException::what() const noexcept
{
    return &what_string_[0];
}

}    // namespace Linter
