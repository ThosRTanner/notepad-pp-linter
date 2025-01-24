#include "DomDocument.h"

#include "SystemError.h"
#include "XmlDecodeException.h"

#include <atlcomcli.h>
#include <comutil.h>
#include <msxml.h>
#include <winerror.h>
#include <wtypes.h>

#include <string>

namespace Linter
{

DomDocument::DomDocument(std::wstring const &filename)
{
    init();

    CComVariant value{filename.c_str()};
    VARIANT_BOOL resultCode = FALSE;
    HRESULT const hr = m_document->load(value, &resultCode);

    checkLoadResults(resultCode, hr);
}

DomDocument::DomDocument(std::string const &xml)
{
    init();

    VARIANT_BOOL resultCode = FALSE;
    HRESULT const hr =
        m_document->loadXML(static_cast<_bstr_t>(xml.c_str()), &resultCode);

    checkLoadResults(resultCode, hr);
}

DomDocument::~DomDocument() = default;

CComPtr<IXMLDOMNodeList> DomDocument::getNodeList(std::string const &xpath)
{
    CComPtr<IXMLDOMNodeList> nodes;
    HRESULT const hr =
        m_document->selectNodes(static_cast<_bstr_t>(xpath.c_str()), &nodes);
    if (! SUCCEEDED(hr))
    {
        throw SystemError(hr, "Can't execute XPath " + xpath);
    }
    return nodes;
}

void DomDocument::init()
{
    HRESULT hr = m_document.CoCreateInstance(__uuidof(DOMDocument));
    if (! SUCCEEDED(hr))
    {
        throw SystemError(hr, "Can't create IID_IXMLDOMDocument2");
    }

    hr = m_document->put_async(VARIANT_FALSE);
    if (! SUCCEEDED(hr))
    {
        throw SystemError(hr, "Can't XMLDOMDocument2::put_async");
    }
}

void DomDocument::checkLoadResults(VARIANT_BOOL resultcode, HRESULT hr)
{
    if (! SUCCEEDED(hr))
    {
        throw SystemError(hr);
    }
    if (resultcode != VARIANT_TRUE)
    {
        CComPtr<IXMLDOMParseError> error;
        m_document->get_parseError(&error);
        throw XmlDecodeException(error);
    }
}

}    // namespace Linter