// This file is part of Notepad++ project
// Copyright (C)2022 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "stdafx.h"
#include "StaticDialog.h"
#include "SystemError.h"

#include <string>
#include <windows.h>

StaticDialog::StaticDialog(HINSTANCE hInst, HWND parent) noexcept : Window(hInst, parent)
{
}

StaticDialog::~StaticDialog()
{
    if (isCreated())
    {
        // Prevent run_dlgProc from doing anything, since its virtual
        ::SetWindowLongPtr(_hSelf, GWLP_USERDATA, NULL);
        destroy();
    }
}

void StaticDialog::destroy() noexcept
{
    ::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, reinterpret_cast<WPARAM>(_hSelf));
    ::DestroyWindow(_hSelf);
}

void StaticDialog::display(bool toShow, bool enhancedPositioningCheckWhenShowing) const noexcept
{
    if (toShow)
    {
        if (enhancedPositioningCheckWhenShowing)
        {
            RECT testPositionRc, candidateRc;

            getWindowRect(testPositionRc);

            candidateRc = getViewablePositionRect(testPositionRc);

            if ((testPositionRc.left != candidateRc.left) || (testPositionRc.top != candidateRc.top))
            {
                ::MoveWindow(_hSelf,
                    candidateRc.left,
                    candidateRc.top,
                    candidateRc.right - candidateRc.left,
                    candidateRc.bottom - candidateRc.top,
                    TRUE);
            }
        }
        else
        {
            // If the user has switched from a dual monitor to a single monitor since we last
            // displayed the dialog, then ensure that it's still visible on the single monitor.
            RECT workAreaRect = {0};
            ::SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRect, 0);
            RECT rc = {0};
            ::GetWindowRect(_hSelf, &rc);
            int newLeft = rc.left;
            int newTop = rc.top;
            int const margin = ::GetSystemMetrics(SM_CYSMCAPTION);

            if (newLeft > ::GetSystemMetrics(SM_CXVIRTUALSCREEN) - margin)
            {
                newLeft -= rc.right - workAreaRect.right;
            }
            if (newLeft + (rc.right - rc.left) < ::GetSystemMetrics(SM_XVIRTUALSCREEN) + margin)
            {
                newLeft = workAreaRect.left;
            }
            if (newTop > ::GetSystemMetrics(SM_CYVIRTUALSCREEN) - margin)
            {
                newTop -= rc.bottom - workAreaRect.bottom;
            }
            if (newTop + (rc.bottom - rc.top) < ::GetSystemMetrics(SM_YVIRTUALSCREEN) + margin)
            {
                newTop = workAreaRect.top;
            }

            if ((newLeft != rc.left) || (newTop != rc.top))    // then the virtual screen size has shrunk
            {
                // Remember that MoveWindow wants width/height.
                ::MoveWindow(_hSelf, newLeft, newTop, rc.right - rc.left, rc.bottom - rc.top, TRUE);
            }
        }
    }

    Window::display(toShow);
}

RECT StaticDialog::getViewablePositionRect(RECT testPositionRc) const noexcept
{
    HMONITOR hMon = ::MonitorFromRect(&testPositionRc, MONITOR_DEFAULTTONULL);

    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);

    bool rectPosViewableWithoutChange = false;

    if (hMon != NULL)
    {
        // rect would be at least partially visible on a monitor

        ::GetMonitorInfo(hMon, &mi);

        int const margin = ::GetSystemMetrics(SM_CYBORDER) + ::GetSystemMetrics(SM_CYSIZEFRAME) + ::GetSystemMetrics(SM_CYCAPTION);

        // require that the title bar of the window be in a viewable place so the user can see it to grab it with the mouse
        if ((testPositionRc.top >= mi.rcWork.top) && (testPositionRc.top + margin <= mi.rcWork.bottom) &&
            // require that some reasonable amount of width of the title bar be in the viewable area:
            (testPositionRc.right - (margin * 2) > mi.rcWork.left) && (testPositionRc.left + (margin * 2) < mi.rcWork.right))
        {
            rectPosViewableWithoutChange = true;
        }
    }
    else
    {
        // rect would not have been visible on a monitor; get info about the nearest monitor to it

        hMon = ::MonitorFromRect(&testPositionRc, MONITOR_DEFAULTTONEAREST);

        ::GetMonitorInfo(hMon, &mi);
    }

    RECT returnRc = testPositionRc;

    if (!rectPosViewableWithoutChange)
    {
        // reposition rect so that it would be viewable on current/nearest monitor, centering if reasonable

        LONG const testRectWidth = testPositionRc.right - testPositionRc.left;
        LONG const testRectHeight = testPositionRc.bottom - testPositionRc.top;
        LONG const monWidth = mi.rcWork.right - mi.rcWork.left;
        LONG const monHeight = mi.rcWork.bottom - mi.rcWork.top;

        returnRc.left = mi.rcWork.left;
        if (testRectWidth < monWidth)
        {
            returnRc.left += (monWidth - testRectWidth) / 2;
        }
        returnRc.right = returnRc.left + testRectWidth;

        returnRc.top = mi.rcWork.top;
        if (testRectHeight < monHeight)
        {
            returnRc.top += (monHeight - testRectHeight) / 2;
        }
        returnRc.bottom = returnRc.top + testRectHeight;
    }

    return returnRc;
}

std::wstring GetLastErrorAsString(DWORD errorCode)
{
    std::wstring errorMsg(_T(""));
    // Get the error message, if any.
    // If both error codes (passed error n GetLastError) are 0, then return empty
    if (errorCode == 0)
    {
        errorCode = GetLastError();
    }
    if (errorCode == 0)
    {
        return errorMsg;    //No error message has been recorded
    }

    LPWSTR messageBuffer = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&messageBuffer,
        0,
        nullptr);

    errorMsg += messageBuffer;

    //Free the buffer.
    LocalFree(messageBuffer);

    return errorMsg;
}

void StaticDialog::create(int dialogID)
{
    _hSelf = ::CreateDialogParam(_hInst, MAKEINTRESOURCE(dialogID), _hParent, dlgProc, reinterpret_cast<LPARAM>(this));

    if (!_hSelf)
    {
        throw Linter::SystemError("Could not create dialogue");
    }

    // if the destination of message NPPM_MODELESSDIALOG is not its parent, then it's the grand-parent
    ::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, reinterpret_cast<WPARAM>(_hSelf));
}

INT_PTR CALLBACK StaticDialog::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
    //This is exceptionally klunky
    StaticDialog *pStaticDlg;
    if (message == WM_INITDIALOG)
    {
        pStaticDlg = reinterpret_cast<StaticDialog *>(lParam);
        pStaticDlg->_hSelf = hwnd;
        ::SetWindowLongPtr(hwnd, GWLP_USERDATA, static_cast<LONG_PTR>(lParam));
        ::GetWindowRect(hwnd, &(pStaticDlg->_rc));
    }
    else
    {
        pStaticDlg = reinterpret_cast<StaticDialog *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (pStaticDlg == nullptr)
        {
            return FALSE;
        }
    }

    try
    {
        return pStaticDlg->run_dlgProc(message, wParam, lParam);
    }
    catch (std::exception const &e)
    {
        try
        {
            std::string const s{e.what()};
            ::MessageBox(pStaticDlg->_hSelf, std::wstring(s.begin(), s.end()).c_str(), L"Linter", MB_OK | MB_ICONERROR);
        }
        catch (std::exception const &)
        {
            ::MessageBox(
                pStaticDlg->_hSelf, L"Something terrible has gone wrong but I can't tell you what", L"Linter", MB_OK | MB_ICONERROR);
        }
        return TRUE;
    }
}
