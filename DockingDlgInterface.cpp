#include "stdafx.h"
#include "DockingDlgInterface.h"

#include "notepad/DockingFeature/dockingResource.h"
#include "notepad/DockingFeature/Docking.h"

#include <Shlwapi.h>
#include <cassert>

DockingDlgInterface::DockingDlgInterface(int dialogID, HINSTANCE hInst, HWND parent, int dlg_num) : StaticDialog(hInst, parent, dialogID)
{
    TCHAR temp[MAX_PATH];
    ::GetModuleFileName(static_cast<HMODULE>(hInst), &temp[0], MAX_PATH);
    _moduleName = ::PathFindFileName(&temp[0]);

    ::GetWindowText(_hSelf, &temp[0], MAX_PATH);
    _pluginName = &temp[0];

    tTbData data{};

    // user information
    data.hClient = _hSelf;
    data.pszName = _pluginName.c_str();

    // supported features by plugin
    data.uMask = 0;

    // additional info
    data.pszAddInfo = NULL;

    // define the default docking behaviour
    //**FIXME Pass in as mask and icon which if not null sets icontab
    data.uMask = DWS_DF_CONT_BOTTOM | DWS_ICONTAB;
    data.pszModuleName = _moduleName.c_str();

    //Add an icon - I don't have one
    //data.hIconTab = (HICON)GetTabIcon();

    //The dialogue num is the function that caused this dialogue to be displayed.
    data.dlgID = dlg_num;

    ::SendMessage(_hParent, NPPM_DMMREGASDCKDLG, 0, reinterpret_cast<LPARAM>(&data));
}

void DockingDlgInterface::updateDockingDlg() noexcept
{
    ::SendMessage(_hParent, NPPM_DMMUPDATEDISPINFO, 0, reinterpret_cast<LPARAM>(_hSelf));
}

void DockingDlgInterface::display() noexcept
{
    _isClosed = false;
    ::SendMessage(_hParent, NPPM_DMMSHOW, 0, reinterpret_cast<LPARAM>(_hSelf));
}

void DockingDlgInterface::hide() noexcept
{
    _isClosed = true;
    ::SendMessage(_hParent, NPPM_DMMHIDE, 0, reinterpret_cast<LPARAM>(_hSelf));
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
                        _isClosed = true;
                        break;

                    case DMN_DOCK:
                        _iDockedPos = HIWORD(pnmh->code);
                        _isFloating = false;
                        break;

                    case DMN_FLOAT:
                        _isFloating = true;
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

        default:
            break;
    }
    return FALSE;
}
