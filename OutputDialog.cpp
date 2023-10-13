#include "stdafx.h"
#include "OutputDialog.h"

#include "resource.h"

#include "notepad/PluginInterface.h"

#include <CommCtrl.h>

#include <algorithm>
#include <cstddef>
#include <sstream>

namespace Linter
{
    /** Columns in the error list */
    enum List_Column
    {
        Column_Number,
        Column_Tool,
        Column_Line,
        Column_Position,
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

/*
////////////////////////////////////////////////////////////////////////////////


#define IDM_TOOLBAR 2000

#define IDM_TB_JSLINT_CURRENT_FILE (IDM_TOOLBAR + 1)
#define IDM_TB_JSLINT_ALL_FILES (IDM_TOOLBAR + 2)
#define IDM_TB_PREV_LINT (IDM_TOOLBAR + 3)
#define IDM_TB_NEXT_LINT (IDM_TOOLBAR + 4)
#define IDM_TB_JSLINT_OPTIONS (IDM_TOOLBAR + 5)

////////////////////////////////////////////////////////////////////////////////

*/
    std::array<OutputDialog::TabDefinition, OutputDialog::Num_Tabs> const OutputDialog::tab_definitions_ = {
        OutputDialog::TabDefinition{L"System Errors", IDC_LIST_OUTPUT},
        OutputDialog::TabDefinition{L"Lint Errors",   IDC_LIST_LINTS },
    };

    ////////////////////////////////////////////////////////////////////////////////

    //Note: we do actually initialise dialogue_ during the construction, but it's done
    //in a callback from create...
    OutputDialog::OutputDialog(NppData const &npp_data, HANDLE module_handle, int dlg_num)
        : DockingDlgInterface(IDD_OUTPUT), npp_data_(npp_data), dialogue_()
    {
        std::fill_n(&list_views_[0], Num_Tabs, static_cast<HWND>(nullptr));

        init(static_cast<HINSTANCE>(module_handle), npp_data._nppHandle);

        tTbData data{};

        create(&data);
        // define the default docking behaviour
        data.uMask = DWS_DF_CONT_BOTTOM | DWS_ICONTAB;
        data.pszModuleName = getPluginFileName();

        //Add an icon - I don't have one
        //data.hIconTab = GetTabIcon();

        //The dialogue ID is the function that caused this dialogue to be displayed.
        data.dlgID = dlg_num;

        ::SendMessage(npp_data._nppHandle, NPPM_DMMREGASDCKDLG, 0, reinterpret_cast<LPARAM>(&data));

        // I'm not sure why I need this. If I don't have it the dialogue opens up every time.
        // If I do have it, the dialogue opens only if it was open when notepad++ was shut down...
        display(false);
    }

    OutputDialog::~OutputDialog()
    {
    }

    void OutputDialog::display(bool toShow) const
    {
        DockingDlgInterface::display(toShow);
        if (toShow)
        {
            ::SetFocus(dialogue_);
        }
    }

    void OutputDialog::clear_lint_info()
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

    void OutputDialog::add_system_error(XmlParser::Error const &err)
    {
        std::vector<XmlParser::Error> errs;
        errs.push_back(err);
        add_errors(Tab::System_Error, errs);
    }

    void OutputDialog::add_lint_errors(std::vector<XmlParser::Error> const &errs)
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
    INT_PTR CALLBACK OutputDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
            case WM_INITDIALOG:
                initialise_dialogue();
                //How can we do this without a static cast?
                for (int tab = 0; tab < Num_Tabs; tab += 1)
                {
                    initialise_tab(static_cast<Tab>(tab));
                }
                selected_tab_changed();
                return FALSE;    //Do NOT focus onto this dialogue

            case WM_COMMAND:
            {
                //Context menu responses
                switch LOWORD(wParam)
                {
                    case Context_Copy_Lints:
                        copy_to_clipboard();
                        return TRUE;

                    case Context_Show_Source_Line:
                    {
                        int tab = TabCtrl_GetCurSel(dialogue_);
                        int item = ListView_GetNextItem(list_views_[tab], -1, LVIS_FOCUSED | LVIS_SELECTED);
                        if (item != -1)
                        {
                            show_selected_lint(item);
                        }
                        return TRUE;
                    }

                    case Context_Select_All:
                        ListView_SetItemState(list_views_[TabCtrl_GetCurSel(dialogue_)], -1, LVIS_SELECTED, LVIS_SELECTED);
                        return TRUE;
                }
            }
            break;

            case WM_NOTIFY:
            {
                LPNMHDR notify_header = reinterpret_cast<LPNMHDR>(lParam);
                switch (notify_header->code)
                {
                    case LVN_KEYDOWN:
                        if (notify_header->idFrom == tab_definitions_[TabCtrl_GetCurSel(dialogue_)].list_view_id_)
                        {
                            LPNMLVKEYDOWN pnkd = reinterpret_cast<LPNMLVKEYDOWN>(lParam);
                            if (pnkd->wVKey == 'A' && (::GetKeyState(VK_CONTROL) & 0x8000U) != 0)
                            {
                                ListView_SetItemState(list_views_[TabCtrl_GetCurSel(dialogue_)], -1, LVIS_SELECTED, LVIS_SELECTED);
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
                        if (notify_header->idFrom == tab_definitions_[TabCtrl_GetCurSel(dialogue_)].list_view_id_)
                        {
                            LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
                            int selected_item = lpnmitem->iItem;
                            if (selected_item != -1)
                            {
                                show_selected_lint(selected_item);
                            }
                        }
                        return TRUE;

                    case TTN_GETDISPINFO:
                    {
                        LPTOOLTIPTEXT lpttt = reinterpret_cast<LPTOOLTIPTEXT>(notify_header);
                        lpttt->hinst = _hInst;

                        // Specify the resource identifier of the descriptive
                        // text for the given button.
                        /*
                        int resId = int(lpttt->hdr.idFrom);
                        TCHAR tip[MAX_PATH];
                        get_name_from_cmd(resId, tip, sizeof(tip));
                        lpttt->lpszText = tip;
                        */
                        return TRUE;
                    }

                    case TCN_SELCHANGE:
                        if (notify_header->idFrom == IDC_TABBAR)
                        {
                            selected_tab_changed();
                            return TRUE;
                        }
                        break;
                }
            }
            break;

            case WM_CONTEXTMENU:
            {
                // Right click in docked window.
                // build context menu
                HMENU menu = ::CreatePopupMenu();

                int const numSelected = ListView_GetSelectedCount(list_views_[TabCtrl_GetCurSel(dialogue_)]);

                if (numSelected > 0)
                {
                    int iTab = TabCtrl_GetCurSel(dialogue_);

                    int iFocused = ListView_GetNextItem(list_views_[iTab], -1, LVIS_FOCUSED | LVIS_SELECTED);
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
                POINT point;
                point.x = LOWORD(lParam);
                point.y = HIWORD(lParam);
                if (point.x == 65535 || point.y == 65535)
                {
                    point.x = 0;
                    point.y = 0;
                    ClientToScreen(list_views_[TabCtrl_GetCurSel(dialogue_)], &point);
                }

                // show context menu
                TrackPopupMenu(menu, 0, point.x, point.y, 0, _hSelf, nullptr);
                return TRUE;
            }
            break;

            case WM_SIZE:
                resize();
                return TRUE;
        }

        //Don't recognise the message. Pass it to the base class
        return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }

#if 0
    void OutputDialog::on_toolbar_cmd(UINT /* message*/)
    {
        switch (message)
        {
            case IDM_TB_JSLINT_CURRENT_FILE:
                jsLintCurrentFile();
                break;

            case IDM_TB_JSLINT_ALL_FILES:
                jsLintAllFiles();
                break;

            case IDM_TB_NEXT_LINT:
                gotoNextLint();
                break;

            case IDM_TB_PREV_LINT:
                gotoPrevLint();
                break;

            case IDM_TB_JSLINT_OPTIONS:
                showJSLintOptionsDlg();
                break;
        }
        */
    }

    //void OutputDialog::OnToolbarDropDown(LPNMTOOLBAR lpnmtb)
    //{
    //}
#endif
    void OutputDialog::initialise_dialogue()
    {
        // I'd initialise this after calling create, but create calls this
        // via a callback which is - quite strange. Also, I can't help feeling
        // that this value must already be known.
        dialogue_ = ::GetDlgItem(_hSelf, IDC_TABBAR);

        TCITEM tie{};

        tie.mask = TCIF_TEXT | TCIF_IMAGE;
        tie.iImage = -1;

        for (int tab = 0; tab < Num_Tabs; tab += 1)
        {
            //This const cast is in no way worrying.
            tie.pszText = const_cast<wchar_t *>(tab_definitions_[tab].tab_name_);
            TabCtrl_InsertItem(dialogue_, tab, &tie);
        }
    }

    void OutputDialog::initialise_tab(Tab tab)
    {
        auto const list_view = ::GetDlgItem(_hSelf, tab_definitions_[tab].list_view_id_);
        list_views_[tab] = list_view;

        ListView_SetExtendedListViewStyle(list_view, LVS_EX_FULLROWSELECT | LVS_EX_AUTOSIZECOLUMNS);

        LVCOLUMN lvc;
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

        lvc.iSubItem = Column_Number;
        lvc.pszText = const_cast<wchar_t *>(L"");
        lvc.cx = 28;
        lvc.fmt = LVCFMT_RIGHT;
        ListView_InsertColumn(list_view, lvc.iSubItem, &lvc);

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

    void OutputDialog::resize()
    {
        RECT rc;
        getClientRect(rc);

        ::MoveWindow(dialogue_, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

        TabCtrl_AdjustRect(dialogue_, FALSE, &rc);
        for (auto const &list_view : list_views_)
        {
            ::SetWindowPos(list_view, dialogue_, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
            ListView_SetColumnWidth(list_view, Column_Message, LVSCW_AUTOSIZE);
        }
    }

    void OutputDialog::selected_tab_changed()
    {
        int const selected = TabCtrl_GetCurSel(dialogue_);
        for (int tab = 0; tab < Num_Tabs; tab += 1)
        {
            ShowWindow(list_views_[tab], selected == tab ? SW_SHOW : SW_HIDE);
        }
    }

    void OutputDialog::update_displayed_counts()
    {
        std::wstringstream stream;
        for (int tab = 0; tab < Num_Tabs; tab += 1)
        {
            std::wstring strTabName;
            int count = ListView_GetItemCount(list_views_[tab]);
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

            TCITEM tie;
            tie.mask = TCIF_TEXT;
            tie.pszText = const_cast<wchar_t *>(strTabName.c_str());
            TabCtrl_SetItem(dialogue_, tab, &tie);
        }
    }

    void OutputDialog::add_errors(Tab tab, std::vector<XmlParser::Error> const &lints)
    {
        HWND list_view = list_views_[tab];

        std::wstringstream stream;

        LVITEM lvI;
        lvI.mask = LVIF_TEXT | LVIF_STATE;

        for (auto const &lint : lints)
        {
            lvI.iSubItem = 0;
            lvI.iItem = ListView_GetItemCount(list_view);
            lvI.state = 0;
            lvI.stateMask = 0;

            stream.str(L"");
            stream << lvI.iItem + 1;
            std::wstring strNum = stream.str();
            lvI.pszText = const_cast<wchar_t *>(strNum.c_str());
            ListView_InsertItem(list_view, &lvI);

            ListView_SetItemText(list_view, lvI.iItem, Column_Message, const_cast<wchar_t *>(lint.m_message.c_str()));

            std::wstring strFile = lint.m_tool;
            ListView_SetItemText(list_view, lvI.iItem, Column_Tool, const_cast<wchar_t *>(strFile.c_str()));

            stream.str(L"");
            stream << lint.m_line;
            std::wstring strLine = stream.str();
            ListView_SetItemText(list_view, lvI.iItem, Column_Line, const_cast<wchar_t *>(strLine.c_str()));

            stream.str(L"");
            stream << lint.m_column;
            std::wstring strColumn = stream.str();
            ListView_SetItemText(list_view, lvI.iItem, Column_Position, const_cast<wchar_t *>(strColumn.c_str()));

            //Ensure the message column is as wide as the widest column.
            ListView_SetColumnWidth(list_view, Column_Message, LVSCW_AUTOSIZE);

            errors_[tab].push_back(lint);
        }

        update_displayed_counts();

        //Not sure we should do this every time we add something, but...
        //Also allow user to sort differently and remember?
        Sort_Call_Info info;

        info.dialogue = this;
        info.tab = tab;

        ListView_SortItemsEx(list_view, sort_call_function, reinterpret_cast<LPARAM>(&info));
        InvalidateRect(getHSelf(), nullptr, TRUE);
    }

#if 0
    void OutputDialog::get_name_from_cmd(UINT resID, LPTSTR tip, UINT count)
    {
        // NOTE: On change, keep sure to change order of IDM_EX_... in toolBarIcons also
        static wchar_t const *szToolTip[] = {
            L"JSLint Current File",
            L"JSLint All Files",
            L"Go To Previous Lint",
            L"Go To Next Lint",
            L"JSLint Options",
        };

        //This is almost definitely wrong.
        wcscpy_s(tip, count, szToolTip[resID /* - IDM_TB_JSLINT_CURRENT_FILE*/]);
    }
#endif
    /*
    void OutputDialog::select_next_lint()
    {
        if (_hSelf == nullptr)
        {
            return;
        }

        int tab = TabCtrl_GetCurSel(dialogue_);
        HWND list_view = list_views_[tab];

        int count = ListView_GetItemCount(list_view);
        if (count == 0)
        {
            // no lints, set focus to editor
            HWND current_window = GetCurrentScintillaWindow();
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
    */

    /*
    void OutputDialog::select_previous_lint()
    {
        if (_hSelf == nullptr)
        {
            return;
        }

        int tab = TabCtrl_GetCurSel(dialogue_);
        HWND list_view = list_views_[tab];

        int count = ListView_GetItemCount(list_view);
        if (count == 0)
        {
            // no lints, set focus to editor
            HWND current_window = GetCurrentScintillaWindow();
            SetFocus(current_window);
            return;
        }

        int row = ListView_GetNextItem(list_view, -1, LVNI_FOCUSED | LVNI_SELECTED);
        if (--row == -1)
        {
            row = count - 1;
        }

        ListView_SetItemState(list_view, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);

        ListView_SetItemState(list_view, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(list_view, row, FALSE);
        show_selected_lint(row);
    }
    */

    void OutputDialog::show_selected_lint(int selected_item)
    {
        int const tab = TabCtrl_GetCurSel(dialogue_);
        XmlParser::Error const &lint_error = errors_[tab][selected_item];

        int const line = std::max(lint_error.m_line - 1, 0);
        int const column = std::max(lint_error.m_column - 1, 0);

        /* We only need to do this if we need to pop up linter.xml. The following isn't ideal */
        if (tab == Tab::System_Error)
        {
            ::SendMessage(npp_data_._nppHandle, NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(lint_error.m_path.c_str()));
        }

        HWND const current_window = GetCurrentScintillaWindow();
        ::SendMessage(current_window, SCI_GOTOLINE, line, 0);

        // since there is no SCI_GOTOCOLUMN, we move to the right until ...
        for (;;)
        {
            ::SendMessage(current_window, SCI_CHARRIGHT, 0, 0);

            LRESULT const curPos = ::SendMessage(current_window, SCI_GETCURRENTPOS, 0, 0);

            LRESULT const curLine = ::SendMessage(current_window, SCI_LINEFROMPOSITION, curPos, 0);
            if (curLine > line)
            {
                // ... current line is greater than desired line or ...
                ::SendMessage(current_window, SCI_CHARLEFT, 0, 0);
                break;
            }

            LRESULT const curCol = ::SendMessage(current_window, SCI_GETCOLUMN, curPos, 0);
            if (curCol > column)
            {
                // ... current column is greater than desired column or ...
                ::SendMessage(current_window, SCI_CHARLEFT, 0, 0);
                break;
            }

            if (curCol == column)
            {
                // ... we reached desired column.
                break;
            }
        }

        InvalidateRect(getHSelf(), nullptr, TRUE);
    }

    void OutputDialog::copy_to_clipboard()
    {
        std::wstringstream stream;

        int const tab = TabCtrl_GetCurSel(dialogue_);

        bool first = true;
        int row = ListView_GetNextItem(list_views_[tab], -1, LVNI_SELECTED);
        while (row != -1)
        {
            auto const &lint_error = errors_[tab][row];

            if (first)
            {
                first = false;
            }
            else
            {
                stream << L"\r\n";
            }

            stream << L"Line " << lint_error.m_line << L", column " << lint_error.m_column << L": " << L"\r\n\t"
                   << lint_error.m_message << L"\r\n";

            row = ListView_GetNextItem(list_views_[tab], row, LVNI_SELECTED);
        }

        std::wstring const str = stream.str();
        if (str.empty())
        {
            return;
        }

        if (!::OpenClipboard(getHSelf()))
        {
            ::MessageBox(_hSelf, L"Cannot open the Clipboard", L"Linter", MB_OK | MB_ICONERROR);
            return;
        }

        class Clipboard_Releaser
        {
          public:
            Clipboard_Releaser()
            {
            }

            ~Clipboard_Releaser()
            {
                ::CloseClipboard();
            }
        } const clipboard_releaser;

        if (!::EmptyClipboard())
        {
            ::MessageBox(getHSelf(), L"Cannot empty the Clipboard", L"Linter", MB_OK | MB_ICONERROR);
            return;
        }

        size_t const size = (str.size() + 1) * sizeof(TCHAR);
        HGLOBAL const mem_handle = ::GlobalAlloc(GMEM_MOVEABLE, size);
        if (mem_handle == nullptr)
        {
            ::MessageBox(_hSelf, L"Cannot allocate memory for clipboard", L"Linter", MB_OK | MB_ICONERROR);
            return;
        }

        LPVOID lpsz = ::GlobalLock(mem_handle);
        if (lpsz == nullptr)
        {
            ::MessageBox(_hSelf, L"Cannot lock memory for clipboard", L"Linter", MB_OK | MB_ICONERROR);
            ::GlobalFree(mem_handle);
            return;
        }

        std::memcpy(lpsz, str.c_str(), size);
        ::GlobalUnlock(mem_handle);

        if (::SetClipboardData(CF_UNICODETEXT, mem_handle) == nullptr)
        {
            ::GlobalFree(mem_handle);
            ::MessageBox(_hSelf, L"Unable to set Clipboard data", L"Linter", MB_OK | MB_ICONERROR);
        }
    }

    HWND OutputDialog::GetCurrentScintillaWindow() const
    {
        LRESULT const view = ::SendMessage(npp_data_._nppHandle, NPPM_GETCURRENTVIEW, 0, 0);
        return view == 0 ? npp_data_._scintillaMainHandle : npp_data_._scintillaSecondHandle;
    }

    int OutputDialog::sort_selected_list(Tab tab, LPARAM row1_index, LPARAM row2_index)
    {
        return errors_[tab][row1_index].m_line - errors_[tab][row2_index].m_line;
    }

    int CALLBACK OutputDialog::sort_call_function(LPARAM val1, LPARAM val2, LPARAM lParamSort)
    {
        auto const &info = *reinterpret_cast<Sort_Call_Info *>(lParamSort);
        return info.dialogue->sort_selected_list(info.tab, val1, val2);
    }

}    // namespace Linter
