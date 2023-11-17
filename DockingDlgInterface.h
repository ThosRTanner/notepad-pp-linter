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

/** A slightly more explicit version of reinterpret_cast which requires
 * both types to be specified, and disables the cast warning.
 * 
 */
template <typename Target_Type, typename Orig_Type>
Target_Type cast_to(Orig_Type val) noexcept
{
#pragma warning(suppress : 26490)
    return reinterpret_cast<Target_Type>(val);
}

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

    /** Called when dialogue is to be displayed.
     * 
     * Remember to call the base class instance
     */
    virtual void display() noexcept;

    /** Called when dialogue is to be hidden
     * 
     * Remember to call the base class instance.
     */
    virtual void hide() noexcept;

    /** Called when dialogue is being moved or resized.
     * 
     * You'll need to implement this to move things around.
     */
    virtual void resize() noexcept = 0;

    /** Find out if the dialogue has been closed. */
    bool isClosed() const noexcept
    {
        return is_closed_;
    }

    /** Requests a redraw (by invalidating the whole dialogue area) */
    void request_redraw() const noexcept;

    /** Utility to get the current client rectangle */
    void getClientRect(RECT &rc) const noexcept;

    /** Utility to get the current window rectangle */
    void getWindowRect(RECT &rc) const noexcept;

    /** Utility to get a dialogue item */
    HWND GetDlgItem(int item) const noexcept;

    /** Utility to get hold of the current dialogue window handle */
    HWND get_handle() const noexcept
    {
        return dialogue_window_;
    }

  protected:
    virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM, LPARAM lParam);

  private:
    /** Utility wrapper round SendMessage to send pointers to our self */
    void SendDialogInfoToNPP(int msg, int wParam = 0) noexcept;

    static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept;

    HINSTANCE module_instance_;
    HWND parent_window_;
    HWND dialogue_window_;
    int docked_pos_ = 0;
    bool is_floating_ = true;
    bool is_closed_ = false;
    std::wstring module_name_;
    std::wstring plugin_name_;
};
