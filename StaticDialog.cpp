#include "stdafx.h"
#include "StaticDialog.h"
#include "SystemError.h"

#include <string>
#include <windows.h>

StaticDialog::StaticDialog(HINSTANCE hInst, HWND parent, int dialogID) : Window(hInst, parent)
{
    _hSelf = ::CreateDialogParam(_hInst, MAKEINTRESOURCE(dialogID), _hParent, dlgProc, reinterpret_cast<LPARAM>(this));
    if (!_hSelf)
    {
        throw Linter::SystemError("Could not create dialogue");
    }

    ::SetWindowLongPtr(_hSelf, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    //::GetWindowRect(_hSelf, &_rc);

    // if the destination of message NPPM_MODELESSDIALOG is not its parent, then it's the grand-parent
    ::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, reinterpret_cast<WPARAM>(_hSelf));
}

StaticDialog::~StaticDialog()
{
    // Prevent run_dlgProc from doing anything, since its virtual
    ::SetWindowLongPtr(_hSelf, GWLP_USERDATA, NULL);
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
            std::string const s{e.what()};
            ::MessageBox(hwnd, std::wstring(s.begin(), s.end()).c_str(), L"Linter", MB_OK | MB_ICONERROR);
        }
        catch (std::exception const &)
        {
            ::MessageBox(hwnd, L"Something terrible has gone wrong but I can't tell you what", L"Linter", MB_OK | MB_ICONERROR);
        }
        return TRUE;
    }
}
