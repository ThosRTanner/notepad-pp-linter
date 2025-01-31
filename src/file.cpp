#include "File.h"

#include "FilePipe.h"
#include "Handle_Wrapper.h"
#include "System_Error.h"

#include <comutil.h>
#include <errhandlingapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <intsafe.h>
#include <minwindef.h>
#include <processthreadsapi.h>
#include <winbase.h>
#include <winnt.h>

#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

namespace Linter
{

File::File(std::filesystem::path const &path) : path_(path)
{
}

File::~File()
{
    if (! temp_file_.empty())
    {
        std::error_code errcode;
        std::filesystem::remove(temp_file_, errcode);
    }
}

std::pair<std::string, std::string> File::exec(
    std::wstring command_line, std::string const *text
)
{
    if (! temp_file_.empty())
    {
        command_line += ' ';
        command_line += '"';
        command_line += temp_file_;
        command_line += '"';
    }

    auto const stdoutpipe = FilePipe::create();
    auto const stderrpipe = FilePipe::create();
    auto const stdinpipe = FilePipe::create();

    // Stop my handle being inherited by the child
    FilePipe::detachFromParent(stdoutpipe.reader);
    FilePipe::detachFromParent(stderrpipe.reader);
    FilePipe::detachFromParent(stdinpipe.writer);

    STARTUPINFO startInfo = {
        .cb = sizeof(STARTUPINFO),
        .dwFlags = STARTF_USESTDHANDLES,
        .hStdInput = stdinpipe.reader,
        .hStdOutput = stdoutpipe.writer,
        .hStdError = stderrpipe.writer
    };

    PROCESS_INFORMATION procInfo = {0};

    // See https://devblogs.microsoft.com/oldnewthing/20090601-00/?p=18083
    std::unique_ptr<wchar_t[]> const cmdline{wcsdup(command_line.c_str())};
    if (not CreateProcess(
            nullptr,
            cmdline.get(),       // command line
            nullptr,             // process security attributes
            nullptr,             // primary thread security attributes
            TRUE,                // handles are inherited
            CREATE_NO_WINDOW,    // creation flags
            nullptr,             // use parent's environment
            path_.parent_path().c_str(),    // use parent's current directory
            &startInfo,                     // STARTUPINFO pointer
            &procInfo
        ))
    {
        DWORD const error{GetLastError()};
        bstr_t const cmd{command_line.c_str()};
        throw System_Error(
            error, "Can't execute command: " + static_cast<std::string>(cmd)
        );
    }

    if (text != nullptr)
    {
        stdinpipe.writer.writeFile(*text);
    }

    // We need to close all the handles for this end otherwise strange things
    // happen.
    CloseHandle(procInfo.hProcess);
    CloseHandle(procInfo.hThread);

    stdoutpipe.writer.close();
    stderrpipe.writer.close();
    stdinpipe.writer.close();

    std::string out = stdoutpipe.reader.readFile();
    std::string err = stderrpipe.reader.readFile();
    return std::make_pair(out, err);
}

void File::write(std::string const &data)
{
    if (data.empty())
    {
        return;
    }

    temp_file_ = std::filesystem::temp_directory_path()
                     .append(path_.filename().string())
                     .concat(".linter");

    Handle_Wrapper handle{CreateFile(
        temp_file_.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY,
        nullptr
    )};

    handle.writeFile(data);
}

}    // namespace Linter
