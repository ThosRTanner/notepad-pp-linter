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
#include "..\Notepad_plus_msgs.h"
#include "Window.h"

typedef HRESULT (WINAPI * ETDTProc) (HWND, DWORD);

enum class PosAlign { left, right, top, bottom };

struct DLGTEMPLATEEX
{
      WORD   dlgVer;
      WORD   signature;
      DWORD  helpID;
      DWORD  exStyle;
      DWORD  style;
      WORD   cDlgItems;
      short  x;
      short  y;
      short  cx;
      short  cy;
      // The structure has more fields but are variable length
};

class StaticDialog : public Window
{
public :
	virtual ~StaticDialog();

	virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true);

    virtual bool isCreated() const noexcept {
		return (_hSelf != NULL);
	}

	void goToCenter() noexcept;

	void display(bool toShow = true, bool enhancedPositioningCheckWhenShowing = false) const noexcept;

	RECT getViewablePositionRect(RECT testRc) const noexcept;

	POINT getTopPoint(HWND hwnd, bool isLeft = true) const noexcept;

	bool isCheckedOrNot(int checkControlID) const noexcept
	{
		return (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, checkControlID), BM_GETCHECK, 0, 0));
	}

	void setChecked(int checkControlID, bool checkOrNot = true) const noexcept
	{
		::SendDlgItemMessage(_hSelf, checkControlID, BM_SETCHECK, checkOrNot ? BST_CHECKED : BST_UNCHECKED, 0);
	}

    void destroy() noexcept override;

protected:
	RECT _rc;
	static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) = 0;

    void alignWith(HWND handle, HWND handle2Align, PosAlign pos, POINT &point) noexcept;
    HGLOBAL makeRTLResource(int dialogID, DLGTEMPLATE **ppMyDlgTemplate) noexcept;
};
