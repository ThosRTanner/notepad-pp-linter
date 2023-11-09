#pragma once
#include "notepad/Notepad_plus_msgs.h"
#include "Window.h"

class StaticDialog : public Window
{
  public:
    StaticDialog(HINSTANCE hInst, HWND paren, int dialogID);

    virtual ~StaticDialog();

    StaticDialog(StaticDialog const &) = delete;
    StaticDialog(StaticDialog &&) = delete;
    StaticDialog &operator=(StaticDialog const &) = delete;
    StaticDialog &operator=(StaticDialog &&) = delete;

    void display(bool toShow = true, bool enhancedPositioningCheckWhenShowing = false) const noexcept;

  protected:
    virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) = 0;

  private:
    static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept;

    RECT getViewablePositionRect(RECT testRc) const noexcept;
};
