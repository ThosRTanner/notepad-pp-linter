#include "Clipboard.h"

#include "SystemError.h"

namespace Linter
{

Clipboard::Clipboard(HWND self)
{
    if (not ::OpenClipboard(self))
    {
        throw SystemError("Cannot open the Clipboard");
    }
}

Clipboard::~Clipboard()
{
    if (mem_handle_ != nullptr)
    {
        ::GlobalFree(mem_handle_);
    }
    ::CloseClipboard();
}

void Clipboard::empty()
{
    if (not ::EmptyClipboard())
    {
        throw SystemError("Cannot empty the Clipboard");
    }
}

void Clipboard::copy(std::wstring const &str)
{
    size_t const size = str.size() * sizeof(str[0]);
    mem_handle_ = ::GlobalAlloc(GMEM_MOVEABLE, size);
    if (mem_handle_ == nullptr)
    {
        throw SystemError("Cannot allocate memory for clipboard");
    }

    LPVOID mem_ptr_ = ::GlobalLock(mem_handle_);
    if (mem_ptr_ == nullptr)
    {
        throw SystemError("Cannot lock memory for clipboard");
    }

    std::memcpy(mem_ptr_, str.c_str(), size);

    ::GlobalUnlock(mem_handle_);

    if (::SetClipboardData(CF_UNICODETEXT, mem_handle_) == nullptr)
    {
        throw SystemError("Unable to set Clipboard data");
    }

    mem_handle_ = nullptr;
}

}    // namespace Linter
