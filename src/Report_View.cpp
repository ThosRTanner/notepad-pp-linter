#include "Report_View.h"

#include "Column_Header.h"

#include <CommCtrl.h>

#include <optional>

namespace Linter
{

Report_View::Report_View(HWND list_view) :
    //Technically, we don't need to HAVE a header, but for now not sure
    // how to work out that we don't need one.
    header_(std::make_unique<Column_Header>(ListView_GetHeader(list_view))),
    Super(list_view)
{
}

Report_View::~Report_View() noexcept = default;

void Report_View::add_column(Data_Column column, Column_Data col) const noexcept
{
    col.subitem = column;
    Super::add_column(col);
}

void Report_View::sort_by_column(
    Data_Column column, Sort_Callback_Function const &callback
) const noexcept
{
    Super::sort_by_column(column, callback);
    if (header_)
    {
        header_->set_sort_icon(
            column, Column_Header::Sort_Ascending
        );
    }
}

}    // namespace Linter
