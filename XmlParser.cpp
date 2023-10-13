#include "stdafx.h"
#include "XmlParser.h"

#include "DomDocument.h"
#include "SystemError.h"
#include "encoding.h"

#include <memory>
#include <sstream>
#include <string>

#include <msxml6.h>

using Linter::SystemError;

std::vector<XmlParser::Error> XmlParser::getErrors(const std::string &xml, std::wstring const &path)
{
    ::Linter::DomDocument XMLDocument(xml);
    // Sample errors:
    // 
    // <error line="12" column="19" severity="error" message="Unexpected identifier" source="jscs" />
    // <error source = "jshint.W101" message = "Line is too long. (W101)" severity = "warning" column = "81" line = "58" />
    // <error source = "eslint.rules.jsdoc/require-description-complete-sentence" message =
    //        "Sentences should start with an uppercase character. (jsdoc/require-description-complete-sentence)" severity =
    //            "warning" column = "1" line = "83" />
    // 
    // The 'severity' is probably a bit useless
    // source might be useful as the tool source.

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
        int const line = std::stoi(value.bstrVal);

        element->getAttribute(bstr_t(L"column"), &value);
        int const column = std::stoi(value.bstrVal);

        element->getAttribute(bstr_t(L"source"), &value);
        std::wstring tool(value.bstrVal);
        std::size_t pos = tool.find_first_of('.');
        if (pos != std::string::npos)
        {
            tool = tool.substr(0, pos);
        }

        element->getAttribute(bstr_t(L"message"), &value);

        errors.push_back(Error{line, column, value.bstrVal, path, tool});
    }

    return errors;
}
