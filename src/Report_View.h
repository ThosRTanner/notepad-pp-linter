#pragma once

#include "List_View.h"
#include "List_View_Types.h"

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

    /* Add a column.
     *
     * You must specify which column this is.
     * NB For a report view, the first column is always left justified.
     */
    void add_column(Data_Column, Column_Data) const noexcept;

    // Like I said, you MUST specify which column this is.
    int add_column(Column_Data const &) const noexcept = delete;

    /* Set all columns to autosize based on header text.
     *
     * Note the last column will fill the remaining space.
     */
    void autosize_columns() const noexcept;

    /* Sort by column using the last used column and direction */
    void sort_by_column(Sort_Callback_Function const &) const noexcept;

    /* Sort by column, toggling direction if same column as last time */
    void sort_by_column(
        Data_Column, Sort_Callback_Function const &
    ) const noexcept;

    /** Total number of columns */
    int get_num_columns() const noexcept;

  private:
    std::unique_ptr<Column_Header> header_;
    // Initial column and direction for sorting
    mutable Data_Column current_sort_column_{0};
    mutable Sort_Direction current_sort_direction_{Sort_Direction::Ascending};
};

}    // namespace Linter
