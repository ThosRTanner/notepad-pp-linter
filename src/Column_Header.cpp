#include "Column_Header.h"

#include "List_View_Types.h"

#include <CommCtrl.h>

namespace Linter
{

Column_Header::Column_Header(HWND handle) noexcept : handle_(handle)
{
}

void Column_Header::set_sort_icon(
    Data_Column column, Sort_Direction sort_direction
) const noexcept
{
    auto const count = Header_GetItemCount(handle_);
    for (int column_number = 0; column_number < count; column_number += 1)
    {
        HDITEM item{.mask = HDI_FORMAT};
        Header_GetItem(handle_, column_number, &item);
        item.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
        if (column_number == column)
        {
            switch (sort_direction)
            {
                case Sort_Direction::Ascending:
                    item.fmt |= HDF_SORTUP;
                    break;

                case Sort_Direction::Descending:
                    item.fmt |= HDF_SORTDOWN;
                    break;

                default:
                    break;
            }
        }
        Header_SetItem(handle_, column_number, &item);
    }
}

int Column_Header::get_num_columns() const noexcept
{
    return Header_GetItemCount(handle_);
}

}    // namespace Linter
