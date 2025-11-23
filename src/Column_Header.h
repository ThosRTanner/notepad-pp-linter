#pragma once

#include <windef.h>    // for HWND

#include "List_View_Types.h"

namespace Linter
{

class Column_Header
{
  public:

    explicit Column_Header(HWND handle) noexcept;

    // No copy or move
    Column_Header(Column_Header const &) = delete;
    Column_Header &operator=(Column_Header const &) = delete;
    Column_Header(Column_Header &&) = delete;
    Column_Header &operator=(Column_Header &&) = delete;

    ~Column_Header() = default;

    using Data_Column = typename List_View_Types::Data_Column;
    using Sort_Direction = typename List_View_Types::Sort_Direction;

    /** Set the sort icon to up arrow, down arrow or off */
    void set_sort_icon(Data_Column, Sort_Direction) const noexcept;

    /** Get the total number of columns */
    int get_num_columns() const noexcept;

  private:
    HWND handle_;
};

}    // namespace Linter
