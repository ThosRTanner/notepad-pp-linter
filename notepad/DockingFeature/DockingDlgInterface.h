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

#include "dockingResource.h"
#include "Docking.h"
#include "StaticDialog.h"

#include <shlwapi.h>
#include <string>



class DockingDlgInterface : public StaticDialog
{
public:
	DockingDlgInterface() = default;
	explicit DockingDlgInterface(int dlgID) noexcept : _dlgID(dlgID) {}

	void init(HINSTANCE hInst, HWND parent) override
    {
		StaticDialog::init(hInst, parent);
		TCHAR temp[MAX_PATH];
        ::GetModuleFileName(static_cast<HMODULE>(hInst), &temp[0], MAX_PATH);
        _moduleName = ::PathFindFileName(&temp[0]);
	}

    void create(tTbData* data, bool isRTL = false) {
		assert(data != nullptr);
		StaticDialog::create(_dlgID, isRTL);
		TCHAR temp[MAX_PATH];
        ::GetWindowText(_hSelf, &temp[0], MAX_PATH);
		_pluginName = &temp[0];

        // user information
		data->hClient = _hSelf;
		data->pszName = _pluginName.c_str();

		// supported features by plugin
		data->uMask = 0;

		// additional info
		data->pszAddInfo = NULL;
	}

	virtual void updateDockingDlg() noexcept {
		::SendMessage(_hParent, NPPM_DMMUPDATEDISPINFO, 0, reinterpret_cast<LPARAM>(_hSelf));
	}

    void destroy() noexcept override {}

	virtual void setBackgroundColor(COLORREF) noexcept {}
	virtual void setForegroundColor(COLORREF) noexcept {}

	void display(bool toShow = true) const noexcept override {
		::SendMessage(_hParent, toShow ? NPPM_DMMSHOW : NPPM_DMMHIDE, 0, reinterpret_cast<LPARAM>(_hSelf));
	}

	bool isClosed() const noexcept {
		return _isClosed;
	}

	void setClosed(bool toClose) noexcept {
		_isClosed = toClose;
	}

	const TCHAR * getPluginFileName() const noexcept {
		return _moduleName.c_str();
	}

protected :
	int	_dlgID = -1;
	bool _isFloating = true;
	int _iDockedPos = 0;
	std::wstring _moduleName;
	std::wstring _pluginName;
	bool _isClosed = false;

	INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM, LPARAM lParam) noexcept override {
		switch (message)
		{
			case WM_NOTIFY: 
			{
				NMHDR const *pnmh = reinterpret_cast<LPNMHDR>(lParam);

				if (pnmh->hwndFrom == _hParent)
				{
					switch (LOWORD(pnmh->code))
					{
						case DMN_CLOSE:
						{
							break;
						}
						case DMN_FLOAT:
						{
							_isFloating = true;
							break;
						}
						case DMN_DOCK:
						{
							_iDockedPos = HIWORD(pnmh->code);
							_isFloating = false;
							break;
						}
						default:
							break;
					}
				}
				break;
			}
			default:
				break;
		}
		return FALSE;
	};
};
