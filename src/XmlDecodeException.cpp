#include "XmlDecodeException.h"

#include <comutil.h>
#include <msxml.h>

#include <cstdio>
#include <iomanip>
#include <ios>
#include <sstream>

Linter::XmlDecodeException::XmlDecodeException(IXMLDOMParseError &error)
{
    // Note that this constructor does allocations. Sorry.

    // Get the error line and column for later.
    error.get_line(&m_line);
    error.get_linepos(&m_column);

    // We use the rest of the information to construct the what message.
    BSTR url;
    error.get_url(&url);

    std::wostringstream msg;
    msg << "Invalid xml in "
        << (url == nullptr ? L"temporary linter output file" : url)
        << " at line " << m_line << " col " << m_column;

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
        &m_buff[0],
        sizeof(m_buff),
        "%s",
        static_cast<char *>(static_cast<bstr_t>(msg.str().c_str()))
    );
}

Linter::XmlDecodeException::XmlDecodeException(XmlDecodeException const
                                                   &) noexcept = default;

Linter::XmlDecodeException::XmlDecodeException(XmlDecodeException &&) noexcept =
    default;

Linter::XmlDecodeException &
Linter::XmlDecodeException::operator=(XmlDecodeException const &) noexcept =
    default;

Linter::XmlDecodeException &
Linter::XmlDecodeException::operator=(XmlDecodeException &&) noexcept = default;

Linter::XmlDecodeException::~XmlDecodeException() = default;

char const *Linter::XmlDecodeException::what() const noexcept
{
    return &m_buff[0];
}
