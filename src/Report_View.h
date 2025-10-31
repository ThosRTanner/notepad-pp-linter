#pragma once
#include "List_View.h"

#include <windef.h>    // for HWND

#include <memory>

namespace Linter
{

class Column_Header;

class Report_View : public List_View
{
    typedef List_View Super;

  public:
    Report_View(HWND list_view);

    ~Report_View() noexcept override;

    // Add a column. You must specify which column this is.
    // NB For a report view, the first column is always left justified.
    void add_column(Data_Column, Column_Data) const noexcept;

    // Like I said, you MUST specify which column this is.
    int add_column(Column_Data const &) const noexcept = delete;

    void sort_by_column(
        Data_Column, Sort_Callback_Function const &
    ) const noexcept override;

 private:
    std::unique_ptr<Column_Header> header_;
};

}    // namespace Linter
