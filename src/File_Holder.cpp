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
#include <processenv.h>
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
    Settings::Linter::Command const &command, std::string const &text
)
{
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

    setup_environment();

    auto program{expand_variables(command.program)};
    auto args{expand_arg_values(command.args)};
    // Microsoft is amazingly evil.
    // You need to supply the application name to avoid one sort of
    // vulnerability. However, if you have a .bat or .cmd file, you must
    // either not supply an application name or you need to supply the correct
    // path to the system cmd.exe, and surround the *entire* command
    // with double quotes.
    args = L"\"" + program + L"\" " + args;
    if (command.program.extension() == L".bat"
        or command.program.extension() == L".cmd")
    {
        program = expand_variables(L"%SystemRoot%\\system32\\cmd.exe");
        // A note: The insertion of '"program" ' is optional, but the "/c "
        // is necesssary. I insert the first bit in case an error is produced,
        // to make it a bit clearer what is happening.
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
            &startInfo,                     // STARTUPINFO pointer
            &procInfo
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
        stdinpipe.writer().writeFile(text);
    }
    stdinpipe.writer().close();

    // Should likely be using WaitForSingleObject(procInfo.hProcess, INFINITE)
    // somewhere here. It does seem that for the first run of this, we don't
    // actually get a result till we change the buffer. However, this seems to
    // hang indefinitely

    // We need to close all the handles for this end otherwise
    // strange things happen.
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
    // FIXME Why do we do this?
    if (data.empty())
    {
        return;
    }

    // We cannot put these files in temp dir because the tools search for
    // configuration in the directory the file is being processed. Therefore we
    // create the file in the same directory but mark it hidden.

    {
        // Don't set temp_file_ until we have the complete path!
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
    // Additionally, we treat a trailing %% as a reference to the target.
    if (args.ends_with(L"%%"))
    {
        args = args.substr(0, args.size() - 2) + L"\"%LINTER_TARGET%\"";
    }
    return expand_variables(args);
}

std::wstring File_Holder::expand_variables(std::wstring const &command_line
) const
{
    // Replace all %x% stuff a-la windows command line.
    // Get the length of the string we'll generate
    auto len = ExpandEnvironmentStrings(command_line.c_str(), nullptr, 0);
    if (len == 0)
    {
        throw System_Error();
    }
    std::wstring res;
    res.resize(len);
    len = ExpandEnvironmentStrings(command_line.c_str(), &*res.begin(), len);
    if (len == 0)
    {
        throw System_Error();
    }
    res.resize(len - 1);    // Remove the trailing null
    return res;
}

}    // namespace Linter
