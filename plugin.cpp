#include "stdafx.h"
#include "plugin.h"

#include "linter.h"
#include "OutputDialog.h"
#include "XmlParser.h"

#include "notepad/PluginInterface.h"

#include <Shlwapi.h>

#include <memory>
#include <string>

#pragma comment(lib, "msxml6.lib")
#pragma comment(lib, "shlwapi.lib")

HANDLE timers(0);

std::unique_ptr<Linter::OutputDialog> output_dialogue;

enum
{
    MENU_ENTRY_EDIT_CONFIG,
    MENU_ENTRY_SHOW_RESULTS
};

namespace
{
    HANDLE module_handle;
    constexpr auto PLUGIN_NAME = L"Linter";
    TCHAR iniFilePath[MAX_PATH];

    NppData nppData;

    void pluginInit(HANDLE module) noexcept
    {
        module_handle = module;
        timers = CreateTimerQueue();
    }

    void pluginCleanUp() noexcept
    {
    }

    void show_results() noexcept
    {
        if (output_dialogue)
        {
            output_dialogue->display();
        }
        else
        {
            ::MessageBox(nppData._nppHandle, L"Unable to show lint errors due to startup issue", PLUGIN_NAME, MB_OK | MB_ICONERROR);
        }
    }

    void commandMenuInit()
    {
        output_dialogue = std::make_unique<Linter::OutputDialog>(module_handle, nppData._nppHandle, MENU_ENTRY_SHOW_RESULTS);
    }

    void ShowError(LRESULT start, LRESULT end, bool on) noexcept
    {
        LRESULT const oldid = SendEditor(SCI_GETINDICATORCURRENT);
        SendEditor(SCI_SETINDICATORCURRENT, SCE_SQUIGGLE_UNDERLINE_RED);
        SendEditor(on ? SCI_INDICATORFILLRANGE : SCI_INDICATORCLEARRANGE, start, end - start);
        SendEditor(SCI_SETINDICATORCURRENT, oldid);
    }

}    // namespace

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reasonForCall, LPVOID /*lpReserved*/) noexcept
{
    switch (reasonForCall)
    {
        case DLL_PROCESS_ATTACH:
            pluginInit(hModule);
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

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData) noexcept
{
    try
    {
        nppData = notpadPlusData;
        initConfig();
        commandMenuInit();
    }
    catch (std::exception const &e)
    {
        try
        {
            ::MessageBox(
                notpadPlusData._nppHandle, static_cast<wchar_t const *>(static_cast<bstr_t>(e.what())), PLUGIN_NAME, MB_OK | MB_ICONERROR);
        }
        catch (std::exception const &)
        {
            ::MessageBox(notpadPlusData._nppHandle,
                L"Something terrible has gone wrong but I can't tell you what",
                PLUGIN_NAME,
                MB_OK | MB_ICONERROR);
        }
    }
}

extern "C" __declspec(dllexport) const TCHAR *getName() noexcept
{
    return &PLUGIN_NAME[0];
}

extern "C" __declspec(dllexport) FuncItem *getFuncsArray(int *nbF) noexcept
{
    int constexpr FUNCTIONS_COUNT = 2;
    static FuncItem funcItem[FUNCTIONS_COUNT] = {
        {L"Edit config",         editConfig,   MENU_ENTRY_EDIT_CONFIG },
        {L"Show linter results", show_results, MENU_ENTRY_SHOW_RESULTS}
    };

    *nbF = FUNCTIONS_COUNT;
    return &funcItem[0];
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT /*Message*/, WPARAM /*wParam*/, LPARAM /*lParam*/) noexcept
{
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL isUnicode() noexcept
{
    return TRUE;
}

void commandMenuCleanUp() noexcept
{
}

void initConfig() noexcept
{
    auto const fileName{&iniFilePath[0]};
    SendApp(NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, fileName);
    if (!PathFileExists(fileName))
    {
        ::CreateDirectory(fileName, nullptr);
    }
    PathAppend(fileName, L"linter.xml");
}

void editConfig() noexcept
{
    SendApp(NPPM_DOOPEN, 0, getIniFileName());
}

wchar_t const *getIniFileName() noexcept
{
    return &iniFilePath[0];
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
#if __cplusplus >= 202002L
    auto buff{std::make_unique_for_overwrite<char[]>(lengthDoc + 1)};
#else
#pragma warning(suppress : 26409 26414)
    std::unique_ptr<char[]> buff{new char[lengthDoc + 1]};
#endif
    SendEditor(SCI_GETTEXT, lengthDoc, buff.get());
    return std::string(buff.get(), lengthDoc);
}

std::string getLineText(int line)
{
    LRESULT const length = SendEditor(SCI_LINELENGTH, line);
#if __cplusplus >= 202002L
    auto buff{std::make_unique_for_overwrite<char[]>(length + 1)};
#else
#pragma warning(suppress : 26409 26414)
    std::unique_ptr<char[]> buff{new char[length + 1]};
#endif
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
