#include "stdafx.h"
#include "DockingDlgInterface.h"

#include "dockingResource.h"

DockingDlgInterface::DockingDlgInterface() = default;

DockingDlgInterface::DockingDlgInterface(int dlgID) noexcept : _dlgID(dlgID)
{
}

void DockingDlgInterface::init(HINSTANCE hInst, HWND parent)
{
    StaticDialog::init(hInst, parent);
    TCHAR temp[MAX_PATH];
    ::GetModuleFileName(static_cast<HMODULE>(hInst), &temp[0], MAX_PATH);
    _moduleName = ::PathFindFileName(&temp[0]);
}

void DockingDlgInterface::create(tTbData *data, bool isRTL)
{
    assert(data != nullptr);
    StaticDialog::create(_dlgID, isRTL);
    TCHAR temp[MAX_PATH];
    ::GetWindowText(_hSelf, &temp[0], MAX_PATH);
    _pluginName = &temp[0];

    // user information
    data->hClient = _hSelf;
    data->pszName = _pluginName.c_str();

    // supported features by plugin
    data->uMask = 0;

    // additional info
    data->pszAddInfo = NULL;
}

void DockingDlgInterface::updateDockingDlg() noexcept
{
    ::SendMessage(_hParent, NPPM_DMMUPDATEDISPINFO, 0, reinterpret_cast<LPARAM>(_hSelf));
}

void DockingDlgInterface::display(bool toShow) const noexcept
{
    ::SendMessage(_hParent, toShow ? NPPM_DMMSHOW : NPPM_DMMHIDE, 0, reinterpret_cast<LPARAM>(_hSelf));
}

INT_PTR DockingDlgInterface::run_dlgProc(UINT message, WPARAM, LPARAM lParam) noexcept
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
                    {
                        break;
                    }
                    case DMN_FLOAT:
                    {
                        _isFloating = true;
                        break;
                    }
                    case DMN_DOCK:
                    {
                        _iDockedPos = HIWORD(pnmh->code);
                        _isFloating = false;
                        break;
                    }
                    default:
                        break;
                }
            }
            break;
        }
        default:
            break;
    }
    return FALSE;
}
