#pragma once
#include <windef.h>    // for HWND

#include "List_View_Types.h"

namespace Linter
{

class Column_Header
{
  public:

    Column_Header(HWND handle) noexcept;

    ~Column_Header();

    using Data_Column = typename List_View_Types::Data_Column;
    using Sort_Direction = typename List_View_Types::Sort_Direction;

    void set_sort_icon(Data_Column, Sort_Direction) const noexcept;

  private:
    HWND handle_;
};

}    // namespace Linter
