#include "stdafx.h"
#include "DockingDlgInterface.h"

#include "SystemError.h"

#include "notepad/DockingFeature/Docking.h"
#include "notepad/DockingFeature/dockingResource.h"
#include "notepad/Notepad_plus_msgs.h"

#include <comutil.h>
#include <ShlwApi.h>
#include <WinUser.h>

namespace
{
    std::wstring get_module_name(HINSTANCE module_instance)
    {
        TCHAR temp[MAX_PATH] = {0};
        ::GetModuleFileName(module_instance, &temp[0], MAX_PATH);
        return ::PathFindFileName(&temp[0]);
    }

    std::wstring get_plugin_name(HWND dialog_handle)
    {
        TCHAR temp[MAX_PATH] = {0};
        ::GetWindowText(dialog_handle, &temp[0], MAX_PATH);
        return &temp[0];
    }

}    // namespace

DockingDlgInterface::DockingDlgInterface(int dialogID, HINSTANCE hInst, HWND parent)
    : module_instance_(hInst),
      parent_window_(parent),
      dialogue_window_(create_dialogue_window(dialogID)),
      module_name_(get_module_name(hInst)),
      plugin_name_(get_plugin_name(dialogue_window_))
{
    ::SetWindowLongPtr(dialogue_window_, GWLP_USERDATA, cast_to<LONG_PTR, DockingDlgInterface *>(this));

    SendDialogInfoToNPP(NPPM_MODELESSDIALOG, MODELESSDIALOGADD);
}

DockingDlgInterface::~DockingDlgInterface()
{
    // Stop dlgProc from doing anything, since it calls a virtual method which won't be there.
    ::SetWindowLongPtr(dialogue_window_, GWLP_USERDATA, NULL);
    SendDialogInfoToNPP(NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE);
    ::DestroyWindow(dialogue_window_);
}

void DockingDlgInterface::register_dialogue(int dlg_num, Position pos, HICON icon, wchar_t const *extra) noexcept
{
    tTbData data{};

    data.hClient = dialogue_window_;
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

    ::SendMessage(parent_window_, NPPM_DMMREGASDCKDLG, 0, cast_to<LPARAM, tTbData *>(&data));

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

void DockingDlgInterface::request_redraw() const noexcept
{
    ::InvalidateRect(dialogue_window_, nullptr, TRUE);
}

RECT DockingDlgInterface::getClientRect() const noexcept
{
    RECT rc;
    ::GetClientRect(dialogue_window_, &rc);
    return rc;
}

RECT DockingDlgInterface::getWindowRect() const noexcept
{
    RECT rc;
    ::GetWindowRect(dialogue_window_, &rc);
    return rc;
}

HWND DockingDlgInterface::GetDlgItem(int item) const noexcept
{
    return ::GetDlgItem(dialogue_window_, item);
}

//We can't make this noexcept as it'd mean child classes would unnecessarily need to be noexcept.
//The caller will handle exceptions correctly.
#pragma warning(suppress : 26440)
std::optional<LONG_PTR> DockingDlgInterface::run_dlgProc(UINT message, WPARAM, LPARAM lParam)
{
    switch (message)
    {
        case WM_NOTIFY:
        {
            NMHDR const *pnmh = cast_to<LPNMHDR, LPARAM>(lParam);

            if (pnmh->hwndFrom == parent_window_)
            {
                switch (LOWORD(pnmh->code))
                {
                    case DMN_CLOSE:
                        is_closed_ = true;
                        //Must not mark this handled or the window never closes.
                        break;

                    case DMN_DOCK:
                        docked_pos_ = HIWORD(pnmh->code);
                        is_floating_ = false;
                        return TRUE;

                    case DMN_FLOAT:
                        is_floating_ = true;
                        return TRUE;

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
            //This must return FALSE (unhandled)
            ::RedrawWindow(dialogue_window_, nullptr, nullptr, RDW_INVALIDATE);
            break;

        default:
            break;
    }
    return std::nullopt;
}

void DockingDlgInterface::SendDialogInfoToNPP(int msg, int wParam) noexcept
{
#pragma warning(suppress : 26490)
    ::SendMessage(parent_window_, msg, wParam, reinterpret_cast<LPARAM>(dialogue_window_));
}

INT_PTR CALLBACK DockingDlgInterface::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
    auto *const instance = cast_to<DockingDlgInterface *, LONG_PTR>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (instance == nullptr)
    {
        return FALSE;
    }

    try
    {
        auto const retval = instance->run_dlgProc(message, wParam, lParam);
        if (retval)
        {
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, *retval);
        }
        return static_cast<bool>(retval);
    }
    catch (std::exception const &e)
    {
        try
        {
            ::MessageBox(
                hwnd, static_cast<wchar_t *>(static_cast<_bstr_t>(e.what())), instance->plugin_name_.c_str(), MB_OK | MB_ICONERROR);
        }
        catch (std::exception const &)
        {
            ::MessageBox(hwnd, L"Caught exception but cannot get reason", instance->plugin_name_.c_str(), MB_OK | MB_ICONERROR);
        }
        return FALSE;
    }
}

HWND DockingDlgInterface::create_dialogue_window(int dialogID)
{
    HWND const dialogue_window{::CreateDialogParam(
        module_instance_, MAKEINTRESOURCE(dialogID), parent_window_, dlgProc, cast_to<LPARAM, DockingDlgInterface *>(this))};
    if (dialogue_window == nullptr)
    {
        throw Linter::SystemError("Could not create dialogue");
    }
    return dialogue_window;
}
