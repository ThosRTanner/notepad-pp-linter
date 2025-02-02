#include "Child_Pipe.h"

#include "Handle_Wrapper.h"
#include "System_Error.h"

#include <handleapi.h>
#include <minwinbase.h>
#include <minwindef.h>
#include <namedpipeapi.h>
#include <winbase.h>
#include <winnt.h>

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
