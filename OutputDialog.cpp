#include "stdafx.h"
#include "OutputDialog.h"

#include "plugin.h"
#include "resource.h"
#include "SystemError.h"

#include <CommCtrl.h>

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

std::array<Linter::OutputDialog::TabDefinition, Linter::OutputDialog::Num_Tabs> const Linter::OutputDialog::tab_definitions_ = {
    Linter::OutputDialog::TabDefinition{L"Lint Errors",   IDC_LIST_LINTS },
    Linter::OutputDialog::TabDefinition{L"System Errors", IDC_LIST_OUTPUT}
};

////////////////////////////////////////////////////////////////////////////////

Linter::OutputDialog::OutputDialog(HANDLE module_handle, HWND npp_win, int dlg_num)
    : DockingDlgInterface(IDD_OUTPUT, static_cast<HINSTANCE>(module_handle), npp_win), tab_bar_()
{
    list_views_.fill(static_cast<HWND>(nullptr));

    initialise_dialogue();
    //How can we do this without a static cast?
    //ANS: merge list_view_ and tab_definitions_
    for (int tab = 0; tab < Num_Tabs; tab += 1)
    {
        initialise_tab(static_cast<Tab>(tab));
    }
    selected_tab_changed();

    register_dialogue(dlg_num,
        Position::Dock_Bottom,
        static_cast<HICON>(::LoadImage(
            static_cast<HINSTANCE>(module_handle), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT)));
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
    for (auto const &list_view : list_views_)
    {
        ListView_DeleteAllItems(list_view);
    }

    for (auto &error : errors_)
    {
        error.clear();
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

/** This is a strange function defined by windows.
 * 
 * The return value is basically TRUE if you've handled it, FALSE otherwise. If we don't
 * understand the message, we hand it onto the DockingDialogInterface.
 *
 * Some messages have special returns though.
 */

INT_PTR CALLBACK Linter::OutputDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_COMMAND:
        {
            //Context menu responses
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
        }
        break;

        case WM_NOTIFY:
        {
            NMHDR const *notify_header = cast_to<LPNMHDR, LPARAM>(lParam);
            switch (notify_header->code)
            {
                case LVN_KEYDOWN:
                    if (notify_header->idFrom == tab_definitions_[current_tab_].list_view_id_)
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
                    if (notify_header->idFrom == tab_definitions_[current_tab_].list_view_id_)
                    {
                        NMITEMACTIVATE const *lpnmitem = cast_to<LPNMITEMACTIVATE, LPARAM>(lParam);
                        int const selected_item = lpnmitem->iItem;
                        if (selected_item != -1)
                        {
                            show_selected_lint(selected_item);
                        }
                    }
                    return TRUE;

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
        }
        break;

        case WM_CONTEXTMENU:
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
        break;

        default:
            break;
    }

    //Don't recognise the message. Pass it to the base class
    return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}

void Linter::OutputDialog::initialise_dialogue() noexcept
{
    tab_bar_ = GetDlgItem(IDC_TABBAR);

    TCITEM tie{};
    tie.mask = TCIF_TEXT | TCIF_IMAGE;
    tie.iImage = -1;

    for (int tab = 0; tab < Num_Tabs; tab += 1)
    {
        //This const cast is in no way worrying.
        tie.pszText = const_cast<wchar_t *>(tab_definitions_[tab].tab_name_);
        TabCtrl_InsertItem(tab_bar_, tab, &tie);
    }
}

void Linter::OutputDialog::initialise_tab(Tab tab) noexcept
{
    HWND const list_view = GetDlgItem(tab_definitions_[tab].list_view_id_);
    list_views_[tab] = list_view;

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

void Linter::OutputDialog::resize() noexcept
{
    RECT rc;
    getClientRect(rc);

    ::MoveWindow(tab_bar_, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

    TabCtrl_AdjustRect(tab_bar_, FALSE, &rc);
    for (auto const &list_view : list_views_)
    {
        ::SetWindowPos(list_view, tab_bar_, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
        ListView_SetColumnWidth(list_view, Column_Message, LVSCW_AUTOSIZE);
    }
}

void Linter::OutputDialog::selected_tab_changed() noexcept
{
    int const selected = TabCtrl_GetCurSel(tab_bar_);
    current_tab_ = static_cast<Tab>(selected);
    current_list_view_ = list_views_[current_tab_];
    for (int tab = 0; tab < Num_Tabs; tab += 1)
    {
        ShowWindow(list_views_[tab], selected == tab ? SW_SHOW : SW_HIDE);
    }
}

void Linter::OutputDialog::update_displayed_counts()
{
    std::wstringstream stream;
    for (int tab = 0; tab < Num_Tabs; tab += 1)
    {
        std::wstring strTabName;
        int const count = ListView_GetItemCount(list_views_[tab]);
        if (count > 0)
        {
            stream.str(L"");
            stream << tab_definitions_[tab].tab_name_ << L" (" << count << L")";
            strTabName = stream.str();
        }
        else
        {
            strTabName = tab_definitions_[tab].tab_name_;
        }

#if __cplusplus >= 202002L
        TCITEM const tie{.mask = TCIF_TEXT, .pszText = const_cast<wchar_t *>(strTabName.c_str())};
#else
        TCITEM tie{};
        tie.mask = TCIF_TEXT;
        tie.pszText = const_cast<wchar_t *>(strTabName.c_str());
#endif
        TabCtrl_SetItem(tab_bar_, tab, &tie);
    }
}

void Linter::OutputDialog::add_errors(Tab tab, std::vector<XmlParser::Error> const &lints)
{
    HWND list_view = list_views_[tab];

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

        errors_[tab].push_back(lint);
    }

    update_displayed_counts();

    //Not sure we should do this every time we add something, but...
    //Also allow user to sort differently and remember?
#if __cplusplus >= 202002L
    Sort_Call_Info const info{.dialogue = this, .tab = tab};
#else
    Sort_Call_Info const info{this, tab};
#endif
    ListView_SortItemsEx(list_view, sort_call_function, reinterpret_cast<LPARAM>(&info));
    request_redraw();
}

void Linter::OutputDialog::select_next_lint() noexcept
{
    //int const tab = TabCtrl_GetCurSel(tab_bar_);
    HWND list_view = list_views_[current_tab_];

    int const count = ListView_GetItemCount(list_view);
    if (count == 0)
    {
        // no lints, set focus to editor
        HWND current_window = getScintillaWindow();
        SetFocus(current_window);
        return;
    }

    int row = ListView_GetNextItem(list_view, -1, LVNI_FOCUSED | LVNI_SELECTED) + 1;
    if (row == count)
    {
        row = 0;
    }

    ListView_SetItemState(list_view, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);

    ListView_SetItemState(list_view, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(list_view, row, FALSE);
    show_selected_lint(row);
}

void Linter::OutputDialog::select_previous_lint() noexcept
{
    //int const tab = TabCtrl_GetCurSel(tab_bar_);
    HWND list_view = list_views_[current_tab_];

    int const count = ListView_GetItemCount(list_view);
    if (count == 0)
    {
        // no lints, set focus to editor
        HWND current_window = getScintillaWindow();
        SetFocus(current_window);
        return;
    }

    int row = ListView_GetNextItem(list_view, -1, LVNI_FOCUSED | LVNI_SELECTED) - 1;
    if (row == -1)
    {
        row = count - 1;
    }

    ListView_SetItemState(list_view, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);

    ListView_SetItemState(list_view, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(list_view, row, FALSE);
    show_selected_lint(row);
}

//FIXME We've already worked out the tab in all callers of this.
void Linter::OutputDialog::show_selected_lint(int selected_item) noexcept
{
#if __cplusplus >= 202002L
    LVITEM const item{.mask = LVIF_PARAM, .iItem = selected_item};
#else
    LVITEM item{};
    item.iItem = selected_item;
    item.mask = LVIF_PARAM;
#endif
    ListView_GetItem(current_list_view_, &item);

    XmlParser::Error const &lint_error = errors_[current_tab_][item.lParam];

    int const line = std::max(lint_error.m_line - 1, 0);
    int const column = std::max(lint_error.m_column - 1, 0);

    /* We only need to do this if we need to pop up linter.xml. The following isn't ideal */
    if (current_tab_ == Tab::System_Error)
    {
        editConfig();
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

        auto const &lint_error = errors_[current_tab_][item.lParam];

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

    //FIXME do i really need to recreate this every copy / paste?
    class Clipboard
    {
      public:
        explicit Clipboard(HWND self)
        {
            if (!::OpenClipboard(self))
            {
                throw SystemError("Cannot open the Clipboard");
            }
        }

        Clipboard(Clipboard const &) = delete;
        Clipboard(Clipboard &&) = delete;

        Clipboard operator=(Clipboard const &) = delete;
        Clipboard operator=(Clipboard &&) = delete;

        ~Clipboard()
        {
            if (mem_handle_ != nullptr)
            {
                ::GlobalFree(mem_handle_);
            }
            ::CloseClipboard();
        }

        void empty()
        {
            if (!::EmptyClipboard())
            {
                throw SystemError("Cannot empty the Clipboard");
            }
        }

        void copy(std::wstring const &str)
        {
            size_t const size = (str.size() + 1) * sizeof(TCHAR);
            mem_handle_ = ::GlobalAlloc(GMEM_MOVEABLE, size);
            if (mem_handle_ == nullptr)
            {
                throw SystemError("Cannot allocate memory for clipboard");
            }

            LPVOID lpsz = ::GlobalLock(mem_handle_);
            if (lpsz == nullptr)
            {
                throw SystemError("Cannot lock memory for clipboard");
            }

            std::memcpy(lpsz, str.c_str(), size);

            ::GlobalUnlock(mem_handle_);

            if (::SetClipboardData(CF_UNICODETEXT, mem_handle_) == nullptr)
            {
                throw SystemError("Unable to set Clipboard data");
            }

            mem_handle_ = nullptr;
        }

      private:
        HGLOBAL mem_handle_ = nullptr;
    };

    Clipboard clipboard{get_handle()};

    clipboard.empty();
    clipboard.copy(str);
}

int Linter::OutputDialog::sort_selected_list(Tab tab, LPARAM row1_index, LPARAM row2_index) noexcept
{
    auto const &errs = errors_[tab];
    int res = errs[row1_index].m_line - errs[row2_index].m_line;
    if (res == 0)
    {
        res = errs[row1_index].m_column - errs[row2_index].m_column;
    }
    return res;
}

int CALLBACK Linter::OutputDialog::sort_call_function(LPARAM val1, LPARAM val2, LPARAM lParamSort) noexcept
{
    auto const &info = *cast_to<Sort_Call_Info const *, LPARAM>(lParamSort);
    return info.dialogue->sort_selected_list(info.tab, val1, val2);
}
