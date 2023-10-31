#include "StdAfx.h"

#include "SystemError.h"

#include "encoding.h"

#if __cplusplus >= 202002L
#include <cstring>
#endif

#include <cstdio>
#include <system_error>

#include <comdef.h>
#include <winbase.h>

using namespace Linter;

SystemError::SystemError(const SourceLocationCurrent &location) noexcept : SystemError(GetLastError(), location)
{
}

SystemError::SystemError(std::string const &info, const SourceLocationCurrent &location) noexcept
    : SystemError(GetLastError(), info, location)
{
}

SystemError::SystemError(DWORD err, const SourceLocationCurrent &location) noexcept
{
    try
    {
        std::snprintf(&m_buff[0], sizeof(m_buff), "%s", std::system_category().message(err).c_str());
    }
    catch (std::exception const &e)
    {
#pragma warning(push)
#pragma warning(disable : 26447)
        std::snprintf(&m_buff[0], sizeof(m_buff), "Error code %08x then got %s", err, e.what());
#pragma warning(pop)
    }
    addLocationToMessage(location);
}

SystemError::SystemError(DWORD err, std::string const &info, const SourceLocationCurrent &location) noexcept
{
    try
    {
        std::snprintf(&m_buff[0], sizeof(m_buff), "%s - %s", info.c_str(), std::system_category().message(err).c_str());
    }
    catch (std::exception const &e)
    {
#pragma warning(push)
#pragma warning(disable : 26447)
        std::snprintf(&m_buff[0], sizeof(m_buff), "%s - Error code %08x then got %s", info.c_str(), err, e.what());
#pragma warning(pop)
    }
    addLocationToMessage(location);
}

SystemError::SystemError(HRESULT err, const SourceLocationCurrent &location) noexcept
{
    IErrorInfo *err_info{nullptr};
    (void)GetErrorInfo(0, &err_info);
    _com_error error{err, err_info};
    try
    {
        _bstr_t const msg{error.ErrorMessage()};
        std::snprintf(&m_buff[0], sizeof(m_buff), "%s", static_cast<char *>(msg));
    }
    catch (std::exception const &e)
    {
#pragma warning(push)
#pragma warning(disable : 26447)
        std::snprintf(&m_buff[0], sizeof(m_buff), "Got error %08x but couldn't decode becuase %s", err, e.what());
#pragma warning(pop)
    }
    addLocationToMessage(location);
}

SystemError::SystemError(HRESULT err, std::string const &info, const SourceLocationCurrent &location) noexcept
{
    IErrorInfo *err_info{nullptr};
    (void)GetErrorInfo(0, &err_info);
    _com_error error{err, err_info};
    try
    {
        _bstr_t const msg{error.ErrorMessage()};
        std::snprintf(&m_buff[0], sizeof(m_buff), "%s - %s", info.c_str(), static_cast<char *>(msg));
    }
    catch (std::exception const &e)
    {
#pragma warning(push)
#pragma warning(disable : 26447)
        std::snprintf(&m_buff[0], sizeof(m_buff), "%s - Got error %08x but couldn't decode becuase %s", info.c_str(), err, e.what());
#pragma warning(pop)
    }
    addLocationToMessage(location);
}

Linter::SystemError::SystemError(SystemError &&) noexcept = default;

SystemError::~SystemError() = default;

char const *Linter::SystemError::what() const noexcept
{
    return &m_buff[0];
}

void SystemError::addLocationToMessage(const SourceLocationCurrent &location) noexcept
{
#if __cplusplus >= 202002L
    const char *const fullPath = location.file_name();
    const char *const fileName = std::strrchr(fullPath, '\\');
    const std::size_t used{std::strlen(&m_buff[0])};
    std::snprintf(&m_buff[used],
        sizeof(m_buff) - used,
        " at %s:%ud %s",
        (fileName == nullptr ? fullPath : &fileName[1]),
        location.line(),
        location.function_name());
#else
    (void)location;
#endif
}
