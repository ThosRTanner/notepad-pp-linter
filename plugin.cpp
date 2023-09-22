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

    bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit)
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
        funcItem[index]._init2Check = check0nInit;
        funcItem[index]._pShKey = sk;

        return true;
    }

    void editConfig()
    {
        static bool called = false;
        if (!called)
        {
            called = true;
            return;
        }
        ::SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)iniFilePath);
    }

    void show_results()
    {
        output_dialogue->display();
    }

    void commandMenuInit()
    {
        setCommand(0, bstr_t(L"Edit config"), editConfig, NULL, false);
        setCommand(1, bstr_t(L"Show linter results"), show_results, NULL, false);
        output_dialogue.reset(new Linter::OutputDialog(nppData, module_handle, 1));
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

TCHAR *getIniFileName()
{
    return iniFilePath;
}

void initConfig()
{
    ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)iniFilePath);
    if (PathFileExists(iniFilePath) == FALSE)
    {
        ::CreateDirectory(iniFilePath, NULL);
    }
    PathAppend(iniFilePath, L"linter.xml");
}

void commandMenuCleanUp()
{
}

HWND getScintillaWindow()
{
    int which = -1;
    SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1)
    {
        return NULL;
    }
    return (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
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
    LRESULT lengthDoc = SendEditor(SCI_GETLENGTH) + 1;

    char *buff = new char[lengthDoc];
    SendEditor(SCI_GETTEXT, lengthDoc, (LPARAM)buff);
    std::string text(buff, lengthDoc);
    text = text.c_str();
    delete[] buff;
    return text;
}

std::string getLineText(int line)
{
    LRESULT length = SendEditor(SCI_LINELENGTH, line);

    char *buff = new char[length + 1];
    SendEditor(SCI_GETLINE, line, (LPARAM)buff);
    std::string text(buff, length);
    text = text.c_str();
    delete[] buff;
    return text;
}

LRESULT getPositionForLine(int line)
{
    return SendEditor(SCI_POSITIONFROMLINE, line);
}

void ShowError(LRESULT start, LRESULT end, bool off)
{
    LRESULT oldid = SendEditor(SCI_GETINDICATORCURRENT);
    SendEditor(SCI_SETINDICATORCURRENT, SCE_SQUIGGLE_UNDERLINE_RED);
    if (off)
    {
        SendEditor(SCI_INDICATORFILLRANGE, start, (end - start));
    }
    else
    {
        SendEditor(SCI_INDICATORCLEARRANGE, start, (end - start));
    }
    SendEditor(SCI_SETINDICATORCURRENT, oldid);
}
