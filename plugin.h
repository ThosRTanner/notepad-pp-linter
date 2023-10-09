#pragma once

#include "notepad/PluginInterface.h"

#include <memory>
#include <string>

namespace Linter
{
    class OutputDialog;
}

extern HANDLE timers;
extern std::unique_ptr<Linter::OutputDialog> output_dialogue;

static const int SCE_SQUIGGLE_UNDERLINE_RED = INDIC_CONTAINER + 2;

void commandMenuCleanUp();
void initConfig();

wchar_t const *getIniFileName();

HWND getScintillaWindow();
LRESULT SendEditor(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
LRESULT SendApp(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);

std::string getDocumentText();
std::string getLineText(int line);
LRESULT getPositionForLine(int line);

void ShowError(LRESULT start, LRESULT end, bool on);

inline void ShowError(LRESULT pos)
{
    ShowError(pos, pos + 1, true);
}

inline void HideErrors()
{
    ShowError(0, SendEditor(SCI_GETLENGTH), false);
}
