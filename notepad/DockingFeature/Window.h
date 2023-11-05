// This file is part of Notepad++ project
// Copyright (C)2022 Don HO <don.h@free.fr>

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
#include <windows.h>

class Window
{
  public:
    Window(HINSTANCE hInst, HWND parent) noexcept;

    Window(const Window &) = delete;
    Window(Window &&) = delete;

    virtual ~Window();

    virtual void destroy() = 0;

    virtual void display(bool toShow = true) const noexcept;

    void reSizeTo(RECT const &rc) const noexcept;

    void reSizeToWH(RECT const &rc) const noexcept;

    virtual void redraw(bool forceUpdate = false) const noexcept;

    void getClientRect(RECT &rc) const noexcept;

    void getWindowRect(RECT &rc) const noexcept;

    int getWidth() const noexcept;

    int getHeight() const noexcept;

    bool isVisible() const noexcept;

    HWND getHSelf() const noexcept
    {
        return _hSelf;
    }

    HWND getHParent() const noexcept
    {
        return _hParent;
    }

    void setFocus() const noexcept
    {
        ::SetFocus(_hSelf);
    }

    HINSTANCE getHinst() const noexcept
    {
        return _hInst;
    }

    Window &operator=(const Window &) = delete;
    Window &operator=(Window &&) = delete;


  protected:
    HINSTANCE _hInst = NULL;
    HWND _hParent = NULL;
    HWND _hSelf = NULL;
};
