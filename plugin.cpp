#include "stdafx.h"
#include "plugin.h"
#include "linter.h"

#include "OutputDialog.h"
#include "XmlParser.h"

#include "notepad/Scintilla.h"

#include <memory>
#include <string>

HANDLE timers(0);

std::unique_ptr<Linter::OutputDialog> output_dialogue;

namespace
{
    HANDLE module_handle;
    const TCHAR PLUGIN_NAME[] = L"Linter";
    TCHAR iniFilePath[MAX_PATH];

    static const int FUNCTIONS_COUNT = 2;
    FuncItem funcItem[FUNCTIONS_COUNT];

    NppData nppData;

    void pluginInit(HANDLE module)
    {
        module_handle = module;
        timers = CreateTimerQueue();
    }

    void pluginCleanUp()
    {
    }

    bool setCommand(size_t index, wchar_t const *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool checkOnInit)
    {
        if (index >= FUNCTIONS_COUNT)
        {
            return false;
        }

        if (!pFunc)
        {
            return false;
        }

        lstrcpy(funcItem[index]._itemName, cmdName);
        funcItem[index]._pFunc = pFunc;
        funcItem[index]._init2Check = checkOnInit;
        funcItem[index]._pShKey = sk;

        return true;
    }

    void show_results()
    {
        output_dialogue->display();
    }

    void commandMenuInit()
    {
        setCommand(0, L"Edit config", editConfig, nullptr, false);
        setCommand(1, L"Show linter results", show_results, nullptr, false);
        output_dialogue.reset(new Linter::OutputDialog(nppData, module_handle, 1));
    }

    void ShowError(LRESULT start, LRESULT end, bool on)
    {
        LRESULT oldid = SendEditor(SCI_GETINDICATORCURRENT);
        SendEditor(SCI_SETINDICATORCURRENT, SCE_SQUIGGLE_UNDERLINE_RED);
        SendEditor(on ? SCI_INDICATORFILLRANGE : SCI_INDICATORCLEARRANGE, start, (end - start));
        SendEditor(SCI_SETINDICATORCURRENT, oldid);
    }

}    // namespace

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reasonForCall, LPVOID /*lpReserved*/)
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
            break;

        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
    nppData = notpadPlusData;
    initConfig();
    commandMenuInit();
}

extern "C" __declspec(dllexport) const TCHAR *getName()
{
    return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem *getFuncsArray(int *nbF)
{
    *nbF = FUNCTIONS_COUNT;
    return funcItem;
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT /*Message*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}

void commandMenuCleanUp()
{
}

void initConfig()
{
    ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, reinterpret_cast<LPARAM>(iniFilePath));
    if (!PathFileExists(iniFilePath))
    {
        ::CreateDirectory(iniFilePath, nullptr);
    }
    PathAppend(iniFilePath, L"linter.xml");
}

void editConfig()
{
    SendApp(NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(iniFilePath));
}

wchar_t const *getIniFileName()
{
    return iniFilePath;
}

HWND getScintillaWindow()
{
    LRESULT view = SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0);
    return view == 0 ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}

LRESULT SendEditor(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    HWND wEditor = getScintillaWindow();
    return SendMessage(wEditor, Msg, wParam, lParam);
}

LRESULT SendApp(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return SendMessage(nppData._nppHandle, Msg, wParam, lParam);
}

std::string getDocumentText()
{
    LRESULT lengthDoc = SendEditor(SCI_GETLENGTH);
#if __cplusplus >= 202002L
    auto buff{std::make_unique_for_overwrite<char[]>(lengthDoc + 1)};
#else
    std::unique_ptr<char[]> buff{new char[lengthDoc + 1]};
#endif
    SendEditor(SCI_GETTEXT, lengthDoc, reinterpret_cast<LPARAM>(buff.get()));
    return std::string(buff.get(), lengthDoc);
}

std::string getLineText(int line)
{
    LRESULT length = SendEditor(SCI_LINELENGTH, line);
#if __cplusplus >= 202002L
    auto buff{std::make_unique_for_overwrite<char[]>(length + 1)};
#else
    std::unique_ptr<char[]> buff{new char[length + 1]};
#endif
    SendEditor(SCI_GETLINE, line, reinterpret_cast<LPARAM>(buff.get()));
    return std::string(buff.get(), length);
}

LRESULT getPositionForLine(int line)
{
    return SendEditor(SCI_POSITIONFROMLINE, line);
}

void ShowError(LRESULT pos)
{
    ShowError(pos, pos + 1, true);
}

void HideErrors()
{
    ShowError(0, SendEditor(SCI_GETLENGTH), false);
}
