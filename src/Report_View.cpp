#include "Report_View.h"

#include <optional>

namespace Linter
{

Report_View::Report_View(HWND list_view) : Super(list_view)
{
}

void Report_View::add_column(Data_Column column, Column_Data col) const noexcept
{
    col.subitem = column;
    Super::add_column(col);
}

}    // namespace Linter
