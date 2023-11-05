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

void Window::reSizeTo(RECT const &rc) const noexcept
{
    ::MoveWindow(_hSelf, rc.left, rc.top, rc.right, rc.bottom, TRUE);
    redraw();
}

void Window::reSizeToWH(RECT const &rc) const noexcept
{
    ::MoveWindow(_hSelf, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    redraw();
}

void Window::redraw(bool forceUpdate) const noexcept
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

int Window::getWidth() const noexcept
{
    RECT rc;
    getClientRect(rc);
    return rc.right - rc.left;
}

int Window::getHeight() const noexcept
{
    RECT rc;
    getClientRect(rc);
    return rc.bottom - rc.top;
}

bool Window::isVisible() const noexcept
{
    return ::IsWindowVisible(_hSelf) ? true : false;
}
