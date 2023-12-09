#include "stdafx.h"
#include "Clipboard.h"

#include "SystemError.h"

Linter::Clipboard::Clipboard(HWND self)
{
    if (!::OpenClipboard(self))
    {
        throw SystemError("Cannot open the Clipboard");
    }
}

Linter::Clipboard::~Clipboard()
{
    if (mem_handle_ != nullptr)
    {
        ::GlobalFree(mem_handle_);
    }
    ::CloseClipboard();
}

void Linter::Clipboard::empty()
{
    if (!::EmptyClipboard())
    {
        throw SystemError("Cannot empty the Clipboard");
    }
}

void Linter::Clipboard::copy(std::wstring const &str)
{
    size_t const size = str.size() * sizeof(TCHAR);
    mem_handle_ = ::GlobalAlloc(GMEM_MOVEABLE, size);
    if (mem_handle_ == nullptr)
    {
        throw SystemError("Cannot allocate memory for clipboard");
    }

    LPVOID lpsz = ::GlobalLock(mem_handle_);
    if (lpsz == nullptr)
    {
        throw SystemError("Cannot lock memory for clipboard");
    }

    std::memcpy(lpsz, str.c_str(), size);

    ::GlobalUnlock(mem_handle_);

    if (::SetClipboardData(CF_UNICODETEXT, mem_handle_) == nullptr)
    {
        throw SystemError("Unable to set Clipboard data");
    }

    mem_handle_ = nullptr;
}
