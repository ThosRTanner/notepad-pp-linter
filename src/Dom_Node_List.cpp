#include "Dom_Node_List.h"

#include "Dom_Node.h"
#include "System_Error.h"

#include <intsafe.h>

#include <utility>    // For std::move

namespace Linter
{

Dom_Node_List::iterator::iterator(
    CComPtr<IXMLDOMNodeList> node_list, LONG item
) noexcept :
    node_list_(std::move(node_list)),
    item_(item)
{
}

Dom_Node Dom_Node_List::iterator::operator*() const
{
    CComPtr<IXMLDOMNode> node;
    auto const hres = node_list_->get_item(item_, &node);
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres, "Can't get item detail");
    }
    return Dom_Node(node);
}

// NOLINTNEXTLINE(*-member-init)
Dom_Node_List::Dom_Node_List(CComPtr<IXMLDOMNodeList> node_list) :
    node_list_(std::move(node_list))
{
    HRESULT const hres = node_list_->get_length(&num_items_);
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres, "Can't get number of items in node list");
    }
}

Dom_Node_List::iterator Dom_Node_List::begin() const noexcept
{
    return iterator(node_list_, 0);
}

Dom_Node_List::iterator Dom_Node_List::end() const noexcept
{
    return iterator(node_list_, num_items_);
}

}    // namespace Linter
