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

#include "Docking.h" //For tTbData. should we fwd declare this?
#include "StaticDialog.h"

//#include <shlwapi.h>
#include <string>

class DockingDlgInterface : public StaticDialog
{
  public:
    DockingDlgInterface();

    explicit DockingDlgInterface(int dlgID) noexcept;

    void init(HINSTANCE hInst, HWND parent) override;

    void create(tTbData *data, bool isRTL = false);

    virtual void updateDockingDlg() noexcept;

    void destroy() noexcept override
    {
    }

    virtual void setBackgroundColor(COLORREF) noexcept
    {
    }
    virtual void setForegroundColor(COLORREF) noexcept
    {
    }

    void display(bool toShow = true) const noexcept override;

    bool isClosed() const noexcept
    {
        return _isClosed;
    }

    void setClosed(bool toClose) noexcept
    {
        _isClosed = toClose;
    }

    const TCHAR *getPluginFileName() const noexcept
    {
        return _moduleName.c_str();
    }

  protected:
    int _dlgID = -1;
    bool _isFloating = true;
    int _iDockedPos = 0;
    std::wstring _moduleName;
    std::wstring _pluginName;
    bool _isClosed = false;

    INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM, LPARAM lParam) noexcept override;
};
