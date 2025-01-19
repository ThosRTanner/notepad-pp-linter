#include "stdafx.h"
#include "plugin_main.h"

#include "OutputDialog.h"
#include "linter.h"

#include "notepad++/Notepad_plus_msgs.h"
#include "notepad++/PluginInterface.h"
#include "notepad++/Scintilla.h"

#include <Shlwapi.h>
#include <comutil.h>

#include <exception>
#include <memory>
#include <string>

#pragma comment(lib, "msxml6.lib")
#pragma comment(lib, "shlwapi.lib")

HANDLE timers(0);

std::unique_ptr<Linter::OutputDialog> output_dialogue;

namespace
{
    HINSTANCE module_handle;
    constexpr auto PLUGIN_NAME = L"Linter";

    NppData nppData;

    void pluginInit(HINSTANCE module) noexcept
    {
        module_handle = module;
//        timers = CreateTimerQueue();
    }

    void pluginCleanUp() noexcept
    {
    }

    void ShowError(LRESULT start, LRESULT end, bool on) noexcept
    {
        LRESULT const oldid = SendEditor(SCI_GETINDICATORCURRENT);
        SendEditor(SCI_SETINDICATORCURRENT, SCE_SQUIGGLE_UNDERLINE_RED);
        SendEditor(on ? SCI_INDICATORFILLRANGE : SCI_INDICATORCLEARRANGE, start, end - start);
        SendEditor(SCI_SETINDICATORCURRENT, oldid);
    }

}    // namespace

int __stdcall DllMain(_In_ void *hModule, _In_ unsigned long reasonForCall, _In_opt_ void *) noexcept
{
    switch (reasonForCall)
    {
        case DLL_PROCESS_ATTACH:
            pluginInit(static_cast<HINSTANCE>(hModule));
            break;

        case DLL_PROCESS_DETACH:
            pluginCleanUp();
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        default:
            break;
    }

    return TRUE;
}

void commandMenuCleanUp() noexcept
{
}

HWND getScintillaWindow() noexcept
{
    LRESULT const view = SendApp(NPPM_GETCURRENTVIEW);
    return view == 0 ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}

LRESULT SendEditor(UINT Msg, WPARAM wParam, LPARAM lParam) noexcept
{
    return SendMessage(getScintillaWindow(), Msg, wParam, lParam);
}

LRESULT SendApp(UINT Msg, WPARAM wParam, LPARAM lParam) noexcept
{
    return SendMessage(nppData._nppHandle, Msg, wParam, lParam);
}

std::string getDocumentText()
{
    LRESULT const lengthDoc = SendEditor(SCI_GETLENGTH);
    auto buff{std::make_unique_for_overwrite<char[]>(lengthDoc + 1)};
    SendEditor(SCI_GETTEXT, lengthDoc, buff.get());
    return std::string(buff.get(), lengthDoc);
}

std::string getLineText(int line)
{
    LRESULT const length = SendEditor(SCI_LINELENGTH, line);
    auto buff{std::make_unique_for_overwrite<char[]>(length + 1)};
    SendEditor(SCI_GETLINE, line, buff.get());
    return std::string(buff.get(), length);
}

LRESULT getPositionForLine(int line) noexcept
{
    return SendEditor(SCI_POSITIONFROMLINE, line);
}

void ShowError(LRESULT pos) noexcept
{
    ShowError(pos, pos + 1, true);
}

void HideErrors() noexcept
{
    ShowError(0, SendEditor(SCI_GETLENGTH), false);
}
