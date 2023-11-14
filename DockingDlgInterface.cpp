#include "stdafx.h"
#include "DockingDlgInterface.h"

#include "SystemError.h"

#include "notepad/Notepad_plus_msgs.h"

#include "notepad/DockingFeature/Docking.h"
#include "notepad/DockingFeature/dockingResource.h"

#include <shlwapi.h>

#include <cassert>

namespace
{
    std::wstring get_module_name(HINSTANCE module_instance)
    {
        TCHAR temp[MAX_PATH];
        ::GetModuleFileName(module_instance, &temp[0], MAX_PATH);
        return ::PathFindFileName(&temp[0]);
    }

    std::wstring get_plugin_name(HWND dialog_handle)
    {
        TCHAR temp[MAX_PATH];
        ::GetWindowText(dialog_handle, &temp[0], MAX_PATH);
        return &temp[0];
    }
}
DockingDlgInterface::DockingDlgInterface(int dialogID, HINSTANCE hInst, HWND parent)
    : module_instance_(hInst),
      parent_window_(parent),
      dialogue_window_(::CreateDialogParam(hInst, MAKEINTRESOURCE(dialogID), parent, dlgProc, reinterpret_cast<LPARAM>(this))),
      module_name_(get_module_name(hInst)),
      plugin_name_(get_plugin_name(dialogue_window_))
{
    if (dialogue_window_ == nullptr)
    {
        throw Linter::SystemError("Could not create dialogue");
    }

    ::SetWindowLongPtr(dialogue_window_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    SendDialogInfoToNPP(NPPM_MODELESSDIALOG, MODELESSDIALOGADD);
}

DockingDlgInterface::~DockingDlgInterface()
{
    // Stop run_dlgProc from doing anything, since it calls a virtual method which won't be there.
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

    ::SendMessage(parent_window_, NPPM_DMMREGASDCKDLG, 0, reinterpret_cast<LPARAM>(&data));

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

void DockingDlgInterface::request_redraw(bool forceUpdate) const noexcept
{
    ::InvalidateRect(dialogue_window_, nullptr, TRUE);
    if (forceUpdate)
    {
        ::UpdateWindow(dialogue_window_);
    }
}

void DockingDlgInterface::getClientRect(RECT &rc) const noexcept
{
    ::GetClientRect(dialogue_window_, &rc);
}

void DockingDlgInterface::getWindowRect(RECT &rc) const noexcept
{
    ::GetWindowRect(dialogue_window_, &rc);
}

void DockingDlgInterface::paint() const noexcept
{
    ::RedrawWindow(dialogue_window_, nullptr, nullptr, RDW_INVALIDATE);
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

            if (pnmh->hwndFrom == parent_window_)
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

void DockingDlgInterface::SendDialogInfoToNPP(int msg, int wParam) noexcept
{
#pragma warning(suppress : 26490)
    ::SendMessage(parent_window_, msg, wParam, reinterpret_cast<LPARAM>(dialogue_window_));
}

INT_PTR CALLBACK DockingDlgInterface::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
    auto *instance = reinterpret_cast<DockingDlgInterface *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (instance == nullptr)
    {
        return FALSE;
    }
    try
    {
        return instance->run_dlgProc(message, wParam, lParam);
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
            ::MessageBox(
                hwnd, L"Something terrible has gone wrong but I can't tell you what", instance->plugin_name_.c_str(), MB_OK | MB_ICONERROR);
        }
        return TRUE;
    }
}
