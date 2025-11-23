#pragma once

#include <atlcomcli.h>
#include <msxml.h>

#include <optional>
#include <string>

namespace Linter
{

class Dom_Node_List;

class Dom_Node
{
  public:
    explicit Dom_Node(CComPtr<IXMLDOMNode> node) noexcept;

    /** Get the list of nodes matching the supplied xpath */
    Dom_Node_List get_node_list(std::string const &xpath) const;

    /** Get the (first) node matching the supplied xpath
     *
     * Throws an exception if no such node exists
     */
    Dom_Node get_node(std::string const &xpath) const;

    /** Get the (first) node matching the supplied xpath
     *
     * If no such node exists returns std::nullopt
     */
    std::optional<Dom_Node> get_optional_node(std::string const &xpath) const;

    /** Get the nodes name */
    std::wstring get_name() const;

    /** Get the value of the node as a string */
    std::wstring get_value() const;

    /** Get the value of the specified attribute
     *
     * Throws an exception if the attribute doesn't exist.
     */
    std::wstring get_attribute(std::wstring const &attribute) const;

  private:
    CComPtr<IXMLDOMNode> node_;
};

}    // namespace Linter
