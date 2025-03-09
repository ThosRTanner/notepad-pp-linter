#include "Dom_Node.h"

#include "Dom_Node_List.h"
#include "System_Error.h"

#include <comutil.h>
#include <intsafe.h>
#include <winerror.h>

#include <stdexcept>

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
    auto node = get_optional_node(xpath);
    if (not node.has_value())
    {
        throw std::runtime_error("Can't get node from xpath " + xpath);
    }
    return *node;
}

std::optional<Dom_Node> Dom_Node::get_optional_node(std::string const &xpath) const
{
    CComPtr<IXMLDOMNode> node;
    auto const hr =
        node_->selectSingleNode(static_cast<bstr_t>(xpath.c_str()), &node);
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't get node from xpath " + xpath);
    }
    if (hr == S_FALSE)
    {
        return std::nullopt;
    }
    return Dom_Node(node);
}

std::wstring Dom_Node::get_name() const
{
    CComBSTR name;
    auto const hr = node_->get_nodeName(&name);
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't get node name");
    }
    return static_cast<wchar_t *>(name);
}

CComVariant Dom_Node::get_value() const
{
    CComVariant res;
    auto const hr = node_->get_nodeTypedValue(&res);
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't get node value");
    }
    return res;
}

std::wstring Dom_Node::get_attribute(std::wstring const &attribute) const
{
    CComQIPtr<IXMLDOMElement> element(node_);
    CComVariant value;

    auto const hr =
        element->getAttribute(static_cast<bstr_t>(attribute.c_str()), &value);
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't get node attribute value");
    }

    return value.bstrVal;
}

}    // namespace Linter
