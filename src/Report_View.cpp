#include "Report_View.h"

#include "Column_Header.h"

#include <CommCtrl.h>

#include <optional>

namespace Linter
{

Report_View::Report_View(HWND list_view) :
    Super(list_view),
    // Technically, we don't need to HAVE a header, but for now not sure
    // how to work out that we don't need one, nor how to get the number of
    // columns without one.
    header_(std::make_unique<Column_Header>(ListView_GetHeader(list_view)))
{
}

Report_View::~Report_View() noexcept = default;

void Report_View::add_column(Data_Column column, Column_Data col) const noexcept
{
    col.subitem = column;
    Super::add_column(col);
}

void Report_View::autosize_columns() const noexcept
{
    int const num_columns = get_num_columns();
    for (int col = 0; col < num_columns; col += 1)
    {
        ListView_SetColumnWidth(handle(), col, LVSCW_AUTOSIZE_USEHEADER);
    }
}

void Report_View::sort_by_column(
    Sort_Callback_Function const &callback
) const noexcept
{
    Super::sort_by_column(
        current_sort_column_, callback, current_sort_direction_
    );
    if (header_)
    {
        header_->set_sort_icon(current_sort_column_, current_sort_direction_);
    }
}

void Report_View::sort_by_column(
    Data_Column column, Sort_Callback_Function const &callback
) const noexcept
{
    if (current_sort_column_ == column)
    {
        // Toggle sort direction
        if (current_sort_direction_ == Sort_Direction::Ascending)
        {
            current_sort_direction_ = Sort_Direction::Descending;
        }
        else if (current_sort_direction_ == Sort_Direction::Descending)
        {
            current_sort_direction_ = Sort_Direction::None;
        }
        else
        {
            current_sort_direction_ = Sort_Direction::Ascending;
        }
    }
    else
    {
        current_sort_column_ = column;
        current_sort_direction_ = Sort_Direction::Ascending;
    }
    sort_by_column(callback);
}

int Report_View::get_num_columns() const noexcept
{
    return header_->get_num_columns();
}

}    // namespace Linter
