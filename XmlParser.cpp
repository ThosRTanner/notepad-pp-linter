#include "stdafx.h"
#include "XmlParser.h"

#include "DomDocument.h"
#include "SystemError.h"
#include "encoding.h"

#include <memory>
#include <sstream>

#include <msxml6.h>

using Linter::SystemError;

std::vector<XmlParser::Error> XmlParser::getErrors(const std::string &xml, std::wstring const &path, std::wstring const &tool)
{
    ::Linter::DomDocument XMLDocument(xml);
    // <error line="12" column="19" severity="error" message="Unexpected identifier" source="jscs" />

    std::vector<XmlParser::Error> errors;

    CComPtr<IXMLDOMNodeList> XMLNodeList{XMLDocument.get_nodelist("//error")};

    //Why do we need uLength if we're using nextNode?
    LONG uLength;
    HRESULT hr = XMLNodeList->get_length(&uLength);
    if (!SUCCEEDED(hr))
    {
        throw SystemError(hr, "Can't get XPath //error length");
    }
    for (int iIndex = 0; iIndex < uLength; iIndex++)
    {
        CComPtr<IXMLDOMNode> node;
        hr = XMLNodeList->nextNode(&node);
        if (!SUCCEEDED(hr))
        {
            throw SystemError(hr, "Can't get next XPath element");
        }

        CComQIPtr<IXMLDOMElement> element(node);
        CComVariant value;

        element->getAttribute(bstr_t(L"line"), &value);
        int const line = _wtoi(value.bstrVal);

        element->getAttribute(bstr_t(L"column"), &value);
        int const column = _wtoi(value.bstrVal);

        element->getAttribute(bstr_t(L"message"), &value);


        errors.push_back(Error{line, column, value.bstrVal, path, tool});
    }

    return errors;
}
