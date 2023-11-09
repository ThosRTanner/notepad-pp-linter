#include "stdafx.h"
#include "Window.h"

Window::Window(HINSTANCE hInst, HWND parent) noexcept : _hInst(hInst), _hParent(parent)
{
}

Window::~Window() = default;

void Window::display(bool toShow) const noexcept
{
    ::ShowWindow(_hSelf, toShow ? SW_SHOW : SW_HIDE);
}

void Window::request_redraw(bool forceUpdate) const noexcept
{
    ::InvalidateRect(_hSelf, nullptr, TRUE);
    if (forceUpdate)
    {
        ::UpdateWindow(_hSelf);
    }
}

void Window::getClientRect(RECT &rc) const noexcept
{
    ::GetClientRect(_hSelf, &rc);
}

void Window::getWindowRect(RECT &rc) const noexcept
{
    ::GetWindowRect(_hSelf, &rc);
}

void Window::paint() const noexcept
{
    ::RedrawWindow(_hSelf, nullptr, nullptr, RDW_INVALIDATE);
}
