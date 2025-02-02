#include "Handle_Wrapper.h"

#include "System_Error.h"

#include <errhandlingapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <intsafe.h>
#include <winerror.h>

#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace Linter
{

Handle_Wrapper::Handle_Wrapper(HANDLE h) : handle_(h)
{
    if (h == INVALID_HANDLE_VALUE)
    {
        throw System_Error();
    }
}

Handle_Wrapper::Handle_Wrapper(Handle_Wrapper &&other) noexcept :
    handle_(std::exchange(other.handle_, INVALID_HANDLE_VALUE))
{
}

Handle_Wrapper::~Handle_Wrapper()
{
    close();
}

void Handle_Wrapper::close() const noexcept
{
    if (handle_ != INVALID_HANDLE_VALUE)
    {
        HANDLE h{std::exchange(handle_, INVALID_HANDLE_VALUE)};
        CloseHandle(h);
    }
}

Handle_Wrapper::operator HANDLE() const noexcept
{
    return handle_;
}

void Handle_Wrapper::writeFile(std::string const &str) const
{
    static_assert(sizeof(str[0]) == 1, "Invalid byte size");

    auto start = str.begin();
    auto const end = str.end();
    while (start != end)
    {
        auto const toWrite = static_cast<DWORD>(std::min(
            static_cast<std::ptrdiff_t>(std::numeric_limits<DWORD>::max()),
            end - start
        ));
        DWORD written;
        if (! WriteFile(handle_, &*start, toWrite, &written, nullptr))
        {
            throw System_Error();
        }
        start += written;
    }
}

std::string Handle_Wrapper::readFile() const
{
    std::string result;

    constexpr DWORD BUFFSIZE = 0x4000;
    std::vector<char> buffer;
    buffer.resize(BUFFSIZE);
    auto const buff{&*buffer.begin()};

    for (;;)
    {
        DWORD bytes_read;
        // The API suggests when the other end closes the pipe, you should get
        // 0. What appears to happen is that you get broken pipe.
        if (! ReadFile(handle_, buff, BUFFSIZE, &bytes_read, nullptr))
        {
            DWORD const err = GetLastError();
            if (err != ERROR_BROKEN_PIPE)
            {
                throw System_Error(err);
            }
        }

        if (bytes_read == 0)
        {
            break;
        }

        result.append(buff, bytes_read);
    }

    return result;
}

}    // namespace Linter
