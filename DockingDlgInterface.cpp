#include "stdafx.h"
#include "DockingDlgInterface.h"

#include "notepad/Notepad_plus_msgs.h"

#include "notepad/DockingFeature/Docking.h"
#include "notepad/DockingFeature/dockingResource.h"

#include <shlwapi.h>

#include <cassert>

DockingDlgInterface::DockingDlgInterface(int dialogID, HINSTANCE hInst, HWND parent) : StaticDialog(hInst, parent, dialogID)
{
}

void DockingDlgInterface::register_dialogue(int dlg_num, Position pos, HICON icon, wchar_t const *extra)
{
    TCHAR temp[MAX_PATH];

    ::GetModuleFileName(static_cast<HMODULE>(_hInst), &temp[0], MAX_PATH);
    module_name_ = ::PathFindFileName(&temp[0]);

    ::GetWindowText(_hSelf, &temp[0], MAX_PATH);
    plugin_name_ = &temp[0];

    tTbData data{};

    data.hClient = _hSelf;
    data.pszName = plugin_name_.c_str();
    data.dlgID = dlg_num;

    data.uMask = pos == Position::Floating ? DWS_DF_FLOATING : static_cast<int>(pos) << 28;
    if (icon != nullptr)
    {
        data.uMask |= DWS_ICONTAB;
        data.hIconTab = icon;
    }
    if (extra != nullptr)
    {
        data.uMask |= DWS_ADDINFO;
        data.pszAddInfo = extra;
    }
    data.pszModuleName = module_name_.c_str();

    ::SendMessage(_hParent, NPPM_DMMREGASDCKDLG, 0, reinterpret_cast<LPARAM>(&data));

    // I'm not sure why I need this. If I don't have it the dialogue opens up every time.
    // If I do have it, the dialogue opens only if it was open when notepad++ was shut down...
    hide();
}

void DockingDlgInterface::updateDockingDlg() noexcept
{
    SendDialogInfoToNPP(NPPM_DMMUPDATEDISPINFO);
}

void DockingDlgInterface::display() noexcept
{
    is_closed_ = false;
    SendDialogInfoToNPP(NPPM_DMMSHOW);
}

void DockingDlgInterface::hide() noexcept
{
    is_closed_ = true;
    SendDialogInfoToNPP(NPPM_DMMHIDE);
}

//We can't make this noexcept as it'd mean child classes would unnecessarily need to be noexcept.
//The caller will handle exceptions correctly.
#pragma warning(suppress : 26440)
INT_PTR DockingDlgInterface::run_dlgProc(UINT message, WPARAM, LPARAM lParam)
{
    switch (message)
    {
        case WM_NOTIFY:
        {
            NMHDR const *pnmh = reinterpret_cast<LPNMHDR>(lParam);

            if (pnmh->hwndFrom == _hParent)
            {
                switch (LOWORD(pnmh->code))
                {
                    case DMN_CLOSE:
                        is_closed_ = true;
                        break;

                    case DMN_DOCK:
                        docked_pos_ = HIWORD(pnmh->code);
                        is_floating_ = false;
                        break;

                    case DMN_FLOAT:
                        is_floating_ = true;
                        break;

                    //These are defined in DockingResource.h but I've not managed
                    //to trigger them.
                    case DMN_SWITCHIN:
                    case DMN_SWITCHOFF:
                    case DMN_FLOATDROPPED:
                    default:
                        break;
                }
            }
            break;
        }

        case WM_PAINT:
            paint();
            break;

        case WM_MOVE:
        case WM_SIZE:
            resize();
            break;

        default:
            break;
    }
    return FALSE;
}
