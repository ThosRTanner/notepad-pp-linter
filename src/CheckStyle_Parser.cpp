#include "Checkstyle_Parser.h"

#include "Dom_Document.h"
#include "System_Error.h"

#include <atlcomcli.h>
#include <comutil.h>
#include <intsafe.h>
#include <msxml.h>

#include <cstddef>
#include <string>
#include <vector>

namespace Linter
{

std::vector<Checkstyle_Parser::Error> Checkstyle_Parser::get_errors(
    std::string const &xml
)
{
    Dom_Document XMLDocument{xml};

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

    std::vector<Checkstyle_Parser::Error> errors;

    CComPtr<IXMLDOMNodeList> XMLNodeList{XMLDocument.getNodeList("//error")};

    LONG num_errors;
    HRESULT hr = XMLNodeList->get_length(&num_errors);
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't get XPath //error length");
    }
    for (LONG error = 0; error < num_errors; error++)
    {
        CComPtr<IXMLDOMNode> node;
        hr = XMLNodeList->get_item(error, &node);
        if (! SUCCEEDED(hr))
        {
            throw System_Error(hr, "Can't get next XPath element");
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

}    // namespace Linter
