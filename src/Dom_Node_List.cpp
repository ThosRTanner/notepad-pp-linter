#include "Dom_Node_List.h"

#include "Dom_Node.h"
#include "System_Error.h"

#include <intsafe.h>

namespace Linter
{

Dom_Node_List::iterator::iterator(
    CComPtr<IXMLDOMNodeList> node_list, LONG item
) noexcept :
    node_list_(node_list),
    item_(item)
{
}

Dom_Node Dom_Node_List::iterator::operator*() const
{
    CComPtr<IXMLDOMNode> node;
    auto const hr = node_list_->get_item(item_, &node);
    if (not SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't get item detail");
    }
    return Dom_Node(node);
}

Dom_Node_List::Dom_Node_List(CComPtr<IXMLDOMNodeList> node_list) :
    node_list_(node_list)
{
    HRESULT const hr = node_list->get_length(&num_items_);
    if (not SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't get number of items in node list");
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
