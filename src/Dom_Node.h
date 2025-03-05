#pragma once
#include <atlcomcli.h>
#include <msxml.h>

#include <string>

namespace Linter
{

class Dom_Node_List;

class Dom_Node
{
  public:
    explicit Dom_Node(CComPtr<IXMLDOMNode> node) noexcept;

    Dom_Node_List get_node_list(std::string const &xpath) const;

    Dom_Node get_node(std::string const &xpath) const;

    std::wstring get_name() const;

    CComVariant get_typed_value() const;

    CComVariant get_attribute(std::wstring const &attribute) const;

  private:
    CComPtr<IXMLDOMNode> node_;
};

}    // namespace Linter
