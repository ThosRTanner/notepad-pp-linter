#include "stdafx.h"
#include "XmlDecodeException.h"

#include <MsXml6.h>

#include <cstdio>

namespace Linter
{

    XmlDecodeException::XmlDecodeException(IXMLDOMParseError *error)
    {
        // Note that this constructor does allocations. Sorry.

        //Get the error line and column for later.
        error->get_line(&m_line);
        error->get_linepos(&m_column);

        //We use the rest of the information to construct the what message.
        BSTR url;
        error->get_url(&url);

        std::size_t pos = std::snprintf(m_buff,
            sizeof(m_buff),
            "Invalid xml in %s at line %ld col %ld",
            url == nullptr ? "temporary linter output file" : static_cast<std::string>(static_cast<bstr_t>(url)).c_str(),
            m_line,
            m_column);

        BSTR text;
        error->get_srcText(&text);
        if (text != nullptr)
        {
            pos += std::snprintf(
                m_buff + pos, sizeof(m_buff) - pos, " (near %s)", static_cast<std::string>(static_cast<bstr_t>(text)).c_str());
        }

        long code;
        error->get_errorCode(&code);
        BSTR reason;
        error->get_reason(&reason);
        std::snprintf(
            m_buff + pos, sizeof(m_buff) - pos, ": code %08lx %s", code, static_cast<std::string>(static_cast<bstr_t>(reason)).c_str());
    }

    XmlDecodeException::~XmlDecodeException() = default;

    char const *XmlDecodeException::what() const noexcept
    {
        return &m_buff[0];
    }

}    // namespace Linter