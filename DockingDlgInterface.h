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

#include "StaticDialog.h"

#include <string>

class DockingDlgInterface : public StaticDialog
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

    /** Register dialogue with Notepad++.
     * 
     * I'm not a fan of 2-phase initialisation, but this bit has to be done after the
     * dialogue is actually created, or things go wrong
     *
     * dlg_num is the ID used to communicate with notepad++ (i.e. the menu entry)
     * extra is extra text to display on dialogue title.

     */
    void register_dialogue(int dlg_num, Position pos, HICON icon = nullptr, wchar_t const *extra = nullptr);

    virtual void updateDockingDlg() noexcept;

    virtual void display() noexcept;

    virtual void hide() noexcept;

    virtual void resize() noexcept = 0;

    bool isClosed() const noexcept
    {
        return is_closed_;
    }

  protected:
    INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM, LPARAM lParam) override;

  private:
    std::wstring module_name_;
    std::wstring plugin_name_;
    int docked_pos_ = 0;
    bool is_floating_ = true;
    bool is_closed_ = false;
};
