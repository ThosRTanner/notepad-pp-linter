#include "stdafx.h"
#include "OutputDialog.h"

#include "Clipboard.h"
#include "SystemError.h"
#include "plugin_main.h"
#include "resource.h"

#include <CommCtrl.h>
#include <WinUser.h>

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>

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

Linter::OutputDialog::OutputDialog(HINSTANCE module_handle, HWND npp_win, int dlg_num)
    : DockingDlgInterface(IDD_OUTPUT, module_handle, npp_win),
      tab_bar_(GetDlgItem(IDC_TABBAR)),
      current_tab_(&tab_definitions_.at(0)),
      tab_definitions_({
          TabDefinition{L"Lint Errors",   IDC_LIST_LINTS,  Lint_Error,   *this},
          TabDefinition{L"System Errors", IDC_LIST_OUTPUT, System_Error, *this}
})
{
    initialise_dialogue();
    for (auto &tab : tab_definitions_)
    {
        initialise_tab(tab);
    }
    selected_tab_changed();

    register_dialogue(dlg_num,
        Position::Dock_Bottom,
        static_cast<HICON>(
            ::LoadImage(module_handle, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT)));
}

Linter::OutputDialog::~OutputDialog()
{
}

void Linter::OutputDialog::display() noexcept
{
    DockingDlgInterface::display();
    ::SetFocus(tab_bar_);
}

void Linter::OutputDialog::clear_lint_info()
{
    for (auto &tab : tab_definitions_)
    {
        ListView_DeleteAllItems(tab.list_view);
        tab.errors.clear();
    }

    update_displayed_counts();
}

void Linter::OutputDialog::add_system_error(XmlParser::Error const &err)
{
    std::vector<XmlParser::Error> errs;
    errs.push_back(err);
    add_errors(Tab::System_Error, errs);
}

void Linter::OutputDialog::add_lint_errors(std::vector<XmlParser::Error> const &errs)
{
    add_errors(Tab::Lint_Error, errs);
}

void Linter::OutputDialog::select_next_lint() noexcept
{
    select_lint(1);
}

void Linter::OutputDialog::select_previous_lint() noexcept
{
    select_lint(-1);
}

std::optional<LONG_PTR> Linter::OutputDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    std::optional<LONG_PTR> result;
    switch (message)
    {
        case WM_COMMAND:
            result = process_dlg_command(wParam);
            break;

        case WM_CONTEXTMENU:
            result = process_dlg_context_menu(lParam);
            break;

        case WM_NOTIFY:
            result = process_dlg_notify(lParam);
            break;

        case WM_WINDOWPOSCHANGED:
            //This must return as if it was unhandled
            window_pos_changed();
            return std::nullopt;

        default:
            break;
    }

    return result ? result : DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

void Linter::OutputDialog::initialise_dialogue() noexcept
{
    TCITEM tie{};
    tie.mask = TCIF_TEXT | TCIF_IMAGE;
    tie.iImage = -1;

    for (auto const &tab : tab_definitions_)
    {
        tie.pszText = const_cast<wchar_t *>(tab.tab_name);
        TabCtrl_InsertItem(tab_bar_, tab.tab, &tie);
    }
}

void Linter::OutputDialog::initialise_tab(TabDefinition &tab) noexcept
{
    HWND const list_view = tab.list_view;

    ListView_SetExtendedListViewStyle(list_view, LVS_EX_FULLROWSELECT | LVS_EX_AUTOSIZECOLUMNS);

    LVCOLUMN lvc{};
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    lvc.iSubItem = Column_Tool;
    lvc.pszText = const_cast<wchar_t *>(L"Tool");
    lvc.cx = 100;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(list_view, lvc.iSubItem, &lvc);

    lvc.iSubItem = Column_Line;
    lvc.pszText = const_cast<wchar_t *>(L"Line");
    lvc.cx = 50;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(list_view, lvc.iSubItem, &lvc);

    lvc.iSubItem = Column_Position;
    lvc.pszText = const_cast<wchar_t *>(L"Col");
    lvc.cx = 50;
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(list_view, lvc.iSubItem, &lvc);

    lvc.iSubItem = Column_Message;
    lvc.pszText = const_cast<wchar_t *>(L"Reason");
    lvc.cx = 500;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(list_view, lvc.iSubItem, &lvc);
}

std::optional<LONG_PTR> Linter::OutputDialog::process_dlg_command(WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
        case Context_Copy_Lints:
            copy_to_clipboard();
            return TRUE;

        case Context_Show_Source_Line:
        {
            int const item = ListView_GetNextItem(current_list_view_, -1, LVIS_FOCUSED | LVIS_SELECTED);
            if (item != -1)
            {
                show_selected_lint(item);
            }
            return TRUE;
        }

        case Context_Select_All:
            ListView_SetItemState(current_list_view_, -1, LVIS_SELECTED, LVIS_SELECTED);
            return TRUE;

        default:
            break;
    }
    return std::nullopt;
}

std::optional<LONG_PTR> Linter::OutputDialog::process_dlg_context_menu(LPARAM lParam) noexcept
{
    // Right click in docked window.
    // build context menu
    HMENU menu = ::CreatePopupMenu();

    int const numSelected = ListView_GetSelectedCount(current_list_view_);

    if (numSelected >= 1)
    {
        int const iFocused = ListView_GetNextItem(current_list_view_, -1, LVIS_FOCUSED | LVIS_SELECTED);
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
#if __cplusplus >= 202002L
    POINT point{.x = LOWORD(lParam), .y = HIWORD(lParam)};
#else
    POINT point{LOWORD(lParam), HIWORD(lParam)};
#endif
    if (point.x == 65535 || point.y == 65535)
    {
        point.x = 0;
        point.y = 0;
        ClientToScreen(current_list_view_, &point);
    }

    // show context menu
    TrackPopupMenu(menu, 0, point.x, point.y, 0, get_handle(), nullptr);
    return TRUE;
}

std::optional<LONG_PTR> Linter::OutputDialog::process_dlg_notify(LPARAM lParam)
{
    auto const notify_header = cast_to<NMHDR const *, LPARAM>(lParam);
    switch (notify_header->code)
    {
        case LVN_KEYDOWN:
            if (notify_header->idFrom == current_tab_->list_view_id)
            {
                NMLVKEYDOWN const *pnkd = cast_to<LPNMLVKEYDOWN, LPARAM>(lParam);
                if (pnkd->wVKey == 'A' && (::GetKeyState(VK_CONTROL) & 0x8000U) != 0)
                {
                    ListView_SetItemState(current_list_view_, -1, LVIS_SELECTED, LVIS_SELECTED);
                    return TRUE;
                }
                else if (pnkd->wVKey == 'C' && (::GetKeyState(VK_CONTROL) & 0x8000U) != 0)
                {
                    copy_to_clipboard();
                    return TRUE;
                }
            }
            break;

        case NM_DBLCLK:
            if (notify_header->idFrom == current_tab_->list_view_id)
            {
                auto const item_activate = cast_to<NMITEMACTIVATE const *, LPARAM>(lParam);
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
                return process_custom_draw(cast_to<NMLVCUSTOMDRAW *, LPARAM>(lParam));
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

std::optional<LONG_PTR> Linter::OutputDialog::process_custom_draw(NMLVCUSTOMDRAW *custom_draw) noexcept
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
#if __cplusplus >= 202002L
                LVITEM const item{.mask = LVIF_PARAM, .iItem = current_item_};
#else
                LVITEM item{};
                item.iItem = current_item_;
                item.mask = LVIF_PARAM;
#endif
                ListView_GetItem(custom_draw->nmcd.hdr.hwndFrom, &item);

                if (static_cast<std::size_t>(item.lParam) >= current_tab_->errors.size())
                {
                    //For reasons I don't entirely understand, windows paints an entry for
                    //a line that doesn't exist. So don't do anything for that.
                    break;
                }

                // Now we colour the text according to the severity level.
                auto const &lint_error = current_tab_->errors[item.lParam];
                if (lint_error.m_severity == L"warning")
                {
                    custom_draw->clrText = RGB(255, 127, 0);    //Orange
                }
                else if (lint_error.m_severity == L"error")
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

void Linter::OutputDialog::selected_tab_changed() noexcept
{
    current_tab_ = &tab_definitions_[TabCtrl_GetCurSel(tab_bar_)];
    current_list_view_ = current_tab_->list_view;
    for (auto const &tab : tab_definitions_)
    {
        ShowWindow(tab.list_view, &tab == current_tab_ ? SW_SHOW : SW_HIDE);
    }
}

void Linter::OutputDialog::window_pos_changed() noexcept
{
    RECT const rc = getClientRect();

    ::MoveWindow(tab_bar_, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

    TabCtrl_AdjustRect(tab_bar_, FALSE, &rc);
    for (auto const &tab : tab_definitions_)
    {
        ::SetWindowPos(tab.list_view, tab_bar_, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
        ListView_SetColumnWidth(tab.list_view, Column_Message, LVSCW_AUTOSIZE);
    }
}

void Linter::OutputDialog::update_displayed_counts()
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

#if __cplusplus >= 202002L
        TCITEM const tie{.mask = TCIF_TEXT, .pszText = const_cast<wchar_t *>(strTabName.c_str())};
#else
        TCITEM tie{};
        tie.mask = TCIF_TEXT;
        tie.pszText = const_cast<wchar_t *>(strTabName.c_str());
#endif
        TabCtrl_SetItem(tab_bar_, tab.tab, &tie);
    }
}

void Linter::OutputDialog::add_errors(Tab tab, std::vector<XmlParser::Error> const &lints)
{
    auto &tab_def = tab_definitions_[tab];
    HWND const list_view = tab_def.list_view;

    std::wstringstream stream;
    for (auto const &lint : lints)
    {
        auto const item = ListView_GetItemCount(list_view);

#if __cplusplus >= 202002L
        LVITEM const lvI{.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM, .iItem = item, .pszText = const_cast<wchar_t *>(L""), .lParam = item};
#else
        LVITEM lvI{};
        lvI.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
        lvI.iItem = item;
        lvI.pszText = const_cast<wchar_t *>(L"");
        lvI.lParam = item;
#endif
        ListView_InsertItem(list_view, &lvI);

        ListView_SetItemText(list_view, item, Column_Message, const_cast<wchar_t *>(lint.m_message.c_str()));

        std::wstring strFile = lint.m_tool;
        ListView_SetItemText(list_view, item, Column_Tool, const_cast<wchar_t *>(strFile.c_str()));

        stream.str(L"");
        stream << lint.m_line;
        std::wstring strLine = stream.str();
        ListView_SetItemText(list_view, item, Column_Line, const_cast<wchar_t *>(strLine.c_str()));

        stream.str(L"");
        stream << lint.m_column;
        std::wstring strColumn = stream.str();
        ListView_SetItemText(list_view, item, Column_Position, const_cast<wchar_t *>(strColumn.c_str()));

        //Ensure the message column is as wide as the widest column.
        ListView_SetColumnWidth(list_view, Column_Message, LVSCW_AUTOSIZE_USEHEADER);

        tab_def.errors.push_back(lint);
    }

    update_displayed_counts();

    //Not sure we should do this every time we add something, but...
    //Also allow user to sort differently and remember?
    ListView_SortItemsEx(list_view, sort_call_function, &tab_def.errors);
    request_redraw();
}

void Linter::OutputDialog::select_lint(int n) noexcept
{
    int const count = ListView_GetItemCount(current_list_view_);
    if (count == 0)
    {
        // no lints, set focus to editor
        HWND current_window = getScintillaWindow();
        SetFocus(current_window);
        return;
    }

    int row = ListView_GetNextItem(current_list_view_, -1, LVNI_FOCUSED | LVNI_SELECTED) + n;
    row = (row % count + count) % count;

    //Unselect all the rows.
    ListView_SetItemState(current_list_view_, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);

    //Select the newly calculated row.
    ListView_SetItemState(current_list_view_, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(current_list_view_, row, FALSE);

    //And select the error location in the editor window
    show_selected_lint(row);
}

void Linter::OutputDialog::show_selected_lint(int selected_item) noexcept
{
    LVITEM const item{.mask = LVIF_PARAM, .iItem = selected_item};
    ListView_GetItem(current_list_view_, &item);

    XmlParser::Error const &lint_error = current_tab_->errors[item.lParam];

    int const line = std::max(lint_error.m_line - 1, 0);
    int const column = std::max(lint_error.m_column - 1, 0);

    /* We only need to do this if we need to pop up linter.xml. The following isn't ideal */
    if (current_tab_->tab == Tab::System_Error)
    {
        //FIXME
        // editConfig();
    }

    SendEditor(SCI_GOTOLINE, line);

    // since there is no SCI_GOTOCOLUMN, we move to the right until ...
    for (;;)
    {
        SendEditor(SCI_CHARRIGHT);
        LRESULT const curPos = SendEditor(SCI_GETCURRENTPOS);
        LRESULT const curLine = SendEditor(SCI_LINEFROMPOSITION, curPos);
        if (curLine > line)
        {
            // ... current line is greater than desired line or ...
            SendEditor(SCI_CHARLEFT);
            break;
        }

        LRESULT const curCol = SendEditor(SCI_GETCOLUMN, curPos);
        if (curCol > column)
        {
            // ... current column is greater than desired column or ...
            SendEditor(SCI_CHARLEFT);
            break;
        }

        if (curCol == column)
        {
            // ... we reached desired column.
            break;
        }
    }

    request_redraw();
}

void Linter::OutputDialog::copy_to_clipboard()
{
    std::wstringstream stream;

    bool first = true;
    int row = ListView_GetNextItem(current_list_view_, -1, LVNI_SELECTED);
    while (row != -1)
    {
        //Get the actual item for the row
#if __cplusplus >= 202002L
        LVITEM const item{.mask = LVIF_PARAM, .iItem = row};
#else
        LVITEM item{};
        item.iItem = row;
        item.mask = LVIF_PARAM;
#endif
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

        stream << L"Line " << lint_error.m_line << L", column " << lint_error.m_column << L": " << L"\r\n\t" << lint_error.m_message
               << L"\r\n";

        row = ListView_GetNextItem(current_list_view_, row, LVNI_SELECTED);
    }

    std::wstring const str = stream.str();
    if (str.empty())
    {
        return;
    }

    Clipboard clipboard{get_handle()};

    clipboard.empty();
    clipboard.copy(str);
}

int CALLBACK Linter::OutputDialog::sort_call_function(LPARAM row1_index, LPARAM row2_index, LPARAM lParamSort) noexcept
{
    //FIXME pass pointer to appropriate errors
    auto const &errs = *cast_to<std::vector<XmlParser::Error> const *, LPARAM>(lParamSort);
    int res = errs[row1_index].m_line - errs[row2_index].m_line;
    if (res == 0)
    {
        res = errs[row1_index].m_column - errs[row2_index].m_column;
    }
    return res;
}

Linter::OutputDialog::TabDefinition::TabDefinition(wchar_t const *name, UINT id, Tab tab, OutputDialog const &parent)
    : tab_name(name), list_view_id(id), tab(tab), list_view(parent.GetDlgItem(id))
{
    if (list_view == nullptr)
    {
        throw Linter::SystemError("Could not create list box");
    }
}
