#include "System_Error.h"

#include "Plugin/Casts.h"

#include <comdef.h>
#include <comutil.h>
#include <errhandlingapi.h>
#include <intsafe.h>
#include <oaidl.h>
#include <oleauto.h>

#include <cstdio>    // For std::snprintf
#include <exception>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
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

System_Error::System_Error( // NOLINT(*-member-init)
    DWORD err, std::source_location const &location
) noexcept
{
    try
    {
        what_length_ = std::snprintf(
            &what_string_[0],
            sizeof(what_string_),
            "%08lx %s",
            err,
            std::generic_category()
                .message(windows_static_cast<int, DWORD>(err))
                .c_str()
        );
    }
    catch (std::exception const &e)
    {
#pragma warning(suppress : 26447)    // MS Bug with e.what() decl
        what_length_ = std::snprintf(
            &what_string_[0],
            sizeof(what_string_),
            "Error code %08lx then got %s",
            err,
            e.what()
        );
    }
    addLocationToMessage(location);
}

System_Error::System_Error( // NOLINT(*-member-init)
    DWORD err, std::string const &info, std::source_location const &location
) noexcept
{
    try
    {
        what_length_ = std::snprintf(
            &what_string_[0],
            sizeof(what_string_),
            "%s - %08lx %s",
            info.c_str(),
            err,
            std::generic_category()
                .message(windows_static_cast<int, DWORD>(err))
                .c_str()
        );
    }
    catch (std::exception const &e)
    {
#pragma warning(suppress : 26447)    // MS Bug with e.what() decl
        what_length_ = std::snprintf(
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

System_Error::System_Error( // NOLINT(*-member-init)
    HRESULT err, std::source_location const &location
) noexcept
{
    IErrorInfo *err_info{nullptr};
    std::ignore = GetErrorInfo(0, &err_info);
    _com_error const error{err, err_info};
    try
    {
        _bstr_t const msg{error.ErrorMessage()};
        what_length_ = std::snprintf(
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
        what_length_ = std::snprintf(
            &what_string_[0],
            sizeof(what_string_),
            "Got error %08lx but couldn't decode because %s",
            err,
            e.what()
        );
    }
    addLocationToMessage(location);
}

System_Error::System_Error( // NOLINT(*-member-init)
    HRESULT err, std::string const &info, std::source_location const &location
) noexcept
{
    IErrorInfo *err_info{nullptr};
    std::ignore = ::GetErrorInfo(0, &err_info);
    _com_error const error{err, err_info};
    try
    {
        _bstr_t const msg{error.ErrorMessage()};
        what_length_ = std::snprintf(
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
        what_length_ = std::snprintf(
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

void System_Error::addLocationToMessage(
    std::source_location const &location
) noexcept
{
    std::string_view filename{location.file_name()};
    if (auto const pos = filename.rfind('\\'); pos != std::string_view::npos)
    {
        filename = filename.substr(pos + 1);
    }
    // Windows somewhat annoyingly issues a warning about std::span being used
    // instead of gsl::span. We can ignore this here as we don't use the []
    // operator.
#pragma warning(push)
#pragma warning(disable : 26821)
    auto const what = std::span{what_string_};
    auto const free_data = what.subspan(what_length_);
#pragma warning(pop)
    auto *const start = &*free_data.begin();
    std::ignore = std::snprintf(
        start,
        &*free_data.rbegin() - start + 1,
        " at %s:%u %s",
        &*filename.begin(),
        location.line(),
        location.function_name()
    );
}

}    // namespace Linter
