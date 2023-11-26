#include "stdafx.h"
#include "file.h"

#include "FilePipe.h"
#include "SystemError.h"

#include <memory>

using namespace Linter;

std::string File::exec(std::wstring commandLine, std::string const *text)
{
    if (!m_file.empty())
    {
        commandLine += ' ';
        commandLine += '"';
        commandLine += m_file;
        commandLine += '"';
    }

    const auto stdoutpipe = FilePipe::create();
    const auto stderrpipe = FilePipe::create();
    const auto stdinpipe = FilePipe::create();

    //Stop my handle being inherited by the child
    FilePipe::detachFromParent(stdoutpipe.m_reader);
    FilePipe::detachFromParent(stderrpipe.m_reader);
    FilePipe::detachFromParent(stdinpipe.m_writer);

    STARTUPINFO startInfo = {0};
    startInfo.cb = sizeof(STARTUPINFO);
    startInfo.hStdError = stderrpipe.m_writer;
    startInfo.hStdOutput = stdoutpipe.m_writer;
    startInfo.hStdInput = stdinpipe.m_reader;
    startInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION procInfo = {0};

    //See https://devblogs.microsoft.com/oldnewthing/20090601-00/?p=18083
    std::unique_ptr<wchar_t[]> const cmdline{wcsdup(commandLine.c_str())};
    BOOL const isSuccess = CreateProcess(nullptr,
        cmdline.get(),          // command line
        nullptr,                // process security attributes
        nullptr,                // primary thread security attributes
        TRUE,                   // handles are inherited
        CREATE_NO_WINDOW,       // creation flags
        nullptr,                // use parent's environment
        m_directory.c_str(),    // use parent's current directory
        &startInfo,             // STARTUPINFO pointer
        &procInfo);             // receives PROCESS_INFORMATION

    if (!isSuccess)
    {
        DWORD const error{GetLastError()};
        bstr_t const cmd{commandLine.c_str()};
        throw SystemError(error, "Can't execute command: " + static_cast<std::string>(cmd));
    }

    if (text != nullptr)
    {
        stdinpipe.m_writer.writeFile(*text);
    }

    //We need to close all the handles for this end otherwise strange things happen.
    CloseHandle(procInfo.hProcess);
    CloseHandle(procInfo.hThread);

    stdoutpipe.m_writer.close();
    stderrpipe.m_writer.close();
    stdinpipe.m_writer.close();

    return stdoutpipe.m_reader.readFile();
}

File::File(const std::wstring &fileName, const std::wstring &directory) : m_fileName(fileName), m_directory(directory)
{
}

File::~File()
{
    if (!m_file.empty())
    {
        _wunlink(m_file.c_str());
    }
}

void File::write(const std::string &data)
{
    if (data.empty())
    {
        return;
    }

    const std::wstring tempFileName = m_directory + L"/" + m_fileName + L".temp.linter.file.tmp";

    HandleWrapper fileHandle{CreateFile(
        tempFileName.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY, nullptr)};

    fileHandle.writeFile(data);

    m_file = tempFileName;
}
