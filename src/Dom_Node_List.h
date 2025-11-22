#pragma once

#include <atlcomcli.h>
#include <intsafe.h>
#include <msxml.h>

namespace Linter
{

class Dom_Node;

class Dom_Node_List
{
  public:
    class iterator
    {
      public:
        iterator(CComPtr<IXMLDOMNodeList> node_list, LONG item) noexcept;

        iterator operator++() noexcept
        {
            item_ += 1;
            return *this;
        }

        bool operator!=(iterator const &other) const noexcept
        {
            return node_list_ != other.node_list_ || item_ != other.item_;
        }

        Dom_Node operator*() const;

      private:
        CComPtr<IXMLDOMNodeList> node_list_;
        LONG item_;
    };

    explicit Dom_Node_List(CComPtr<IXMLDOMNodeList> node_list);

    iterator begin() const noexcept;

    iterator end() const noexcept;

  private:
    CComPtr<IXMLDOMNodeList> node_list_;
    LONG num_items_;
};

}    // namespace Linter
