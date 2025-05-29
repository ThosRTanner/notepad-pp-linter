#include "Checkstyle_Parser.h"

#include "Dom_Document.h"
#include "Dom_Node.h"
#include "Dom_Node_List.h"
#include "Error_Info.h"

#include <cstddef>
#include <string>
#include <vector>

namespace Linter
{

std::vector<Error_Info> Checkstyle_Parser::get_errors(std::string const &xml)
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

    std::vector<Error_Info> errors;

    Dom_Node_List nodes(document.get_node_list("//error"));
    for (auto const node : nodes)
    {
        std::wstring tool{node.get_attribute(L"source")};
        std::size_t const pos = tool.find_first_of('.');
        if (pos != std::string::npos)
        {
            tool.resize(pos);
        }

        errors.push_back(Error_Info{
            .message_ = node.get_attribute(L"message"),
            .severity_ = node.get_attribute(L"severity"),
            .tool_ = tool,
            .mode_ = Error_Info::Standard,
            .line_ = std::stoi(node.get_attribute(L"line")),
            .column_ = std::stoi(node.get_attribute(L"column"))
        });
    }

    return errors;
}

}    // namespace Linter
