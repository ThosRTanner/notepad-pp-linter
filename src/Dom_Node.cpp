#include "Dom_Node.h"

#include "Dom_Node_List.h"
#include "System_Error.h"

#include <comutil.h>
#include <intsafe.h>

namespace Linter
{

Dom_Node::Dom_Node(CComPtr<IXMLDOMNode> node) noexcept : node_(node)
{
}

Dom_Node_List Dom_Node::get_node_list(std::string const &xpath) const
{
    CComPtr<IXMLDOMNodeList> node_list;
    auto const hr =
        node_->selectNodes(static_cast<bstr_t>(xpath.c_str()), &node_list);
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't get node list from " + xpath);
    }
    return Dom_Node_List(node_list);
}

Dom_Node Dom_Node::get_node(std::string const &xpath) const
{
    CComPtr<IXMLDOMNode> node;
    auto const hr =
        node_->selectSingleNode(static_cast<bstr_t>(xpath.c_str()), &node);
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't get node from xpath " + xpath);
    }
    return Dom_Node(node);
}

CComVariant Dom_Node::get_typed_value() const
{
    CComVariant res;
    auto const hr = node_->get_nodeTypedValue(&res);
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't get node value");
    }
    return res;
}

CComVariant Dom_Node::get_attribute(
    std::wstring const &attribute
) const
{
    CComQIPtr<IXMLDOMElement> element(node_);
    CComVariant value;

    auto const hr = element->getAttribute(static_cast<bstr_t>(attribute.c_str()), &value);
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't get node attribute value");
    }

    return value;
}

}    // namespace Linter
