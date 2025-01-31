#include "FilePipe.h"

#include "HandleWrapper.h"
#include "SystemError.h"

#include <errhandlingapi.h>
#include <handleapi.h>
#include <minwinbase.h>
#include <minwindef.h>
#include <namedpipeapi.h>
#include <winbase.h>
#include <winnt.h>

namespace Linter
{

FilePipe::Pipe Linter::FilePipe::create()
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
        throw SystemError(GetLastError());
    }

    return {HandleWrapper(parent), HandleWrapper(child)};
}

void FilePipe::detachFromParent(HandleWrapper const &handle)
{
    if (! SetHandleInformation(handle, HANDLE_FLAG_INHERIT, 0))
    {
        throw SystemError(GetLastError());
    }
}

}    // namespace Linter
