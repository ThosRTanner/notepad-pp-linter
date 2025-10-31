#pragma once
#include <windef.h>    // for HWND

namespace Linter
{

class Column_Header
{
  public:
    Column_Header(HWND handle) noexcept;

    ~Column_Header();

    enum Sort_Direction : int
    {
        Sort_None = 0,
        Sort_Ascending = 1,
        Sort_Descending = 2
    };

    void set_sort_icon(int column, Sort_Direction sort_direction) const noexcept;

  private:
    HWND handle_;
};

}    // namespace Linter
