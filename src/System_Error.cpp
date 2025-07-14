#include "System_Error.h"

#include <comdef.h>
#include <comutil.h>
#include <errhandlingapi.h>
#include <intsafe.h>
#include <oaidl.h>
#include <oleauto.h>

#include <cstdio>
#include <cstring>
#include <exception>
#include <source_location>
#include <string>
#include <system_error>
#include <utility>

namespace Linter
{

System_Error::System_Error(std::source_location const &location) noexcept :
    System_Error(GetLastError(), location)
{
}

System_Error::System_Error(
    std::string const &info, std::source_location const &location
) noexcept :
    System_Error(GetLastError(), info, location)
{
}

System_Error::System_Error(
    DWORD err, std::source_location const &location
) noexcept
{
    try
    {
        std::snprintf(
            &what_string_[0],
            sizeof(what_string_),
            "%08lx %s",
            err,
            std::generic_category().message(err).c_str()
        );
    }
    catch (std::exception const &e)
    {
#pragma warning(suppress : 26447)    // MS Bug with e.what() decl
        std::snprintf(
            &what_string_[0],
            sizeof(what_string_),
            "Error code %08lx then got %s",
            err,
            e.what()
        );
    }
    addLocationToMessage(location);
}

System_Error::System_Error(
    DWORD err, std::string const &info, std::source_location const &location
) noexcept
{
    try
    {
        std::snprintf(
            &what_string_[0],
            sizeof(what_string_),
            "%s - %08lx %s",
            info.c_str(),
            err,
            std::generic_category().message(err).c_str()
        );
    }
    catch (std::exception const &e)
    {
#pragma warning(suppress : 26447)    // MS Bug with e.what() decl
        std::snprintf(
            &what_string_[0],
            sizeof(what_string_),
            "%s - Error code %08lx then got %s",
            info.c_str(),
            err,
            e.what()
        );
    }
    addLocationToMessage(location);
}

System_Error::System_Error(
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
            &what_string_[0],
            sizeof(what_string_),
            "%08lx %s",
            err,
            static_cast<char *>(msg)
        );
    }
    catch (std::exception const &e)
    {
#pragma warning(suppress : 26447)    // MS Bug with e.what() decl
        std::snprintf(
            &what_string_[0],
            sizeof(what_string_),
            "Got error %08lx but couldn't decode because %s",
            err,
            e.what()
        );
    }
    addLocationToMessage(location);
}

System_Error::System_Error(
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
            &what_string_[0],
            sizeof(what_string_),
            "%s - %08lx %s",
            info.c_str(),
            err,
            static_cast<char *>(msg)
        );
    }
    catch (std::exception const &e)
    {
#pragma warning(suppress : 26447)    // MS Bug with e.what() decl
        std::snprintf(
            &what_string_[0],
            sizeof(what_string_),
            "%s - Got error %08lx but couldn't decode because %s",
            info.c_str(),
            err,
            e.what()
        );
    }
    addLocationToMessage(location);
}

System_Error::System_Error(System_Error const &) noexcept = default;

System_Error::System_Error(System_Error &&) noexcept = default;

System_Error &System_Error::operator=(System_Error const &) noexcept = default;

System_Error &System_Error::operator=(System_Error &&) noexcept = default;

System_Error::~System_Error() = default;

char const *System_Error::what() const noexcept
{
    return &what_string_[0];
}

void System_Error::addLocationToMessage(std::source_location const &location
) noexcept
{
    auto const full_path = location.file_name();
    char const *const file_name = std::strrchr(full_path, '\\');
    std::size_t const used{std::strlen(&what_string_[0])};
    std::snprintf(
        &what_string_[used],
        sizeof(what_string_) - used,
        " at %s:%u %s",
        (file_name == nullptr ? full_path : &file_name[1]),
        location.line(),
        location.function_name()
    );
    what_string_[sizeof(what_string_) - 1] = 0;
}

}    // namespace Linter
