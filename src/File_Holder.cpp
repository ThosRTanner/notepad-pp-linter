#include "File_Holder.h"

#include "Child_Pipe.h"
#include "Handle_Wrapper.h"
#include "Settings.h"
#include "System_Error.h"

#include <comutil.h>
#include <errhandlingapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <intsafe.h>
#include <minwindef.h>
#include <processenv.h>
#include <processthreadsapi.h>
#include <winbase.h>
#include <winnt.h>

#include <cwchar>    // for wcsdup
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <tuple>

namespace Linter
{

File_Holder::File_Holder(std::filesystem::path const &path) : path_(path)
{
}

File_Holder::~File_Holder()
{
    if (not temp_file_.empty())
    {
        std::error_code errcode;
        std::filesystem::remove(temp_file_, errcode);
    }
}

std::tuple<std::wstring, std::string, std::string> File_Holder::exec(
    Settings::Linter::Command const &command, std::string const &text
)
{
    auto const stdout_pipe = Child_Pipe::create_output_pipe();
    auto const stderr_pipe = Child_Pipe::create_output_pipe();
    auto const stdin_pipe = Child_Pipe::create_input_pipe();

    STARTUPINFO startup_info = {
        .cb = sizeof(STARTUPINFO),
        .dwFlags = STARTF_USESTDHANDLES,
        .hStdInput = stdin_pipe.reader(),
        .hStdOutput = stdout_pipe.writer(),
        .hStdError = stderr_pipe.writer()
    };

    PROCESS_INFORMATION proc_info = {0};

    setup_environment();

    auto program{expand_variables(command.program)};
    auto args{expand_arg_values(command.args)};
    // Microsoft is amazingly evil.
    // You need to supply the application name to avoid one sort of
    // vulnerability. However, if you have a .bat or .cmd file, you must either
    // not supply an application name or you need to supply the correct path to
    // the system cmd.exe, and surround the *entire* command with double quotes.
    args = L"\"" + program + L"\" " + args;
    if (command.program.extension() == L".bat"
        or command.program.extension() == L".cmd")
    {
        program = expand_variables(L"%SystemRoot%\\system32\\cmd.exe");
        // A note: The insertion of '"program" ' is optional, but the "/c " is
        // necesssary. I insert the first bit in case an error is produced, to
        // make it a bit clearer what is happening.
        args = L"\"" + program + L"\" /c \"" + args + L"\"";
    }
    // Oh yes, and this
    // See https://devblogs.microsoft.com/oldnewthing/20090601-00/?p=18083
    std::unique_ptr<wchar_t[]> const args_copy{wcsdup(args.c_str())};
    if (not CreateProcess(
            program.c_str(),
            args_copy.get(),
            nullptr,             // process security attributes
            nullptr,             // primary thread security attributes
            TRUE,                // handles are inherited
            CREATE_NO_WINDOW,    // creation flags
            nullptr,             // use parent's environment
            path_.parent_path().c_str(),    // use parent's current directory
            &startup_info,                  // STARTUPINFO pointer
            &proc_info
        ))
    {
        DWORD const error{GetLastError()};
        bstr_t const cmd{args.c_str()};
        throw System_Error(
            error, "Can't execute command: " + static_cast<std::string>(cmd)
        );
    }

    if (command.use_stdin)
    {
        // FIXME Use WaitForInputIdle here
        stdin_pipe.writer().writeFile(text);
    }
    stdin_pipe.writer().close();

    // FIXME use GetExitCodeProcess to get the exit code. However, that always
    // seems to return 0x103

    // We need to close all the handles for this end otherwise strange things
    // happen.
    CloseHandle(proc_info.hProcess);
    CloseHandle(proc_info.hThread);

    stdout_pipe.writer().close();
    stderr_pipe.writer().close();
    stdin_pipe.reader().close();

    auto const res = Child_Pipe::read_output_pipes(stdout_pipe, stderr_pipe);
    return std::make_tuple(args, res.first, res.second);
}

void File_Holder::write(std::string const &data)
{
    // We cannot put these files in temp dir because the tools search for
    // configuration in the directory the file is being processed. Therefore we
    // create the file in the same directory but mark it hidden.
    temp_file_ = [&]()
    {
        std::filesystem::path temp = path_;
        temp.concat(".linter.tmp").concat(path_.extension().c_str());
        return temp;
    }();

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

void File_Holder::setup_environment() const
{
    // Set up the supported environment variables.
    if (not SetEnvironmentVariable(L"LINTER_TARGET", temp_file_.c_str()))
    {
        throw System_Error();
    }
    if (not SetEnvironmentVariable(L"TARGET", path_.c_str()))
    {
        throw System_Error();
    }
    if (not SetEnvironmentVariable(L"TARGET_DIR", path_.parent_path().c_str()))
    {
        throw System_Error();
    }
    if (not SetEnvironmentVariable(L"TARGET_EXT", path_.extension().c_str()))
    {
        throw System_Error();
    }
    if (not SetEnvironmentVariable(
            L"TARGET_FILENAME", path_.filename().c_str()
        ))
    {
        throw System_Error();
    }
}

std::wstring File_Holder::expand_arg_values(std::wstring args) const
{
    // We treat a trailing %% as a reference to the linter temp file.
    if (args.ends_with(L"%%"))
    {
        args = args.substr(0, args.size() - 2) + L"\"%LINTER_TARGET%\"";
    }
    return expand_variables(args);
}

std::wstring File_Holder::expand_variables(std::wstring const &text) const
{
    // Replace all %x% stuff a-la windows command line.

    // First we get the length of the string we'll generate
    auto len = ExpandEnvironmentStrings(text.c_str(), nullptr, 0);
    if (len == 0)
    {
        throw System_Error();
    }
    std::wstring res;
    res.resize(len);

    // Then we get the actual string.
    len = ExpandEnvironmentStrings(text.c_str(), &*res.begin(), len);
    if (len == 0)
    {
        throw System_Error();
    }
    // This can't possibly overflow because len cannot be 0!
    res.resize(len - 1);    // Remove the trailing null
    return res;
}

}    // namespace Linter
