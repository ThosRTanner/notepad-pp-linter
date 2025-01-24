#include "HandleWrapper.h"

#include "SystemError.h"

#include <utility>
#include <vector>

namespace Linter
{

HandleWrapper::HandleWrapper(HANDLE h) : m_handle(h)
{
    if (h == INVALID_HANDLE_VALUE)
    {
        throw SystemError();
    }
}

HandleWrapper::HandleWrapper(HandleWrapper &&other) noexcept :
    m_handle(std::exchange(other.m_handle, INVALID_HANDLE_VALUE))
{
}

void HandleWrapper::close() const noexcept
{
    if (m_handle != INVALID_HANDLE_VALUE)
    {
        HANDLE h{std::exchange(m_handle, INVALID_HANDLE_VALUE)};
        CloseHandle(h);
    }
}

HandleWrapper::operator HANDLE() const noexcept
{
    return m_handle;
}

HandleWrapper::~HandleWrapper()
{
    close();
}

void HandleWrapper::writeFile(std::string const &str) const
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
        if (! WriteFile(m_handle, &*start, toWrite, &written, nullptr))
        {
            throw SystemError();
        }
        start += written;
    }
}

std::string HandleWrapper::readFile() const
{
    std::string result;

    constexpr DWORD BUFFSIZE = 0x4000;
    std::vector<char> buffer;
    buffer.resize(BUFFSIZE);
    auto const buff{&*buffer.begin()};

    for (;;)
    {
        DWORD readBytes;
        // The API suggests when the other end closes the pipe, you should get
        // 0. What appears to happen is that you get broken pipe.
        if (! ReadFile(m_handle, buff, BUFFSIZE, &readBytes, nullptr))
        {
            DWORD const err = GetLastError();
            if (err != ERROR_BROKEN_PIPE)
            {
                throw SystemError(err);
            }
        }

        if (readBytes == 0)
        {
            break;
        }

        result.append(buff, readBytes);
    }

    return result;
}

}    // namespace Linter