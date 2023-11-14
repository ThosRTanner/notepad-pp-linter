// This file is part of Notepad++ project
// Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <string>

class DockingDlgInterface
{
  public:
    /** Where to place dialogue initially */
    enum class Position
    {
        Dock_Left,
        Dock_Right,
        Dock_Top,
        Dock_Bottom,
        Floating
    };

    /** Create a docking dialogue.
     * 
     * dialogID is the resource number of the dialogue
     */
    DockingDlgInterface(int dialogID, HINSTANCE hInst, HWND npp_win);

    DockingDlgInterface(DockingDlgInterface const &) = delete;
    DockingDlgInterface(DockingDlgInterface &&) = delete;
    DockingDlgInterface &operator=(DockingDlgInterface const &) = delete;
    DockingDlgInterface &operator=(DockingDlgInterface &&) = delete;

    virtual ~DockingDlgInterface();

    /** Register dialogue with Notepad++.
     * 
     * I'm not a fan of 2-phase initialisation, but this bit has to be done after the
     * dialogue is actually created, or things go wrong
     *
     * dlg_num is the ID used to communicate with notepad++ (i.e. the menu entry)
     * extra is extra text to display on dialogue title.
     */
    void register_dialogue(int dlg_num, Position pos, HICON icon = nullptr, wchar_t const *extra = nullptr) noexcept;

    virtual void updateDockingDlg() noexcept;

    virtual void display() noexcept;

    virtual void hide() noexcept;

    virtual void resize() noexcept = 0;

    bool isClosed() const noexcept
    {
        return is_closed_;
    }

    void request_redraw(bool forceUpdate = false) const noexcept;

    void getClientRect(RECT &rc) const noexcept;

    void getWindowRect(RECT &rc) const noexcept;

    void paint() const noexcept;

  protected:
    virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM, LPARAM lParam);

  private:
    /** Utility wrapper round SendMessage to send pointers to our self */
    void SendDialogInfoToNPP(int msg, int wParam = 0) noexcept;

    static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept;

  protected:             //FIXME No!
    HINSTANCE module_instance_;
  private:
    HWND parent_window_;
  protected:            //FIXME No!
    HWND dialogue_window_;
  private:
    int docked_pos_ = 0;
    bool is_floating_ = true;
    bool is_closed_ = false;
    std::wstring module_name_;
    std::wstring plugin_name_;
};
