#include "Column_Header.h"

#include <CommCtrl.h>

namespace Linter
{

Column_Header::Column_Header(HWND handle) noexcept : handle_(handle)
{
}

Column_Header::~Column_Header() = default;

void Column_Header::set_sort_icon(
    int column, Sort_Direction sort_direction
) const noexcept
{
    auto const count = Header_GetItemCount(handle_);
    for (int column_number = 0; column_number < count; column_number += 1)
    {
        HDITEM item{};
        item.mask = HDI_FORMAT;
        Header_GetItem(handle_, column_number, &item);
        item.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
        if (column_number == column)
        {
            switch (sort_direction)
            {
                case Sort_Ascending:
                    item.fmt |= HDF_SORTUP;
                    break;

                case Sort_Descending:
                    item.fmt |= HDF_SORTDOWN;
                    break;

                default:
                    break;
            }
        }
        Header_SetItem(handle_, column_number, &item);
    }
}

}    // namespace Linter
