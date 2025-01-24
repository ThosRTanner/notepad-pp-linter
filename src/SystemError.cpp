#include "SystemError.h"

//#include "encoding.h"

#include <cstdio>
#include <cstring>
#include <system_error>
#include <tuple>

#include <comdef.h>

using namespace Linter;

SystemError::SystemError(std::source_location const &location) noexcept :
    SystemError(GetLastError(), location)
{
}

SystemError::SystemError(
    std::string const &info, std::source_location const &location
) noexcept :
    SystemError(GetLastError(), info, location)
{
}

SystemError::SystemError(
    DWORD err, std::source_location const &location
) noexcept
{
    try
    {
        std::snprintf(
            &m_buff[0],
            sizeof(m_buff),
            "%s",
            std::generic_category().message(err).c_str()
        );
    }
    catch (std::exception const &e)
    {
#pragma warning(suppress : 26447)    // MS Bug with e.what() decl
        std::snprintf(
            &m_buff[0],
            sizeof(m_buff),
            "Error code %08x then got %s",
            err,
            e.what()
        );
    }
    addLocationToMessage(location);
}

SystemError::SystemError(
    DWORD err, std::string const &info, std::source_location const &location
) noexcept
{
    try
    {
        std::snprintf(
            &m_buff[0],
            sizeof(m_buff),
            "%s - %s",
            info.c_str(),
            std::generic_category().message(err).c_str()
        );
    }
    catch (std::exception const &e)
    {
#pragma warning(suppress : 26447)    // MS Bug with e.what() decl
        std::snprintf(
            &m_buff[0],
            sizeof(m_buff),
            "%s - Error code %08x then got %s",
            info.c_str(),
            err,
            e.what()
        );
    }
    addLocationToMessage(location);
}

SystemError::SystemError(
    HRESULT err, std::source_location const &location
) noexcept
{
    IErrorInfo *err_info{nullptr};
    std::ignore = GetErrorInfo(0, &err_info);
    _com_error error{err, err_info};
    try
    {
        _bstr_t const msg{error.ErrorMessage()};
        std::snprintf(
            &m_buff[0], sizeof(m_buff), "%s", static_cast<char *>(msg)
        );
    }
    catch (std::exception const &e)
    {
#pragma warning(suppress : 26447)    // MS Bug with e.what() decl
        std::snprintf(
            &m_buff[0],
            sizeof(m_buff),
            "Got error %08x but couldn't decode because %s",
            err,
            e.what()
        );
    }
    addLocationToMessage(location);
}

SystemError::SystemError(
    HRESULT err, std::string const &info, std::source_location const &location
) noexcept
{
    IErrorInfo *err_info{nullptr};
    std::ignore = ::GetErrorInfo(0, &err_info);
    _com_error error{err, err_info};
    try
    {
        _bstr_t const msg{error.ErrorMessage()};
        std::snprintf(
            &m_buff[0],
            sizeof(m_buff),
            "%s - %s",
            info.c_str(),
            static_cast<char *>(msg)
        );
    }
    catch (std::exception const &e)
    {
#pragma warning(suppress : 26447)    // MS Bug with e.what() decl
        std::snprintf(
            &m_buff[0],
            sizeof(m_buff),
            "%s - Got error %08x but couldn't decode because %s",
            info.c_str(),
            err,
            e.what()
        );
    }
    addLocationToMessage(location);
}

Linter::SystemError::SystemError(SystemError const &) noexcept = default;

Linter::SystemError::SystemError(SystemError &&) noexcept = default;

SystemError &Linter::SystemError::operator=(SystemError const &) noexcept =
    default;

SystemError &Linter::SystemError::operator=(SystemError &&) noexcept = default;

SystemError::~SystemError() = default;

char const *Linter::SystemError::what() const noexcept
{
    return &m_buff[0];
}

void SystemError::addLocationToMessage(std::source_location const &location
) noexcept
{
    char const *const fullPath = location.file_name();
    char const *const fileName = std::strrchr(fullPath, '\\');
    std::size_t const used{std::strlen(&m_buff[0])};
    std::snprintf(
        &m_buff[used],
        sizeof(m_buff) - used,
        " at %s:%ud %s",
        (fileName == nullptr ? fullPath : &fileName[1]),
        location.line(),
        location.function_name()
    );
}