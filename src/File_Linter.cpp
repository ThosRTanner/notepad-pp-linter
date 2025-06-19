#include "File_Linter.h"

#include "Child_Pipe.h"
#include "Handle_Wrapper.h"
#include "Settings.h"
#include "System_Error.h"
#include "encoding.h"

#include "Plugin/Plugin.h"

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
#include <utility>

namespace Linter
{

File_Linter::File_Linter(
    std::filesystem::path const &target,
    std::filesystem::path const &plugin_dir,
    std::filesystem::path const &settings_dir,
    std::vector<Settings::Variable> const &variables, std::string const &text
) :
    target_{target},
    plugin_dir_{plugin_dir},
    settings_dir_{settings_dir},
    temp_file_{get_temp_file_name()},
    variables_{variables},
    text_{text},
    created_temp_file_{false},
    orig_env_(GetEnvironmentStrings())
{
    if (orig_env_ == NULL)
    {
        // This is a guess. All the documentation says is that it returns null.
        // It says nothing about how you can determine what the cause was.
        throw System_Error();
    }
    setup_environment();
}

File_Linter::~File_Linter()
{
    std::error_code errcode;
    std::filesystem::remove(temp_file_, errcode);
    // Ignore errors...
    // Restore environment to original state
    SetEnvironmentStrings(const_cast<wchar_t *>(orig_env_));
    // And discard the saved buffer.
    FreeEnvironmentStrings(const_cast<wchar_t *>(orig_env_));
}

std::tuple<std::wstring, DWORD, std::string, std::string>
File_Linter::run_linter(Settings::Command const &command)
{
    if (not command.use_stdin)
    {
        ensure_temp_file_exists();
    }

    auto const [exit_code, out, err] =
        execute(command, command.use_stdin ? &text_ : nullptr);
    return std::make_tuple(command.args, exit_code, out, err);
}

std::filesystem::path File_Linter::get_temp_file_name() const
{
    // We cannot put these files in temp dir because the tools search for
    // configuration in the directory the file is being processed. Therefore we
    // create the file in the same directory but mark it hidden.
    std::filesystem::path temp = target_;
    temp.concat(".linter.tmp").concat(target_.extension().c_str());
    return temp;
}

void File_Linter::ensure_temp_file_exists()
{
    if (created_temp_file_)
    {
        return;
    }

    // Ideally we'd make this read-write and dup the handle whenever we needed
    // to pass as stdin, but it seems some things like to open their input
    // exclusively
    Handle_Wrapper handle{CreateFile(
        temp_file_.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY,
        nullptr
    )};

    handle.write_file(text_);

    created_temp_file_ = true;
}

void File_Linter::setup_environment()
{
    // Set up the supported environment variables.
    if (not SetEnvironmentVariable(L"LINTER_TARGET", temp_file_.c_str()))
    {
        throw System_Error();
    }

    if (not SetEnvironmentVariable(L"LINTER_PLUGIN_DIR", plugin_dir_.c_str()))
    {
        throw System_Error();
    }
    if (not SetEnvironmentVariable(L"LINTER_CONFIG_DIR", settings_dir_.c_str()))
    {
        throw System_Error();
    }

    if (not SetEnvironmentVariable(L"TARGET", target_.c_str()))
    {
        throw System_Error();
    }
    if (not SetEnvironmentVariable(
            L"TARGET_DIR", target_.parent_path().c_str()
        ))
    {
        throw System_Error();
    }
    if (not SetEnvironmentVariable(L"TARGET_EXT", target_.extension().c_str()))
    {
        throw System_Error();
    }
    if (not SetEnvironmentVariable(
            L"TARGET_FILENAME", target_.filename().c_str()
        ))
    {
        throw System_Error();
    }

    for (auto const &[name, command] : variables_)
    {
        // This consists of a variable name and a command to run.
        auto const [res, out, err] = execute(command);
        if (err.length() != 0)
        {
            // Add a warning
            warnings_.push_back(
                "Variable '" + Encoding::convert(name)
                + "' command produced stderr output " + err
            );
        }
        if (res != 0)
        {
            // Add a warning
            warnings_.push_back(
                "Variable '" + Encoding::convert(name)
                + "' command returned non-zero exit code: "
                + std::to_string(res)
            );
        }
        std::string output{out};
        if (not output.empty())
        {
            // Remove the trailing newline, if any.
            if (output.back() == '\n')
            {
                output.pop_back();
            }
        }
        // FIXME do we care about errors?
        if (not SetEnvironmentVariable(
                name.c_str(), Encoding::convert(output).c_str()
            ))
        {
            throw System_Error();
        }
    }
}

std::wstring File_Linter::expand_variables(std::wstring const &text)
{
    // Replace all %x% stuff a-la windows command line.

    // First we get the length of the string we'll generate
    auto const len = ExpandEnvironmentStrings(text.c_str(), nullptr, 0);
    if (len == 0)
    {
        throw System_Error();
    }
    std::wstring res;
    res.resize(len);

    if (ExpandEnvironmentStrings(text.c_str(), &*res.begin(), len) == 0)
    {
        throw System_Error();
    }

    // We do this because the compiler is too dim to work out that there can't
    // be an overflow because len can't possibly be zero, because of the check
    // just above.
    std::size_t const len2 = len;
    res.resize(len2 - 1);    // Remove the trailing null
    return res;
}

std::tuple<DWORD, std::string, std::string> File_Linter::execute(
    Settings::Command const &command, std::string const *const input
) const
{
    std::wstring program;
    auto args = expand_variables(command.args);
    if (not command.program.empty())
    {
        program = expand_variables(command.program);
        // Microsoft is amazingly evil.
        // You need to supply the application name to avoid one sort of
        // vulnerability. However, if you have a .bat or .cmd file, you must
        // either not supply an application name or you need to supply the
        // correct path to the system cmd.exe, and surround the *entire* command
        // with double quotes.
        args = L"\"" + program + L"\" " + args;
        if (auto const ext = command.program.extension().wstring();
            ext == L".bat" or ext == L".cmd")
        {
            program = expand_variables(L"%SystemRoot%\\system32\\cmd.exe");
            // A note: The insertion of '"program" ' is optional, but the "/c "
            // is necesssary. I insert the first bit in case an error is
            // produced, to make it a bit clearer what is happening.
            args = L"\"" + program + L"\" /c \"" + args + L"\"";
        }
    }

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

    // See https://devblogs.microsoft.com/oldnewthing/20090601-00/?p=18083
    std::unique_ptr<wchar_t[]> const args_copy{wcsdup(args.c_str())};
    if (not CreateProcess(
            program.empty() ? nullptr : program.c_str(),
            args_copy.get(),
            nullptr,             // process security attributes
            nullptr,             // primary thread security attributes
            TRUE,                // handles are inherited
            CREATE_NO_WINDOW,    // creation flags
            nullptr,             // use my environment
            target_.parent_path().wstring().c_str(),
            &startup_info,
            &proc_info
        ))
    {
        DWORD const error{GetLastError()};
        bstr_t const cmd{args.c_str()};
        throw System_Error(
            error, "Can't execute command: " + static_cast<std::string>(cmd)
        );
    }

    if (input != nullptr)
    {
        stdin_pipe.writer().write_file(*input);
    }
    stdin_pipe.writer().close();
    stdin_pipe.reader().close();

    auto const res = Child_Pipe::read_output_pipes(
        proc_info.hProcess, stdout_pipe, stderr_pipe
    );

    DWORD exit_code;
    if (not GetExitCodeProcess(proc_info.hProcess, &exit_code))
    {
        throw System_Error();
    }

    // And finally clean up.
    CloseHandle(proc_info.hProcess);
    CloseHandle(proc_info.hThread);

    return std::make_tuple(exit_code, res.first, res.second);
}

}    // namespace Linter
