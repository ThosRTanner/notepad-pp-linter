#include "Clipboard.h"

#include "System_Error.h"

#include <winbase.h>
#include <winuser.h>

#include <cstring>
#include <string>

namespace Linter
{

Clipboard::Clipboard(HWND self)
{
    if (::OpenClipboard(self) == FALSE)
    {
        throw System_Error("Cannot open the Clipboard");
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

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Clipboard::empty()
{
    if (::EmptyClipboard() == FALSE)
    {
        throw System_Error("Cannot empty the Clipboard");
    }
}

void Clipboard::copy(std::wstring const &str)
{
    size_t const size = str.size() * sizeof(str[0]);
    mem_handle_ = ::GlobalAlloc(GMEM_MOVEABLE, size);
    if (mem_handle_ == nullptr)
    {
        throw System_Error("Cannot allocate memory for clipboard");
    }

    LPVOID mem_ptr_ = ::GlobalLock(mem_handle_);
    if (mem_ptr_ == nullptr)
    {
        throw System_Error("Cannot lock memory for clipboard");
    }

    std::memcpy(mem_ptr_, str.c_str(), size);

    ::GlobalUnlock(mem_handle_);

    if (::SetClipboardData(CF_UNICODETEXT, mem_handle_) == nullptr)
    {
        throw System_Error("Unable to set Clipboard data");
    }

    mem_handle_ = nullptr;
}

}    // namespace Linter
