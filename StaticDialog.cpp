#include "stdafx.h"
#include "StaticDialog.h"

#include "SystemError.h"

#include "notepad/Notepad_plus_msgs.h"

#include <windows.h>

StaticDialog::StaticDialog(HINSTANCE module, HWND parent, int dialogID) :
    _hInst(module), _hParent(parent), _hSelf(::CreateDialogParam(module, MAKEINTRESOURCE(dialogID), parent, dlgProc, reinterpret_cast<LPARAM>(this)))
{
    if (_hSelf == nullptr)
    {
        throw Linter::SystemError("Could not create dialogue");
    }

    ::SetWindowLongPtr(_hSelf, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    SendDialogInfoToNPP(NPPM_MODELESSDIALOG, MODELESSDIALOGADD);
}

StaticDialog::~StaticDialog()
{
    // Stop run_dlgProc from doing anything, since it calls a virtual method which won't be there.
    ::SetWindowLongPtr(_hSelf, GWLP_USERDATA, NULL);
    SendDialogInfoToNPP(NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE);
    ::DestroyWindow(_hSelf);
}

INT_PTR CALLBACK StaticDialog::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
    try
    {
        StaticDialog *pStaticDlg = reinterpret_cast<StaticDialog *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
        return pStaticDlg == nullptr ? FALSE : pStaticDlg->run_dlgProc(message, wParam, lParam);
    }
    catch (std::exception const &e)
    {
        try
        {
            ::MessageBox(hwnd, static_cast<wchar_t *>(static_cast<_bstr_t>(e.what())), L"Linter", MB_OK | MB_ICONERROR);
        }
        catch (std::exception const &)
        {
            ::MessageBox(hwnd, L"Something terrible has gone wrong but I can't tell you what", L"Linter", MB_OK | MB_ICONERROR);
        }
        return TRUE;
    }
}

void StaticDialog::request_redraw(bool forceUpdate) const noexcept
{
    ::InvalidateRect(_hSelf, nullptr, TRUE);
    if (forceUpdate)
    {
        ::UpdateWindow(_hSelf);
    }
}

void StaticDialog::getClientRect(RECT &rc) const noexcept
{
    ::GetClientRect(_hSelf, &rc);
}

void StaticDialog::getWindowRect(RECT &rc) const noexcept
{
    ::GetWindowRect(_hSelf, &rc);
}

void StaticDialog::paint() const noexcept
{
    ::RedrawWindow(_hSelf, nullptr, nullptr, RDW_INVALIDATE);
}

void StaticDialog::SendDialogInfoToNPP(int msg, int wParam) noexcept
{
#pragma warning(suppress : 26490)
    ::SendMessage(_hParent, msg, wParam, reinterpret_cast<LPARAM>(_hSelf));
}
