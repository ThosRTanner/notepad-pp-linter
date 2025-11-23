#include "List_View.h"

#include "List_View_Types.h"
#include "System_Error.h"

#include "Plugin/Casts.h"

#include <CommCtrl.h>
#include <intsafe.h>
#include <minwindef.h>
#include <windef.h>
#include <winuser.h>

#if __cplusplus >= 202302L
#include <coroutine>
#endif
#include <utility>    //For std::move

namespace Linter
{

List_View::List_View(HWND handle) : handle_(handle)
{
    if (handle_ == nullptr)
    {
        throw ::Linter::System_Error("Could not create list box");
    }

    set_extended_style_flags(LVS_EX_AUTOSIZECOLUMNS);
}

List_View::~List_View() noexcept = default;

// Allow moving
List_View::List_View(List_View &&) noexcept = default;

List_View &List_View::operator=(List_View &&) noexcept = default;

int List_View::add_column(Column_Data const &col) const noexcept
{
    LVCOLUMN lvc{};
    //"fmt" field
    if (col.justification.has_value())
    {
        switch (*col.justification)
        {
            case Column_Data::Justification::Left:
                lvc.fmt = LVCFMT_LEFT;
                break;
            case Column_Data::Justification::Right:
                lvc.fmt = LVCFMT_RIGHT;
                break;
            case Column_Data::Justification::Center:
                lvc.fmt = LVCFMT_CENTER;
                break;
        }
        lvc.mask |= LVCF_FMT;
    }
    if (col.bitmap_on_right)
    {
        lvc.fmt |= LVCFMT_BITMAP_ON_RIGHT;
        lvc.mask |= LVCF_FMT;
    }
    if (col.header_has_images)
    {
        lvc.fmt |= LVCFMT_COL_HAS_IMAGES;
        lvc.mask |= LVCF_FMT;
    }
    if (col.fixed_width)
    {
        lvc.fmt |= LVCFMT_FIXED_WIDTH;
        lvc.mask |= LVCF_FMT;
    }
    if (col.no_dpi_scale)
    {
        lvc.fmt |= LVCFMT_NO_DPI_SCALE;
        lvc.mask |= LVCF_FMT;
    }
    if (col.fixed_aspect_ratio)
    {
        lvc.fmt |= LVCFMT_FIXED_RATIO;
        lvc.mask |= LVCF_FMT;
    }
    if (col.split_button)
    {
        lvc.fmt |= LVCFMT_SPLITBUTTON;
        lvc.mask |= LVCF_FMT;
    }
    // Other fields
    if (col.width.has_value())
    {
        lvc.cx = *col.width;
        lvc.mask |= LVCF_WIDTH;
    }
    if (col.text.has_value())
    {
        lvc.pszText = windows_const_cast<wchar_t *>(col.text->c_str());
        lvc.mask |= LVCF_TEXT;
    }
    if (col.subitem.has_value())
    {
        lvc.iSubItem = *col.subitem;
        lvc.mask |= LVCF_SUBITEM;
    }
    if (col.image.has_value())
    {
        lvc.iImage = *col.image;
        lvc.mask |= LVCF_FMT | LVCF_IMAGE;
        lvc.fmt |= LVCFMT_IMAGE;
    }
    if (col.order.has_value())
    {
        lvc.iOrder = *col.order;
        lvc.mask |= LVCF_ORDER;
    }
    if (col.min_width.has_value())
    {
        lvc.cxMin = *col.min_width;
        lvc.mask |= LVCF_MINWIDTH;
    }
    if (col.default_width.has_value())
    {
        lvc.cxDefault = *col.default_width;
        lvc.mask |= LVCF_DEFAULTWIDTH;
    }

    return ListView_InsertColumn(handle_, lvc.iSubItem, &lvc);
}

// I have no idea why clang-tidy thinks this can throw an exception.
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void List_View::add_row(Data_Row row, Row_Data const &row_data) const
{
    LVITEM lvi{.iItem = row, .iSubItem = row_data.sub_item};
    if (row_data.state.has_value())
    {
        auto const &state = row_data.state.value();
        if (state.activating.has_value() && state.activating.value())
        {
            lvi.state |= LVIS_ACTIVATING;
            lvi.stateMask |= LVIS_ACTIVATING;
        }
        if (state.cut.has_value() && state.cut.value())
        {
            lvi.state |= LVIS_CUT;
            lvi.stateMask |= LVIS_CUT;
        }
        if (state.drop_hilited.has_value() && state.drop_hilited.value())
        {
            lvi.state |= LVIS_DROPHILITED;
            lvi.stateMask |= LVIS_DROPHILITED;
        }
        if (state.focused.has_value() && state.focused.value())
        {
            lvi.state |= LVIS_FOCUSED;
            lvi.stateMask |= LVIS_FOCUSED;
        }
        if (state.selected.has_value() && state.selected.value())
        {
            lvi.state |= LVIS_SELECTED;
            lvi.stateMask |= LVIS_SELECTED;
        }
        if (state.overlay_image.has_value())
        {
            lvi.state |= INDEXTOOVERLAYMASK(state.overlay_image.value());
            lvi.stateMask |= LVIS_OVERLAYMASK;
        }
        if (state.state_image.has_value())
        {
            lvi.state |= INDEXTOSTATEIMAGEMASK(state.state_image.value());
            lvi.stateMask |= LVIS_STATEIMAGEMASK;
        }
        lvi.mask |= LVIF_STATE;
    }
    if (row_data.text.has_value())
    {
        lvi.pszText =
            windows_const_cast<wchar_t *>(row_data.text.value().c_str());
        lvi.mask |= LVIF_TEXT;
    }
    if (row_data.image.has_value())
    {
        lvi.iImage = row_data.image.value();
        lvi.mask |= LVIF_IMAGE;
    }
    if (row_data.user_data.has_value())
    {
        lvi.lParam = row_data.user_data.value();
        lvi.mask |= LVIF_PARAM;
    }
    if (row_data.indent.has_value())
    {
        lvi.iIndent = row_data.indent.value();
        lvi.mask |= LVIF_INDENT;
    }
    if (row_data.group_id.has_value())
    {
        lvi.iGroupId = row_data.group_id.value();
        lvi.mask |= LVIF_GROUPID;
    }
    if (row_data.columns.has_value())
    {
        lvi.cColumns = row_data.columns.value();
        lvi.mask |= LVIF_COLUMNS;
    }
    lvi.puColumns = row_data.columns_array;

    if (row_data.columns_format.has_value())
    {
        lvi.piColFmt = row_data.columns_format.value();
        lvi.mask |= LVIF_COLFMT;
    }

    ListView_InsertItem(handle_, &lvi);
}

void Linter::List_View::ensure_rows(int total_rows) const noexcept
{
    ListView_SetItemCount(handle_, total_rows);
}

void List_View::set_item_text(
    Data_Row row, Data_Column col, std::wstring const &message
) const noexcept
{
    ListView_SetItemText(
        handle_, row, col, windows_const_cast<wchar_t *>(message.c_str())
    );
}

void List_View::clear() const noexcept
{
    ListView_DeleteAllItems(handle_);
}

void List_View::select_all() const noexcept
{
    ListView_SetItemState(handle_, -1, LVIS_SELECTED, LVIS_SELECTED);
}

List_View::Data_Row List_View::move_selection(int rows) const noexcept
{
    int const count = get_num_rows();

    int row = get_first_selected_item() + rows;
    row = (row % count + count) % count;

    // Deselect all first
    ListView_SetItemState(handle_, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);

    // Select the specified item
    ListView_SetItemState(
        handle_, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED
    );

    // Ensure it's visible
    ListView_EnsureVisible(handle_, row, FALSE);

    return get_index(row);
}

int List_View::num_selected_items() const noexcept
{
    return ListView_GetSelectedCount(handle_);
}

int List_View::get_num_rows() const noexcept
{
    return ListView_GetItemCount(handle_);
}

List_View::Data_Row List_View::get_first_selected_index() const noexcept
{
    auto const item = get_first_selected_item();
    if (item == -1)
    {
        return item;
    }
    return get_index(item);
}

List_View::Data_Row List_View::get_index(int item) const noexcept
{
    LVITEM const lvitem{.mask = LVIF_PARAM, .iItem = item};
    ListView_GetItem(handle_, &lvitem);
#pragma warning(suppress : 26472)
    return static_cast<Data_Row>(lvitem.lParam);
}

#if __cplusplus >= 202302L
std::generator<List_View::Data_Row> List_View::selected_items() const noexcept
{
    int item = -1;
    while ((item = ListView_GetNextItem(handle_, item, LVNI_SELECTED)) != -1)
    {
        co_yield get_index(item);
    }
}
#else
std::vector<List_View::Data_Row> List_View::selected_items() const noexcept
{
    std::vector<Data_Row> selected;
    int item = -1;
    while ((item = ListView_GetNextItem(handle_, item, LVNI_SELECTED)) != -1)
    {
        selected.push_back(get_index(item));
    }
    return selected;
}
#endif

void List_View::show() const noexcept
{
    ShowWindow(handle_, SW_SHOW);
}

void List_View::hide() const noexcept
{
    ShowWindow(handle_, SW_HIDE);
}

void List_View::sort_by_column(
    Data_Column col, Sort_Callback_Function callback_fn,
    Sort_Direction direction
) const noexcept
{
    struct Details
    {
        Data_Column column;
        Sort_Callback_Function callback;
        int direction;
    } const details{
        .column = col,
        // This has to be a bug. callback_fn is not accessed after this move
        .callback = std::move(callback_fn),    // NOLINT
        .direction = direction == Sort_Direction::None ? 0
            : direction == Sort_Direction::Ascending   ? 1
                                                       : -1
    };
    auto c_callback =
        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        [](LPARAM param1, LPARAM param2, LPARAM details) noexcept -> int
    {
        Details const &params =
            *windows_cast_to<Details const *, LPARAM>(details);
        if (params.direction == 0)
        {
            return windows_static_cast<int, LPARAM>(param1 - param2);
        }
#ifndef __cpp_lib_copyable_function
#pragma warning(suppress : 26447)
#endif
        return params.direction
            * params.callback(param1, param2, params.column);
    };

    ListView_SortItems(handle_, c_callback, &details);
}

void List_View::get_screen_coordinates(POINT *point) const noexcept
{
    ClientToScreen(handle_, point);
}

void List_View::set_window_position(
    HWND prev_win, RECT const &rect
) const noexcept
{
    ::SetWindowPos(
        handle_,
        prev_win,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        0
    );
}

/** Manipulate flags */
DWORD List_View::modify_extended_style_flags(
    DWORD mask, DWORD style
) const noexcept
{
    return ListView_SetExtendedListViewStyleEx(handle_, mask, style);
}

int List_View::get_first_selected_item() const noexcept
{
    return ListView_GetNextItem(handle_, -1, LVNI_FOCUSED | LVNI_SELECTED);
}

}    // namespace Linter
