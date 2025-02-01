#include "Output_Dialogue.h"

#include "Plugin/Plugin.h"

#include "Checkstyle_Parser.h"
#include "Clipboard.h"
#include "Linter.h"
#include "System_Error.h"

#include "resource.h"

#include <CommCtrl.h>
#include <intsafe.h>

#include <cstddef>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Linter
{

namespace
{
/** A slightly more explicit version of reinterpret_cast which
 * requires both types to be specified, and disables the cast warning.
 */
template <typename Target_Type, typename Orig_Type>
Target_Type cast_to(Orig_Type val) noexcept
{
#pragma warning(suppress : 26490)
    return reinterpret_cast<Target_Type>(val);
}

/** A similar cast to wrap const_cast without the warnings.
 * The windows APIs leave a lot to be desired in the area of const correctness.
 */
template <typename Orig_Type>
Orig_Type windows_const_cast(std::remove_pointer_t<Orig_Type> const *val
) noexcept
{
#pragma warning(suppress : 26492)
    return const_cast<Orig_Type>(val);
}

/** Columns in the error list */
enum List_Column
{
    Column_Line,
    Column_Position,
    Column_Tool,
    Column_Message
};

/** Context menu items
 * Ensure they don't clash with anything in the resource file.
 */
enum Context_Menu_Entry
{
    Context_Copy_Lints = 1500,
    Context_Show_Source_Line,
    Context_Select_All
};

}    // namespace

Output_Dialogue::Output_Dialogue(int menu_entry, Linter const &plugin) :
    Super(IDD_OUTPUT, plugin),
    tab_bar_(GetDlgItem(IDC_TABBAR)),
    tab_definitions_({
        TabDefinition{L"Lint Errors",   IDC_LIST_LINTS,  Lint_Error,   *this},
        TabDefinition{L"System Errors", IDC_LIST_OUTPUT, System_Error, *this}
}),
    current_tab_(&tab_definitions_.at(0))
{
    initialise_dialogue();
    for (auto &tab : tab_definitions_)
    {
        initialise_tab(tab);
    }
    selected_tab_changed();

    // Possibly one should free the icon up, but I don't see the dialogue memory
    // being freed up anywhere.
    register_dialogue(
        menu_entry,
        Position::Dock_Bottom,
        static_cast<HICON>(::LoadImage(
            plugin.module(),
            MAKEINTRESOURCE(IDI_ICON1),
            IMAGE_ICON,
            0,
            0,
            LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT
        ))
    );
}

Output_Dialogue::~Output_Dialogue()
{
}

void Output_Dialogue::display() noexcept
{
    Super::display();
    ::SetFocus(tab_bar_);
}

void Output_Dialogue::clear_lint_info()
{
    for (auto &tab : tab_definitions_)
    {
        ListView_DeleteAllItems(tab.list_view);
        tab.errors.clear();
    }

    update_displayed_counts();
}

void Output_Dialogue::add_system_error(Checkstyle_Parser::Error const &err)
{
    std::vector<Checkstyle_Parser::Error> errs;
    errs.push_back(err);
    add_errors(Tab::System_Error, errs);
}

void Output_Dialogue::add_lint_errors(
    std::vector<Checkstyle_Parser::Error> const &errs
)
{
    add_errors(Tab::Lint_Error, errs);
}

void Output_Dialogue::select_next_lint() noexcept
{
    select_lint(1);
}

void Output_Dialogue::select_previous_lint() noexcept
{
    select_lint(-1);
}

Output_Dialogue::Message_Return Output_Dialogue::on_dialogue_message(
    UINT message, WPARAM wParam, LPARAM lParam
)
{
    switch (message)
    {
        case WM_COMMAND:
            return process_dlg_command(wParam);

        case WM_CONTEXTMENU:
            return process_dlg_context_menu(lParam);

        case WM_NOTIFY:
            return process_dlg_notify(lParam);

        case WM_WINDOWPOSCHANGED:
            // This must return as if it was unhandled
            window_pos_changed();
            return std::nullopt;

        default:
            break;
    }

    return std::nullopt;
}

void Output_Dialogue::initialise_dialogue() noexcept
{
    TCITEM tie{.mask = TCIF_TEXT | TCIF_IMAGE, .iImage = -1};

    for (auto const &tab : tab_definitions_)
    {
        tie.pszText = windows_const_cast<wchar_t *>(tab.tab_name);
        TabCtrl_InsertItem(tab_bar_, tab.tab, &tie);
    }
}

void Output_Dialogue::initialise_tab(TabDefinition &tab) noexcept
{
    HWND const list_view = tab.list_view;

    ListView_SetExtendedListViewStyle(
        list_view, LVS_EX_FULLROWSELECT | LVS_EX_AUTOSIZECOLUMNS
    );

    LVCOLUMN lvc{};
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    lvc.iSubItem = Column_Tool;
    lvc.pszText = windows_const_cast<wchar_t *>(L"Tool");
    lvc.cx = 100;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(list_view, lvc.iSubItem, &lvc);

    lvc.iSubItem = Column_Line;
    lvc.pszText = windows_const_cast<wchar_t *>(L"Line");
    lvc.cx = 50;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(list_view, lvc.iSubItem, &lvc);

    lvc.iSubItem = Column_Position;
    lvc.pszText = windows_const_cast<wchar_t *>(L"Col");
    lvc.cx = 50;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(list_view, lvc.iSubItem, &lvc);

    lvc.iSubItem = Column_Message;
    lvc.pszText = windows_const_cast<wchar_t *>(L"Reason");
    lvc.cx = 500;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(list_view, lvc.iSubItem, &lvc);
}

std::optional<LONG_PTR> Output_Dialogue::process_dlg_command(WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
        case Context_Copy_Lints:
            copy_to_clipboard();
            return TRUE;

        case Context_Show_Source_Line:
        {
            int const item = ListView_GetNextItem(
                current_list_view_, -1, LVIS_FOCUSED | LVIS_SELECTED
            );
            if (item != -1)
            {
                show_selected_lint(item);
            }
            return TRUE;
        }

        case Context_Select_All:
            ListView_SetItemState(
                current_list_view_, -1, LVIS_SELECTED, LVIS_SELECTED
            );
            return TRUE;

        default:
            break;
    }
    return std::nullopt;
}

std::optional<LONG_PTR> Output_Dialogue::process_dlg_context_menu(LPARAM lParam
) noexcept
{
    // Right click in docked window.
    // build context menu
    HMENU menu = ::CreatePopupMenu();

    int const numSelected = ListView_GetSelectedCount(current_list_view_);

    if (numSelected >= 1)
    {
        int const iFocused = ListView_GetNextItem(
            current_list_view_, -1, LVIS_FOCUSED | LVIS_SELECTED
        );
        if (iFocused != -1)
        {
            AppendMenu(menu, MF_ENABLED, Context_Show_Source_Line, L"Show");
        }
    }

    if (GetMenuItemCount(menu) > 0)
    {
        AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
    }

    if (numSelected > 0)
    {
        AppendMenu(menu, MF_ENABLED, Context_Copy_Lints, L"Copy");
    }

    AppendMenu(menu, MF_ENABLED, Context_Select_All, L"Select All");

    // determine context menu position
    POINT point{.x = LOWORD(lParam), .y = HIWORD(lParam)};
    if (point.x == 65535 || point.y == 65535)
    {
        point.x = 0;
        point.y = 0;
        ClientToScreen(current_list_view_, &point);
    }

    // show context menu
    TrackPopupMenu(menu, 0, point.x, point.y, 0, window(), nullptr);
    return TRUE;
}

std::optional<LONG_PTR> Output_Dialogue::process_dlg_notify(LPARAM lParam)
{
    auto const notify_header = cast_to<NMHDR const *, LPARAM>(lParam);
    switch (notify_header->code)
    {
        case LVN_KEYDOWN:
            if (notify_header->idFrom == current_tab_->list_view_id)
            {
                NMLVKEYDOWN const *pnkd =
                    cast_to<LPNMLVKEYDOWN, LPARAM>(lParam);
                if (pnkd->wVKey == 'A'
                    && (::GetKeyState(VK_CONTROL) & 0x8000U) != 0)
                {
                    ListView_SetItemState(
                        current_list_view_, -1, LVIS_SELECTED, LVIS_SELECTED
                    );
                    return TRUE;
                }
                else if (pnkd->wVKey == 'C'
                         && (::GetKeyState(VK_CONTROL) & 0x8000U) != 0)
                {
                    copy_to_clipboard();
                    return TRUE;
                }
            }
            break;

        case NM_DBLCLK:
            if (notify_header->idFrom == current_tab_->list_view_id)
            {
                auto const item_activate =
                    cast_to<NMITEMACTIVATE const *, LPARAM>(lParam);
                int const selected_item = item_activate->iItem;
                if (selected_item != -1)
                {
                    show_selected_lint(selected_item);
                }
                return TRUE;
            }
            break;

        case NM_CUSTOMDRAW:
            if (notify_header->idFrom == current_tab_->list_view_id)
            {
                return process_custom_draw(
                    cast_to<NMLVCUSTOMDRAW *, LPARAM>(lParam)
                );
            }
            break;

        case TCN_SELCHANGE:
            if (notify_header->idFrom == IDC_TABBAR)
            {
                selected_tab_changed();
                return TRUE;
            }
            break;

        default:
            break;
    }
    return std::nullopt;
}

std::optional<LONG_PTR> Output_Dialogue::process_custom_draw(
    NMLVCUSTOMDRAW *custom_draw
) noexcept
{
    switch (custom_draw->nmcd.dwDrawStage)
    {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;

        case CDDS_ITEMPREPAINT:
            current_item_ = static_cast<int>(custom_draw->nmcd.dwItemSpec);
            return CDRF_NOTIFYSUBITEMDRAW;

        case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
            if (custom_draw->iSubItem == Column_Message)
            {
                LVITEM const item{.mask = LVIF_PARAM, .iItem = current_item_};
                ListView_GetItem(custom_draw->nmcd.hdr.hwndFrom, &item);

                if (static_cast<std::size_t>(item.lParam)
                    >= current_tab_->errors.size())
                {
                    // For reasons I don't entirely understand, windows paints
                    // an entry for a line that doesn't exist. So don't do
                    // anything for that.
                    break;
                }

                // Now we colour the text according to the severity level.
                auto const &lint_error = current_tab_->errors[item.lParam];
                if (lint_error.severity_ == L"warning")
                {
                    custom_draw->clrText = RGB(255, 127, 0);    // Orange
                }
                else if (lint_error.severity_ == L"error")
                {
                    custom_draw->clrText = RGB(255, 0, 0);    // Red
                }
                else
                {
                    custom_draw->clrText = RGB(0, 0, 0);    // Black
                }

                // Tell Windows to paint the control itself.
                return CDRF_DODEFAULT;
            }
            break;

        default:
            break;
    }
    return std::nullopt;
}

void Output_Dialogue::selected_tab_changed() noexcept
{
    current_tab_ = &tab_definitions_[TabCtrl_GetCurSel(tab_bar_)];
    current_list_view_ = current_tab_->list_view;
    for (auto const &tab : tab_definitions_)
    {
        ShowWindow(tab.list_view, &tab == current_tab_ ? SW_SHOW : SW_HIDE);
    }
}

void Output_Dialogue::window_pos_changed() noexcept
{
    RECT const rc = getClientRect();

    ::MoveWindow(
        tab_bar_, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE
    );

    TabCtrl_AdjustRect(tab_bar_, FALSE, &rc);
    for (auto const &tab : tab_definitions_)
    {
        ::SetWindowPos(
            tab.list_view,
            tab_bar_,
            rc.left,
            rc.top,
            rc.right - rc.left,
            rc.bottom - rc.top,
            0
        );
        ListView_SetColumnWidth(tab.list_view, Column_Message, LVSCW_AUTOSIZE);
    }
}

void Output_Dialogue::update_displayed_counts()
{
    for (auto const &tab : tab_definitions_)
    {
        std::wstring strTabName;
        int const count = ListView_GetItemCount(tab.list_view);
        if (count > 0)
        {
            std::wstringstream stream;
            stream << tab.tab_name << L" (" << count << L")";
            strTabName = stream.str();
        }
        else
        {
            strTabName = tab.tab_name;
        }

        TCITEM const tie{
            .mask = TCIF_TEXT,
            .pszText = windows_const_cast<wchar_t *>(strTabName.c_str())
        };
        TabCtrl_SetItem(tab_bar_, tab.tab, &tie);
    }
}

void Output_Dialogue::add_errors(
    Tab tab, std::vector<Checkstyle_Parser::Error> const &lints
)
{
    auto &tab_def = tab_definitions_[tab];
    HWND const list_view = tab_def.list_view;

    std::wstringstream stream;
    for (auto const &lint : lints)
    {
        auto const item = ListView_GetItemCount(list_view);

        LVITEM const lvI{
            .mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM,
            .iItem = item,
            .pszText = windows_const_cast<wchar_t *>(L""),
            .lParam = item
        };
        ListView_InsertItem(list_view, &lvI);

        ListView_SetItemText(
            list_view,
            item,
            Column_Message,
            windows_const_cast<wchar_t *>(lint.message_.c_str())
        );

        std::wstring strFile = lint.tool_;
        ListView_SetItemText(
            list_view,
            item,
            Column_Tool,
            windows_const_cast<wchar_t *>(strFile.c_str())
        );

        stream.str(L"");
        stream << lint.line_;
        std::wstring strLine = stream.str();
        ListView_SetItemText(
            list_view,
            item,
            Column_Line,
            windows_const_cast<wchar_t *>(strLine.c_str())
        );

        stream.str(L"");
        stream << lint.column_;
        std::wstring strColumn = stream.str();
        ListView_SetItemText(
            list_view,
            item,
            Column_Position,
            windows_const_cast<wchar_t *>(strColumn.c_str())
        );

        // Ensure the message column is as wide as the widest column.
        ListView_SetColumnWidth(
            list_view, Column_Message, LVSCW_AUTOSIZE_USEHEADER
        );

        tab_def.errors.push_back(lint);
    }

    update_displayed_counts();

    // Not sure we should do this every time we add something, but...
    // Also allow user to sort differently and remember?
    ListView_SortItemsEx(list_view, sort_call_function, &tab_def.errors);
    InvalidateRect();
}

void Output_Dialogue::select_lint(int n) noexcept
{
    int const count = ListView_GetItemCount(current_list_view_);
    if (count == 0)
    {
        // no lints, set focus to editor
        ::SetFocus(plugin()->get_scintilla_window());
        return;
    }

    int row = ListView_GetNextItem(
                  current_list_view_, -1, LVNI_FOCUSED | LVNI_SELECTED
              )
        + n;
    row = (row % count + count) % count;

    // Unselect all the rows.
    ListView_SetItemState(
        current_list_view_, -1, 0, LVIS_SELECTED | LVIS_FOCUSED
    );

    // Select the newly calculated row.
    ListView_SetItemState(
        current_list_view_,
        row,
        LVIS_SELECTED | LVIS_FOCUSED,
        LVIS_SELECTED | LVIS_FOCUSED
    );
    ListView_EnsureVisible(current_list_view_, row, FALSE);

    // And select the error location in the editor window
    show_selected_lint(row);
}

void Output_Dialogue::show_selected_lint(int selected_item) noexcept
{
    LVITEM const item{.mask = LVIF_PARAM, .iItem = selected_item};
    ListView_GetItem(current_list_view_, &item);

    Checkstyle_Parser::Error const &lint_error =
        current_tab_->errors[item.lParam];

    int const line = std::max(lint_error.line_ - 1, 0);
    int const column = std::max(lint_error.column_ - 1, 0);

    /* We only need to do this if we need to pop up linter.xml. The following
     * isn't ideal */
    if (current_tab_->tab == Tab::System_Error)
    {
        // FIXME
        //  editConfig();
    }

    plugin()->send_to_editor(SCI_GOTOLINE, line);

    // since there is no SCI_GOTOCOLUMN, we move to the right until ...
    for (;;)
    {
        plugin()->send_to_editor(SCI_CHARRIGHT);
        LRESULT const curPos = plugin()->send_to_editor(SCI_GETCURRENTPOS);
        LRESULT const curLine =
            plugin()->send_to_editor(SCI_LINEFROMPOSITION, curPos);
        if (curLine > line)
        {
            // ... current line is greater than desired line or ...
            plugin()->send_to_editor(SCI_CHARLEFT);
            break;
        }

        LRESULT const curCol = plugin()->send_to_editor(SCI_GETCOLUMN, curPos);
        if (curCol > column)
        {
            // ... current column is greater than desired column or ...
            plugin()->send_to_editor(SCI_CHARLEFT);
            break;
        }

        if (curCol == column)
        {
            // ... we reached desired column.
            break;
        }
    }

    InvalidateRect();
}

void Output_Dialogue::copy_to_clipboard()
{
    std::wstringstream stream;

    bool first = true;
    int row = ListView_GetNextItem(current_list_view_, -1, LVNI_SELECTED);
    while (row != -1)
    {
        // Get the actual item for the row
        LVITEM const item{.mask = LVIF_PARAM, .iItem = row};
        ListView_GetItem(current_list_view_, &item);

        auto const &lint_error = current_tab_->errors[item.lParam];

        if (first)
        {
            first = false;
        }
        else
        {
            stream << L"\r\n";
        }

        stream << L"Line " << lint_error.line_ << L", column "
               << lint_error.column_ << L": " << L"\r\n\t"
               << lint_error.message_ << L"\r\n";

        row = ListView_GetNextItem(current_list_view_, row, LVNI_SELECTED);
    }

    std::wstring const str = stream.str();
    if (str.empty())
    {
        return;
    }

    Clipboard clipboard{window()};

    clipboard.empty();
    clipboard.copy(str);
}

int CALLBACK Output_Dialogue::sort_call_function(
    LPARAM row1_index, LPARAM row2_index, LPARAM lParamSort
) noexcept
{
    // FIXME pass pointer to appropriate errors
    auto const &errs =
        *cast_to<std::vector<Checkstyle_Parser::Error> const *, LPARAM>(
            lParamSort
        );
    int res = errs[row1_index].line_ - errs[row2_index].line_;
    if (res == 0)
    {
        res = errs[row1_index].column_ - errs[row2_index].column_;
    }
    return res;
}

Output_Dialogue::TabDefinition::TabDefinition(
    wchar_t const *name, UINT id, Tab tab, Output_Dialogue const &parent
) :
    tab_name(name),
    list_view_id(id),
    tab(tab),
    list_view(parent.GetDlgItem(id))
{
    if (list_view == nullptr)
    {
        throw ::Linter::System_Error("Could not create list box");
    }
}

}    // namespace Linter
