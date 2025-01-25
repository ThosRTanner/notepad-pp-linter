#include "file.h"

#include "FilePipe.h"
#include "SystemError.h"

#include <comutil.h>
#include <errhandlingapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <io.h>
#include <processthreadsapi.h>

#include <cstring>
#include <memory>
#include <string>
#include <utility>

namespace Linter
{

std::pair<std::string, std::string> File::exec(
    std::wstring command_line, std::string const *text
)
{
    if (! file_.empty())
    {
        command_line += ' ';
        command_line += '"';
        command_line += file_;
        command_line += '"';
    }

    auto const stdoutpipe = FilePipe::create();
    auto const stderrpipe = FilePipe::create();
    auto const stdinpipe = FilePipe::create();

    // Stop my handle being inherited by the child
    FilePipe::detachFromParent(stdoutpipe.reader);
    FilePipe::detachFromParent(stderrpipe.reader);
    FilePipe::detachFromParent(stdinpipe.writer);

    STARTUPINFO startInfo = {0};
    startInfo.cb = sizeof(STARTUPINFO);
    startInfo.hStdError = stderrpipe.writer;
    startInfo.hStdOutput = stdoutpipe.writer;
    startInfo.hStdInput = stdinpipe.reader;
    startInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION procInfo = {0};

    // See https://devblogs.microsoft.com/oldnewthing/20090601-00/?p=18083
    std::unique_ptr<wchar_t[]> const cmdline{wcsdup(command_line.c_str())};
    if (not CreateProcess(
            nullptr,
            cmdline.get(),         // command line
            nullptr,               // process security attributes
            nullptr,               // primary thread security attributes
            TRUE,                  // handles are inherited
            CREATE_NO_WINDOW,      // creation flags
            nullptr,               // use parent's environment
            directory_.c_str(),    // use parent's current directory
            &startInfo,            // STARTUPINFO pointer
            &procInfo
        ))
    {
        DWORD const error{GetLastError()};
        bstr_t const cmd{command_line.c_str()};
        throw SystemError(
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

File::File(std::wstring const &filename, std::wstring const &directory) :
    filename_(filename),
    directory_(directory)
{
}

File::~File()
{
    if (! file_.empty())
    {
        _wunlink(file_.c_str());
    }
}

void File::write(std::string const &data)
{
    if (data.empty())
    {
        return;
    }

    std::wstring const tempfile =
        directory_ + L"/" + filename_ + L".temp.linter.file.tmp";

    HandleWrapper handle{CreateFile(
        tempfile.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY,
        nullptr
    )};

    handle.writeFile(data);

    file_ = tempfile;
}

}    // namespace Linter
