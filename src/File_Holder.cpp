#include "File_Holder.h"

#include "Child_Pipe.h"
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

File_Holder::File_Holder(std::filesystem::path const &path) : path_(path)
{
}

File_Holder::~File_Holder()
{
    if (! temp_file_.empty())
    {
        std::error_code errcode;
        std::filesystem::remove(temp_file_, errcode);
    }
}

std::pair<std::string, std::string> File_Holder::exec(
    Settings::Linter::Command const &command, std::string const *text
)
{
    std::wstring command_line = command.program + L" " + command.args;
    if (! temp_file_.empty())
    {
        command_line += ' ';
        command_line += '"';
        command_line += temp_file_;
        command_line += '"';
    }

    auto const stdoutpipe = Child_Pipe::create_output_pipe();
    auto const stderrpipe = Child_Pipe::create_output_pipe();
    auto const stdinpipe = Child_Pipe::create_input_pipe();

    STARTUPINFO startInfo = {
        .cb = sizeof(STARTUPINFO),
        .dwFlags = STARTF_USESTDHANDLES,
        .hStdInput = stdinpipe.reader(),
        .hStdOutput = stdoutpipe.writer(),
        .hStdError = stderrpipe.writer()
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

    if (command.use_stdin)
    {
        stdinpipe.writer().writeFile(*text);
    }
    stdinpipe.writer().close();

    // We need to close all the handles for this end otherwise strange things
    // happen.
    CloseHandle(procInfo.hProcess);
    CloseHandle(procInfo.hThread);

    stdoutpipe.writer().close();
    stderrpipe.writer().close();
    stdinpipe.reader().close();

    std::string out = stdoutpipe.reader().readFile();
    std::string err = stderrpipe.reader().readFile();
    return std::make_pair(out, err);
}

void File_Holder::write(std::string const &data)
{
    if (data.empty())
    {
        return;
    }

    // We cannot put these files in temp dir because the tools search for
    // configuration in the directory the file is being processed. Therefore we
    // create the file in the same directory but mark it hidden.

    {
        //Don't set temp_file_ until we have the complete path!
        std::filesystem::path temp = path_;
        temp.concat(".tmp.linter.tmp").concat(path_.extension().c_str());
        temp_file_ = temp;
    }

    Handle_Wrapper handle{CreateFile(
        temp_file_.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_HIDDEN,
        nullptr
    )};

    handle.writeFile(data);
}

}    // namespace Linter
