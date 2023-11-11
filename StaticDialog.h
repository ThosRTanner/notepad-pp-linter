#pragma once
//#include "notepad/Notepad_plus_msgs.h"

class StaticDialog
{
  public:
    StaticDialog(HINSTANCE hInst, HWND paren, int dialogID);

    virtual ~StaticDialog();

    StaticDialog(StaticDialog const &) = delete;
    StaticDialog(StaticDialog &&) = delete;
    StaticDialog &operator=(StaticDialog const &) = delete;
    StaticDialog &operator=(StaticDialog &&) = delete;

    void request_redraw(bool forceUpdate = false) const noexcept;

    void getClientRect(RECT &rc) const noexcept;

    void getWindowRect(RECT &rc) const noexcept;

    void paint() const noexcept;

    void SendDialogInfoToNPP(int msg, int wParam = 0) noexcept;

  protected:
    virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) = 0;

    //  private: why?
    HINSTANCE _hInst;    //module_instance_
    HWND _hParent;       //npp_window_
    HWND _hSelf;         //self_

  private:
    static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept;

};
