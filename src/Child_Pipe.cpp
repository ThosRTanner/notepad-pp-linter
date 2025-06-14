#include "Child_Pipe.h"

#include "Handle_Wrapper.h"
#include "System_Error.h"

#include <errhandlingapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <intsafe.h>
#include <minwinbase.h>
#include <minwindef.h>
#include <namedpipeapi.h>
#include <synchapi.h>
#include <winbase.h>
#include <winerror.h>
#include <winnt.h>

#include <string>
#include <utility>
#include <vector>

#include <stddef.h>

namespace Linter
{

Child_Pipe Child_Pipe::create_input_pipe()
{
    Child_Pipe pipe;
    detach(pipe.pipes_.writer_);
    return pipe;
}

Child_Pipe Child_Pipe::create_output_pipe()
{
    Child_Pipe pipe;
    detach(pipe.pipes_.reader_);
    return pipe;
}

namespace
{

constexpr DWORD BUFFSIZE = 0x4000;

DWORD read_data(char *const buff, Handle_Wrapper const &pipe)
{
    DWORD bytes_avail = 0;
    if (not PeekNamedPipe(pipe, NULL, 0, NULL, &bytes_avail, NULL))
    {
        DWORD const err = GetLastError();
        if (err != ERROR_BROKEN_PIPE)
        {
            throw System_Error(err);
        }
    }
    if (bytes_avail != 0)
    {
        if (not ReadFile(
                pipe, buff, std::min(BUFFSIZE, bytes_avail), &bytes_avail, NULL
            ))
        {
            throw System_Error();
        }
    }
    return bytes_avail;
}

}    // namespace

std::pair<std::string, std::string> Child_Pipe::read_output_pipes(
    HANDLE process, Child_Pipe const &pipe1, Child_Pipe const &pipe2
)
{
    std::vector<char> buffer;
    buffer.resize(BUFFSIZE);
    auto const buff{&*buffer.begin()};

    std::string res1;
    std::string res2;

    // Have to close the writers or the outputting process can hang.
    pipe1.writer().close();
    pipe2.writer().close();

    // Unfortunately, WaitForMultiple object doesn't support anonymous pipes and
    // will behave as though they'd had an event every time you call the
    // function. So instead, we call WaitForSingleObject with a short timeout.
    // If we time out, the called process is still running, so we read any data
    // that's available on the two pipes. If we don't timeout, then the process
    // is no longer running, so we consume all the data left on the pipes and
    // then break out of the loop.
    bool more_to_read = true;
    while (more_to_read)
    {
        more_to_read = false;

        auto const state = WaitForSingleObject(process, 50);
        if (state == WAIT_FAILED)
        {
            throw System_Error();
        }

        more_to_read = state == WAIT_TIMEOUT;

        res1.append(buff, read_data(buff, pipe1.reader()));
        res2.append(buff, read_data(buff, pipe2.reader()));
    }
    return std::make_pair(res1, res2);
}

#pragma warning(suppress : 26455)
Child_Pipe::Child_Pipe() : pipes_(create_pipes())
{
}

Child_Pipe::Pipes Child_Pipe::create_pipes()
{
    SECURITY_ATTRIBUTES security = {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = TRUE,
    };

    HANDLE reader;
    HANDLE writer;
    if (not ::CreatePipe(&reader, &writer, &security, 0))
    {
        throw System_Error();
    }

    return {Handle_Wrapper(reader), Handle_Wrapper(writer)};
}

void Child_Pipe::detach(Handle_Wrapper const &handle)
{
    if (not ::SetHandleInformation(handle, HANDLE_FLAG_INHERIT, 0))
    {
        throw System_Error();
    }
}

}    // namespace Linter
