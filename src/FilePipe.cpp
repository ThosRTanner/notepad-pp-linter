#include "FilePipe.h"

#include "Handle_Wrapper.h"
#include "System_Error.h"

#include <errhandlingapi.h>
#include <handleapi.h>
#include <minwinbase.h>
#include <minwindef.h>
#include <namedpipeapi.h>
#include <winbase.h>
#include <winnt.h>

namespace Linter
{

FilePipe::Pipe FilePipe::create()
{
    SECURITY_ATTRIBUTES security = {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = nullptr,
        .bInheritHandle = TRUE,
    };

    HANDLE parent;
    HANDLE child;
    if (! CreatePipe(&parent, &child, &security, 0))
    {
        throw System_Error(GetLastError());
    }

    return {Handle_Wrapper(parent), Handle_Wrapper(child)};
}

void FilePipe::detachFromParent(Handle_Wrapper const &handle)
{
    if (! SetHandleInformation(handle, HANDLE_FLAG_INHERIT, 0))
    {
        throw System_Error(GetLastError());
    }
}

}    // namespace Linter
