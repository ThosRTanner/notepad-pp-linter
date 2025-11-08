#include "Handle_Wrapper.h"

#include "System_Error.h"

#include <fileapi.h>
#include <handleapi.h>
#include <intsafe.h>

#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace Linter
{

Handle_Wrapper::Handle_Wrapper(HANDLE hand) : handle_(hand)
{
    if (hand == INVALID_HANDLE_VALUE)
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
        CloseHandle(std::exchange(handle_, INVALID_HANDLE_VALUE));
    }
}

Handle_Wrapper::operator HANDLE() const noexcept
{
    return handle_;
}

void Handle_Wrapper::write_file(std::string const &str) const
{
    auto start = str.begin();
    auto const end = str.end();

    while (start != end)
    {
        // Very annoyingly, WriteFile() takes a DWORD for the number of bytes to
        // write, and we have two pointers to work with. The difference between
        // two pointers is a ptrdiff_t, which is 32 bits signed on 32-bit
        // systems and 64 bits signed on 64-bit systems. However, a DWORD is
        // always 32 bits unsigned, so we have to be careful not to overflow it.
        // As we know end can never be less than start, however, we can safely
        // cast end - start to unsigned long long and then downcast the result
        // of std::min
        DWORD const to_write = static_cast<DWORD>(std::min(
            static_cast<unsigned long long>(end - start),
            static_cast<unsigned long long>(std::numeric_limits<DWORD>::max())
        ));

        DWORD written;    // NOLINT(cppcoreguidelines-init-variables)
        if (not WriteFile(handle_, &*start, to_write, &written, nullptr))
        {
            throw System_Error();
        }
        start += written;
    }
}

std::string Handle_Wrapper::read_file() const
{
    std::string result;

    constexpr DWORD BUFFSIZE = 0x4000;
    std::vector<char> buffer;
    buffer.resize(BUFFSIZE);
    auto * const buff{&*buffer.begin()};

    for (;;)
    {
        DWORD bytes_read; // NOLINT(cppcoreguidelines-init-variables)
        if (not ReadFile(handle_, buff, BUFFSIZE, &bytes_read, nullptr))
        {
            throw System_Error();
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
