#include "Dom_Node.h"

#include "Dom_Node_List.h"
#include "System_Error.h"

#include <comutil.h>
#include <intsafe.h>
#include <oleauto.h>
#include <winerror.h>
#include <wtypes.h>

#include <stdexcept>

namespace Linter
{

Dom_Node::Dom_Node(CComPtr<IXMLDOMNode> node) noexcept : node_(node)
{
}

Dom_Node_List Dom_Node::get_node_list(std::string const &xpath) const
{
    CComPtr<IXMLDOMNodeList> node_list;
    auto const hres =
        node_->selectNodes(static_cast<bstr_t>(xpath.c_str()), &node_list);
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres, "Can't get node list from " + xpath);
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

std::optional<Dom_Node> Dom_Node::get_optional_node(
    std::string const &xpath
) const
{
    CComPtr<IXMLDOMNode> node;
    auto const hres =
        node_->selectSingleNode(static_cast<bstr_t>(xpath.c_str()), &node);
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres, "Can't get node from xpath " + xpath);
    }
    if (hres == S_FALSE)
    {
        return std::nullopt;
    }
    return Dom_Node(node);
}

std::wstring Dom_Node::get_name() const
{
    CComBSTR name;
    auto const hres = node_->get_nodeName(&name);
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres, "Can't get node name");
    }
    return std::wstring(static_cast<BSTR>(name), name.Length());
}

std::wstring Dom_Node::get_value() const
{
    CComVariant res;
    auto const hres = node_->get_nodeTypedValue(&res);
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres, "Can't get node value");
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    return std::wstring(res.bstrVal, SysStringLen(res.bstrVal));
}

std::wstring Dom_Node::get_attribute(std::wstring const &attribute) const
{
    CComQIPtr<IXMLDOMElement> const element(node_);
    CComVariant value;

    auto const hres =
        element->getAttribute(static_cast<bstr_t>(attribute.c_str()), &value);
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres, "Can't get node attribute value");
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    return std::wstring(value.bstrVal, SysStringLen(value.bstrVal));
}

}    // namespace Linter
