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
#include <winbase.h>
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

std::pair<std::string, std::string> Child_Pipe::read_output_pipes(
    Child_Pipe const &pipe1, Child_Pipe const &pipe2
)
{
    std::string res1;
    std::string res2;

    constexpr DWORD BUFFSIZE = 0x4000;
    std::vector<char> buffer;
    buffer.resize(BUFFSIZE);
    auto const buff{&*buffer.begin()};

    bool more_to_read = true;
    while (more_to_read)
    {
        more_to_read = false;

        DWORD bytes_avail = 0;
        if (not PeekNamedPipe(
                pipe1.reader(), NULL, 0, NULL, &bytes_avail, NULL
            ))
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
                    pipe1.reader(), buff, BUFFSIZE, &bytes_avail, NULL
                ))
            {
                throw System_Error();
            }
            res1.append(buff, bytes_avail);
            more_to_read = true;
        }
        bytes_avail = 0;
        if (not PeekNamedPipe(
                pipe2.reader(), NULL, 0, NULL, &bytes_avail, NULL
            ))
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
                    pipe2.reader(), buff, BUFFSIZE, &bytes_avail, NULL
                ))
            {
                throw System_Error();
            }
            res2.append(buff, bytes_avail);
            more_to_read = true;
        }
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
