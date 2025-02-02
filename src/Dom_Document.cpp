#include "Dom_Document.h"

#include "System_Error.h"
#include "XML_Decode_Error.h"

#include <atlcomcli.h>
#include <comutil.h>
#include <intsafe.h>
#include <minwindef.h>    //For FALSE
#include <msxml.h>

#include <string>

namespace Linter
{

Dom_Document::Dom_Document(std::wstring const &filename)
{
    init();

    CComVariant value{filename.c_str()};
    VARIANT_BOOL resultCode = FALSE;
    HRESULT const hr = document_->load(value, &resultCode);

    checkLoadResults(resultCode, hr);
}

Dom_Document::Dom_Document(std::string const &xml)
{
    init();

    VARIANT_BOOL resultCode = FALSE;
    HRESULT const hr =
        document_->loadXML(static_cast<_bstr_t>(xml.c_str()), &resultCode);

    checkLoadResults(resultCode, hr);
}

Dom_Document::~Dom_Document() = default;

CComPtr<IXMLDOMNodeList> Dom_Document::getNodeList(std::string const &xpath)
{
    CComPtr<IXMLDOMNodeList> nodes;
    HRESULT const hr =
        document_->selectNodes(static_cast<_bstr_t>(xpath.c_str()), &nodes);
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't execute XPath " + xpath);
    }
    return nodes;
}

void Dom_Document::init()
{
    HRESULT hr = document_.CoCreateInstance(__uuidof(DOMDocument));
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't create IID_IXMLDOMDocument2");
    }

    hr = document_->put_async(VARIANT_FALSE);
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't XMLDOMDocument2::put_async");
    }
}

void Dom_Document::checkLoadResults(VARIANT_BOOL resultcode, HRESULT hr)
{
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr);
    }
    if (resultcode == VARIANT_FALSE)
    {
        CComPtr<IXMLDOMParseError> error;
        document_->get_parseError(&error);
        throw XML_Decode_Error(*(error.p));
    }
}

}    // namespace Linter
