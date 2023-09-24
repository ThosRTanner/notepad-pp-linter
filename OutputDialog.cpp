#include "stdafx.h"
#include "OutputDialog.h"
#include "resource.h"

#include "notepad/PluginInterface.h"

#include <CommCtrl.h>

namespace Linter
{

    /*
////////////////////////////////////////////////////////////////////////////////

#define ID_COPY_LINTS 1500
#define ID_SHOW_LINT 1501
#define ID_ADD_PREDEFINED 1502
#define ID_SELECT_ALL 1503

#define IDM_TOOLBAR 2000

#define IDM_TB_JSLINT_CURRENT_FILE (IDM_TOOLBAR + 1)
#define IDM_TB_JSLINT_ALL_FILES (IDM_TOOLBAR + 2)
#define IDM_TB_PREV_LINT (IDM_TOOLBAR + 3)
#define IDM_TB_NEXT_LINT (IDM_TOOLBAR + 4)
#define IDM_TB_JSLINT_OPTIONS (IDM_TOOLBAR + 5)

////////////////////////////////////////////////////////////////////////////////

*/
    OutputDialog::TabDefinition OutputDialog::tab_definitions_[] = {
  //FIXME certain amount of redefinition here
        {TEXT("System"),   IDC_TEXTOUT,      TabDefinition::SYSTEM_ERROR},
        {TEXT("Errors"),   IDC_ERROR_LIST,   TabDefinition::LINT_ERROR  },
        {TEXT("Warnings"), IDC_WARNING_LIST, TabDefinition::LINT_WARNING},
        {TEXT("Info"),     IDC_INFO_LIST,    TabDefinition::LINT_INFO   }
    };

    ////////////////////////////////////////////////////////////////////////////////

    //Note: we do actually initialise tab window during the construction, but it's done
    //in a callback from create...
    OutputDialog::OutputDialog(NppData const &npp_data, HANDLE module_handle, int dlg_num)
        : DockingDlgInterface(IDD_OUTPUT), npp_data_(npp_data), tab_window_()
    {
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

        for (int i = 0; i < NUM_TABS; ++i)
        {
            tab_views_[i] = 0;
        }

        //FIXME I'm not sure why I need this. I don't want this to open automatically unless it
        //was open when npp was last exited, but it seems to all the time.
        display(false);
    }

    OutputDialog::~OutputDialog()
    {
        //DestroyIcon(tab_icon_);
    }

    void OutputDialog::display(bool toShow) const
    {
        DockingDlgInterface::display(toShow);
        if (toShow)
        {
            //This is strange because though it's defined in resource.h, is not in the .rc file
            //::SetFocus(::GetDlgItem(_hSelf, IDC_OUTPUT));
            ::SetFocus(tab_window_);
        }
    }

    void OutputDialog::clear_lint_info()
    {
        errors_.clear();
    }

    void OutputDialog::add_error(std::string const &err)
    {
        errors_.push_back(err);
    };

    INT_PTR CALLBACK OutputDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
            case WM_INITDIALOG:
                initialise_tab();
                for (int tab = 0; tab < NUM_TABS; tab += 1)
                {
                    initialise_list_view(tab);
                }
                selected_tab_changed();
                break;

            case WM_COMMAND:
            {
                /*
                if (LOWORD(wParam) == ID_COPY_LINTS)
                {
                    copy_to_clipboard();
                    return TRUE;
                }
                else if (LOWORD(wParam) == ID_SHOW_LINT)
                {
                    int iTab = TabCtrl_GetCurSel(tab_window_);
                    int iLint = ListView_GetNextItem(tab_views_[iTab], -1, LVIS_FOCUSED | LVIS_SELECTED);
                    if (iLint != -1)
                    {
                        show_selected_lint(iLint);
                    }
                    return TRUE;
                }
                else if (LOWORD(wParam) == ID_ADD_PREDEFINED)
                {
                    int iTab = TabCtrl_GetCurSel(tab_window_);
                    int iLint = ListView_GetNextItem(tab_views_[iTab], -1, LVIS_FOCUSED | LVIS_SELECTED);
                    if (iLint != -1)
                    {
                        const FileLint &fileLint = m_fileLints[iTab][iLint];
                        std::wstring var = fileLint.lint.GetUndefVar();
                        if (!var.empty())
                        {
                            JSLintOptions::GetInstance().AppendOption(IDC_PREDEFINED, var);
                        }
                    }
                    return TRUE;
                }
                else if (LOWORD(wParam) == ID_SELECT_ALL)
                {
                    ListView_SetItemState(tab_views_[TabCtrl_GetCurSel(tab_window_)], -1, LVIS_SELECTED, LVIS_SELECTED);
                    return TRUE;
                }
                */
            }
            break;

            case WM_NOTIFY:
            {
                LPNMHDR notify_header = reinterpret_cast<LPNMHDR>(lParam);
                switch (notify_header->code)
                {
                    case LVN_KEYDOWN:
                        if (notify_header->idFrom == tab_definitions_[TabCtrl_GetCurSel(tab_window_)].list_view_id_)
                        {
                            LPNMLVKEYDOWN pnkd = reinterpret_cast<LPNMLVKEYDOWN>(lParam);
                            if (pnkd->wVKey == 'A' && (::GetKeyState(VK_CONTROL) >> 15 & 1))
                            {
                                ListView_SetItemState(tab_views_[TabCtrl_GetCurSel(tab_window_)], -1, LVIS_SELECTED, LVIS_SELECTED);
                            }
                            else if (pnkd->wVKey == 'C' && (::GetKeyState(VK_CONTROL) >> 15 & 1))
                            {
                                copy_to_clipboard();
                            }
                        }
                        break;

                    case NM_DBLCLK:
                        if (notify_header->idFrom == tab_definitions_[TabCtrl_GetCurSel(tab_window_)].list_view_id_)
                        {
                            LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
                            int iFocused = lpnmitem->iItem;
                            if (iFocused != -1)
                            {
                                show_selected_lint(iFocused);
                            }
                        }
                        break;

                    case TTN_GETDISPINFO:
                    {
                        LPTOOLTIPTEXT lpttt = reinterpret_cast<LPTOOLTIPTEXT>(notify_header);
                        lpttt->hinst = _hInst;

                        // Specify the resource identifier of the descriptive
                        // text for the given button.
                        int resId = int(lpttt->hdr.idFrom);

                        TCHAR tip[MAX_PATH];
                        get_name_from_cmd(resId, tip, sizeof(tip));
                        lpttt->lpszText = tip;
                        return TRUE;
                    }

                    case TCN_SELCHANGE:
                        if (notify_header->idFrom == IDC_TABBAR)
                        {
                            selected_tab_changed();
                            return TRUE;
                        }
#if __cplusplus >= 201703L
                        [[fallthrough]];
#endif

                    default:
                        return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
                }
            }
            break;

            case WM_CONTEXTMENU:
            {
                // Right click in docked window.
                // build context menu
                HMENU menu = ::CreatePopupMenu();

                int numSelected = ListView_GetSelectedCount(tab_views_[TabCtrl_GetCurSel(tab_window_)]);

                if (numSelected > 0)
                {
                    int iTab = TabCtrl_GetCurSel(tab_window_);

                    int iFocused = ListView_GetNextItem(tab_views_[iTab], -1, LVIS_FOCUSED | LVIS_SELECTED);
                    if (iFocused != -1)
                    {
                        /*
                        AppendMenu(menu, MF_ENABLED, ID_SHOW_LINT, TEXT("Show"));

                        const FileLint &fileLint = m_fileLints[iTab][iFocused];
                        bool reasonIsUndefVar = fileLint.lint.IsReasonUndefVar();
                        if (reasonIsUndefVar)
                        {
                            AppendMenu(menu, MF_ENABLED, ID_ADD_PREDEFINED, TEXT("Add to the Predefined List"));
                        }
                        */
                    }
                }

                if (GetMenuItemCount(menu) > 0)
                {
                    AppendMenu(menu, MF_SEPARATOR, 0, NULL);
                }

                if (numSelected > 0)
                {
                    //AppendMenu(menu, MF_ENABLED, ID_COPY_LINTS, TEXT("Copy"));
                }

                //AppendMenu(menu, MF_ENABLED, ID_SELECT_ALL, TEXT("Select All"));

                // determine context menu position
                POINT point;
                point.x = LOWORD(lParam);
                point.y = HIWORD(lParam);
                if (point.x == 65535 || point.y == 65535)
                {
                    point.x = 0;
                    point.y = 0;
                    ClientToScreen(tab_views_[TabCtrl_GetCurSel(tab_window_)], &point);
                }

                // show context menu
                TrackPopupMenu(menu, 0, point.x, point.y, 0, _hSelf, NULL);
            }
            break;

            case WM_SIZE:
            case WM_MOVE:
                //FIXME Resize currently does nothing. Maybe we should just default this
                //and the next one.
                resize();
                break;

            case WM_PAINT:
                //FIXME Should we just drop through?
                /*::RedrawWindow(m_toolbar.getHSelf(), NULL, NULL, TRUE);*/
                break;

            default:
                return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
        }

        return FALSE;
    }

    void OutputDialog::on_toolbar_cmd(UINT /* message*/)
    {
        /*
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

    void OutputDialog::initialise_tab()
    {
        // I'd initialise this after calling create, but create calls this
        // via a callback which is - quite strange. Also, I can't help feeling
        // that this value must already be known.
        tab_window_ = ::GetDlgItem(_hSelf, IDC_TABBAR);

        TCITEM tie{};

        tie.mask = TCIF_TEXT | TCIF_IMAGE;
        tie.iImage = -1;

        for (int i = 0; i < NUM_TABS; ++i)
        {
            //This const cast is in no way worrying.
            tie.pszText = const_cast<wchar_t *>(tab_definitions_[i].tab_name_);
            TabCtrl_InsertItem(tab_window_, i, &tie);
        }
    }

    void OutputDialog::initialise_list_view(int i)
    {
        tab_views_[i] = ::GetDlgItem(_hSelf, tab_definitions_[i].list_view_id_);

        ListView_SetExtendedListViewStyle(tab_views_[i], LVS_EX_FULLROWSELECT);

        LVCOLUMN lvc;
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

        int column = 0;

        lvc.iSubItem = 0;
        lvc.pszText = const_cast<wchar_t *>(L"");
        lvc.cx = 28;
        lvc.fmt = LVCFMT_RIGHT;
        ListView_InsertColumn(tab_views_[i], column, &lvc);
        column += 1;
        
        if (tab_definitions_[i].type_ == TabDefinition::SYSTEM_ERROR)
        {
            lvc.iSubItem = 1;
            lvc.pszText = const_cast<wchar_t *>(L"Reason");
            lvc.cx = 500;
            lvc.fmt = LVCFMT_LEFT;
            ListView_InsertColumn(tab_views_[i], column, &lvc);
        }
        else
        {
            lvc.iSubItem = column;
            lvc.pszText = const_cast<wchar_t *>(L"Reason");
            lvc.cx = 200;
            lvc.fmt = LVCFMT_LEFT;
            ListView_InsertColumn(tab_views_[i], column, &lvc);

            column += 1;
            lvc.iSubItem = column;
            lvc.pszText = const_cast<wchar_t *>(L"File");
            lvc.cx = 200;
            lvc.fmt = LVCFMT_LEFT;
            ListView_InsertColumn(tab_views_[i], column, &lvc);

            column += 1;
            lvc.iSubItem = column;
            lvc.pszText = const_cast<wchar_t *>(L"Line");
            lvc.cx = 50;
            lvc.fmt = LVCFMT_RIGHT;
            ListView_InsertColumn(tab_views_[i], column, &lvc);

            column += 1;
            lvc.iSubItem = column;
            lvc.pszText = const_cast<wchar_t *>(L"Column");
            lvc.cx = 50;
            lvc.fmt = LVCFMT_RIGHT;
            ListView_InsertColumn(tab_views_[i], column, &lvc);
        }
    }

    void OutputDialog::resize()
    {
        RECT rc;
        getClientRect(rc);

        //m_toolbar.reSizeTo(rc);
        //m_rebar.reSizeTo(rc);
        /* Not sure what is going on here as we don't have a toolbar
    RECT rcToolbar;
    GetWindowRect(m_toolbar.getHSelf(), &rcToolbar);

	getClientRect(rc);
    rc.top += rcToolbar.bottom - rcToolbar.top;
	*/

        //InflateRect(&rc, -4, -4);
        ::MoveWindow(tab_window_, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

        TabCtrl_AdjustRect(tab_window_, FALSE, &rc);
        //InflateRect(&rc, -4, -4);
        for (int i = 0; i < NUM_TABS; ++i)
        {
            ::SetWindowPos(tab_views_[i], tab_window_, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
        }
    }

    void OutputDialog::selected_tab_changed()
    {
        int iSel = TabCtrl_GetCurSel(tab_window_);
        for (int i = 0; i < NUM_TABS; ++i)
        {
            ShowWindow(tab_views_[i], iSel == i ? SW_SHOW : SW_HIDE);
        }
    }

    /*HICON OutputDialog::GetTabIcon()
    {
        if (tab_icon_ == NULL)
        {
            tab_icon_ = (HICON)::LoadImage(
                (HINSTANCE)g_hDllModule, MAKEINTRESOURCE(IDI_JSLINT_TAB), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
        }
        return tab_icon_;
    }*/

    void OutputDialog::get_name_from_cmd(UINT resID, LPTSTR tip, UINT count)
    {
        // NOTE: On change, keep sure to change order of IDM_EX_... in toolBarIcons also
        static wchar_t const * szToolTip[] = {
            L"JSLint Current File",
            L"JSLint All Files",
            L"Go To Previous Lint",
            L"Go To Next Lint",
            L"JSLint Options",
        };

        //This is almost definitely wrong.
        wcscpy_s(tip, count, szToolTip[resID /* - IDM_TB_JSLINT_CURRENT_FILE*/]);
    }

    void OutputDialog::clear_all_lints()
    {
        for (int i = 0; i < NUM_TABS; ++i)
        {
            //m_fileLints[tab].clear();
            ListView_DeleteAllItems(tab_views_[i]);
        }
    }

    /*
    void OutputDialog::AddLints(const std::wstring &strFilePath, const std::list<JSLintReportItem> &lints)
    {
        std::basic_stringstream<TCHAR> stream;

        LVITEM lvI;
        lvI.mask = LVIF_TEXT | LVIF_STATE;

        for (std::list<JSLintReportItem>::const_iterator it = lints.begin(); it != lints.end(); ++it)
        {
            const JSLintReportItem &lint = *it;

            HWND hWndListView = tab_views_[lint.GetType()];

            lvI.iSubItem = 0;
            lvI.iItem = ListView_GetItemCount(hWndListView);
            lvI.state = 0;
            lvI.stateMask = 0;

            stream.str(TEXT(""));
            stream << lvI.iItem + 1;
            std::wstring strNum = stream.str();

            lvI.pszText = (LPTSTR)strNum.c_str();

            ListView_InsertItem(hWndListView, &lvI);

            int column = 1;

            if (tab_definitions_[lint.GetType()].is_error_list_)
            {
                std::wstring strReason = lint.GetReason();
                ListView_SetItemText(hWndListView, lvI.iItem, column++, (LPTSTR)strReason.c_str());
            }
            else
            {
                std::wstring strVariable = lint.GetReason();
                ListView_SetItemText(hWndListView, lvI.iItem, column++, (LPTSTR)strVariable.c_str());
                std::wstring strFunction = lint.GetEvidence();
                ListView_SetItemText(hWndListView, lvI.iItem, column++, (LPTSTR)strFunction.c_str());
            }

            std::wstring strFile = Path::GetFileName(strFilePath);
            ListView_SetItemText(hWndListView, lvI.iItem, column++, (LPTSTR)strFile.c_str());

            stream.str(TEXT(""));
            stream << lint.GetLine() + 1;
            std::wstring strLine = stream.str();
            ListView_SetItemText(hWndListView, lvI.iItem, column++, (LPTSTR)strLine.c_str());

            stream.str(TEXT(""));
            stream << lint.GetCharacter() + 1;
            std::wstring strColumn = stream.str();
            ListView_SetItemText(hWndListView, lvI.iItem, column++, (LPTSTR)strColumn.c_str());

            m_fileLints[lint.GetType()].push_back(FileLint(strFilePath, lint));
        }

        for (int tab = 0; tab < NUM_TABS; ++tab)
        {
            std::wstring strTabName;
            int count = ListView_GetItemCount(tab_views_[tab]);
            if (count > 0)
            {
                stream.str(TEXT(""));
                stream << tab_definitions_[tab].tab_name_ << TEXT(" (") << count << TEXT(")");
                strTabName = stream.str();
            }
            else
            {
                strTabName = tab_definitions_[tab].tab_name_;
            }
            TCITEM tie;
            tie.mask = TCIF_TEXT;
            tie.pszText = (LPTSTR)strTabName.c_str();
            TabCtrl_SetItem(tab_window_, tab, &tie);
        }

        InvalidateRect(getHSelf(), NULL, TRUE);
    }
    */

    /*
    void OutputDialog::select_next_lint()
    {
        if (_hSelf == NULL)
        {
            return;
        }

        int iTab = TabCtrl_GetCurSel(tab_window_);
        HWND hWndListView = tab_views_[iTab];

        int count = ListView_GetItemCount(hWndListView);
        if (count == 0)
        {
            // no lints, set focus to editor
            HWND hWndScintilla = GetCurrentScintillaWindow();
            SetFocus(hWndScintilla);
            return;
        }

        int tab = ListView_GetNextItem(hWndListView, -1, LVNI_FOCUSED | LVNI_SELECTED);
        if (++tab == count)
        {
            tab = 0;
        }

        ListView_SetItemState(hWndListView, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);

        ListView_SetItemState(hWndListView, tab, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(hWndListView, tab, FALSE);
        show_selected_lint(tab);
    }
    */

    /*
    void OutputDialog::select_previous_lint()
    {
        if (_hSelf == NULL)
        {
            return;
        }

        int iTab = TabCtrl_GetCurSel(tab_window_);
        HWND hWndListView = tab_views_[iTab];

        int count = ListView_GetItemCount(hWndListView);
        if (count == 0)
        {
            // no lints, set focus to editor
            HWND hWndScintilla = GetCurrentScintillaWindow();
            SetFocus(hWndScintilla);
            return;
        }

        int tab = ListView_GetNextItem(hWndListView, -1, LVNI_FOCUSED | LVNI_SELECTED);
        if (--tab == -1)
        {
            tab = count - 1;
        }

        ListView_SetItemState(hWndListView, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);

        ListView_SetItemState(hWndListView, tab, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(hWndListView, tab, FALSE);
        show_selected_lint(tab);
    }
    */

    void OutputDialog::show_selected_lint(int /*tab*/)
    {
        /*
        int iTab = TabCtrl_GetCurSel(tab_window_);
        const FileLint &fileLint = m_fileLints[iTab][tab];

        int line = fileLint.lint.GetLine();
        int column = fileLint.lint.GetCharacter();

        if (!fileLint.strFilePath.empty() && line >= 0 && column >= 0)
        {
            LRESULT lRes = ::SendMessage(g_nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)fileLint.strFilePath.c_str());
            if (lRes)
            {
                HWND hWndScintilla = GetCurrentScintillaWindow();
                if (hWndScintilla != NULL)
                {
                    ::SendMessage(hWndScintilla, SCI_GOTOLINE, line, 0);
                    // since there is no SCI_GOTOCOLUMN, we move to the right until ...
                    while (true)
                    {
                        ::SendMessage(hWndScintilla, SCI_CHARRIGHT, 0, 0);

                        int curPos = (int)::SendMessage(hWndScintilla, SCI_GETCURRENTPOS, 0, 0);

                        int curLine = (int)::SendMessage(hWndScintilla, SCI_LINEFROMPOSITION, curPos, 0);
                        if (curLine > line)
                        {
                            // ... current line is greater than desired line or ...
                            ::SendMessage(hWndScintilla, SCI_CHARLEFT, 0, 0);
                            break;
                        }

                        int curCol = (int)::SendMessage(hWndScintilla, SCI_GETCOLUMN, curPos, 0);
                        if (curCol > column)
                        {
                            // ... current column is greater than desired column or ...
                            ::SendMessage(hWndScintilla, SCI_CHARLEFT, 0, 0);
                            break;
                        }

                        if (curCol == column)
                        {
                            // ... we reached desired column.
                            break;
                        }
                    }
                }
            }
        }

        InvalidateRect(getHSelf(), NULL, TRUE);
    */
    }

    void OutputDialog::copy_to_clipboard()
    {
        /*
        std::basic_stringstream<TCHAR> stream;

        int iTab = TabCtrl_GetCurSel(tab_window_);

        bool bFirst = true;
        int tab = ListView_GetNextItem(tab_views_[iTab], -1, LVNI_SELECTED);
        while (tab != -1)
        {
            const FileLint &fileLint = m_fileLints[iTab][tab];

            if (bFirst)
            {
                bFirst = false;
            }
            else
            {
                stream << TEXT("\r\n");
            }

            stream << TEXT("Line ") << fileLint.lint.GetLine() + 1 << TEXT(", column ") << fileLint.lint.GetCharacter() + 1 << TEXT(": ")
                   << fileLint.lint.GetReason().c_str() << TEXT("\r\n\t") << fileLint.lint.GetEvidence().c_str() << TEXT("\r\n");

            tab = ListView_GetNextItem(tab_views_[TabCtrl_GetCurSel(tab_window_)], tab, LVNI_SELECTED);
        }

        std::wstring str = stream.str();
        if (str.empty())
        {
            return;
        }

        if (OpenClipboard(_hSelf))
        {
            if (EmptyClipboard())
            {
                size_t size = (str.size() + 1) * sizeof(TCHAR);
                HGLOBAL hResult = GlobalAlloc(GMEM_MOVEABLE, size);
                LPTSTR lpsz = (LPTSTR)GlobalLock(hResult);
                memcpy(lpsz, str.c_str(), size);
                GlobalUnlock(hResult);

                if (SetClipboardData(CF_UNICODETEXT, hResult) == NULL)
                {
                    GlobalFree(hResult);
                    MessageBox(_hSelf, TEXT("Unable to set Clipboard data"), TEXT("JSLint"), MB_OK | MB_ICONERROR);
                }
            }
            else
            {
                MessageBox(_hSelf, TEXT("Cannot empty the Clipboard"), TEXT("JSLint"), MB_OK | MB_ICONERROR);
            }
            CloseClipboard();
        }
        else
        {
            MessageBox(_hSelf, TEXT("Cannot open the Clipboard"), TEXT("JSLint"), MB_OK | MB_ICONERROR);
        }
    */
    }
}    // namespace Linter
