#include "XmlParser.h"

#include "DomDocument.h"
#include "SystemError.h"

#include <string>
#include <vector>

#include <msxml.h>

using Linter::SystemError;

std::vector<XmlParser::Error> XmlParser::getErrors(std::string const &xml)
{
    ::Linter::DomDocument XMLDocument{xml};
    // Sample errors:
    //
    // <error line="12" column="19" severity="error" message="Unexpected
    // identifier" source="jscs" /> <error source="jshint.W101" message="Line is
    // too long. (W101)" severity="warning" column="81" line="58" /> <error
    // source="eslint.rules.jsdoc/require-description-complete-sentence"
    //        message="Sentences should start with an uppercase character.
    //        (jsdoc/require-description-complete-sentence)" severity="warning"
    //        column="1" line="83" />
    //
    // We currently ignore severity, and use the 1st word in source as the tool.

    std::vector<XmlParser::Error> errors;

    CComPtr<IXMLDOMNodeList> XMLNodeList{XMLDocument.getNodeList("//error")};

    LONG num_errors;
    HRESULT hr = XMLNodeList->get_length(&num_errors);
    if (! SUCCEEDED(hr))
    {
        throw SystemError(hr, "Can't get XPath //error length");
    }
    for (LONG error = 0; error < num_errors; error++)
    {
        CComPtr<IXMLDOMNode> node;
        hr = XMLNodeList->get_item(error, &node);
        if (! SUCCEEDED(hr))
        {
            throw SystemError(hr, "Can't get next XPath element");
        }

        CComQIPtr<IXMLDOMElement> element(node);
        CComVariant value;

        element->getAttribute(static_cast<bstr_t>(L"line"), &value);
        int const line = std::stoi(value.bstrVal);

        element->getAttribute(static_cast<bstr_t>(L"column"), &value);
        int const column = std::stoi(value.bstrVal);

        element->getAttribute(static_cast<bstr_t>(L"severity"), &value);
        std::wstring const severity(value.bstrVal);

        element->getAttribute(static_cast<bstr_t>(L"source"), &value);
        std::wstring tool{value.bstrVal};
        std::size_t const pos = tool.find_first_of('.');
        if (pos != std::string::npos)
        {
            tool.resize(pos);
        }

        element->getAttribute(static_cast<bstr_t>(L"message"), &value);
        errors.push_back(Error{
            .line_ = line,
            .column_ = column,
            .message_ = value.bstrVal,
            .severity_ = severity,
            .tool_ = tool
        });
    }

    return errors;
}
