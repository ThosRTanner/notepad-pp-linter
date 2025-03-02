#include "Checkstyle_Parser.h"

#include "Dom_Document.h"
#include "Dom_Node.h"
#include "Dom_Node_List.h"
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
    Dom_Document document{xml};

    // Sample errors:
    //
    // <error line="12" column="19" severity="error" message="Unexpected
    // identifier" source="jscs" />
    //
    // <error source="jshint.W101" message="Line is too long. (W101)"
    // severity="warning" column="81" line="58" />
    //
    // <error source="eslint.rules.jsdoc/require-description-complete-sentence"
    // message="Sentences should start with an uppercase character.
    // (jsdoc/require-description-complete-sentence)" severity="warning"
    // column="1" line="83" />
    //
    // We use the 1st word in source as the tool.

    std::vector<Checkstyle_Parser::Error> errors;

    Dom_Node_List nodes(document.get_node_list("//error"));
    for (auto const node : nodes)
    {
        CComVariant value = node.get_attribute(L"line");
        int const line = std::stoi(value.bstrVal);

        value = node.get_attribute(L"column");
        int const column = std::stoi(value.bstrVal);

        value = node.get_attribute(L"severity");
        std::wstring const severity(value.bstrVal);

        value = node.get_attribute(L"source");
        std::wstring tool{value.bstrVal};
        std::size_t const pos = tool.find_first_of('.');
        if (pos != std::string::npos)
        {
            tool.resize(pos);
        }

        value = node.get_attribute(L"message");
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
