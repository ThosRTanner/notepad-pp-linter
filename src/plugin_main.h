#pragma once

#include "notepad++/Scintilla.h"

#include <memory>
#include <string>

namespace Linter
{
class Output_Dialogue;
}
extern Linter::Output_Dialogue *output_dialogue;

struct NppData;
void set_legacy_nppdata(NppData const &);

int constexpr SCE_SQUIGGLE_UNDERLINE_RED = INDIC_CONTAINER + 2;

HWND getScintillaWindow() noexcept;

LRESULT SendEditor(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0) noexcept;

inline LRESULT SendEditor(UINT Msg, WPARAM wParam, void const *lParam) noexcept
{
#pragma warning(suppress : 26490)
    return SendEditor(Msg, wParam, reinterpret_cast<LPARAM>(lParam));
}

LRESULT SendApp(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0) noexcept;

inline LRESULT SendApp(UINT Msg, WPARAM wParam, void const *lParam) noexcept
{
#pragma warning(suppress : 26490)
    return SendApp(Msg, wParam, reinterpret_cast<LPARAM>(lParam));
}

std::string getDocumentText();
std::string getLineText(int line);
LRESULT getPositionForLine(int line) noexcept;

void ShowError(LRESULT pos) noexcept;
void HideErrors() noexcept;
