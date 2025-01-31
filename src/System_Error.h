#pragma once

#include <intsafe.h>

#include <exception>
#include <source_location>
#include <string>

namespace Linter
{

class System_Error : public std::exception
{
  public:
    /** Creates an exception object from the current system error */
    explicit System_Error(
        std::source_location const &location = std::source_location::current()
    ) noexcept;

    /** Creates an exception object from the current system error, appends
     * string */
    explicit System_Error(
        std::string const &,
        std::source_location const &location = std::source_location::current()
    ) noexcept;

    /** Creates an exception object given a system error number */
    explicit System_Error(
        DWORD err,
        std::source_location const &location = std::source_location::current()
    ) noexcept;

    /** Creates an exception object from specified error with addition
     * information string */
    System_Error(
        DWORD err, std::string const &,
        std::source_location const &location = std::source_location::current()
    ) noexcept;

    /** Creates an exception object given an HRESULT */
    explicit System_Error(
        HRESULT err,
        std::source_location const &location = std::source_location::current()
    ) noexcept;

    /** Creates an exception object from specified error with addition
     * information string */
    System_Error(
        HRESULT err, std::string const &,
        std::source_location const &location = std::source_location::current()
    ) noexcept;

    System_Error(System_Error const &) noexcept;
    System_Error(System_Error &&) noexcept;
    System_Error &operator=(System_Error const &) noexcept;
    System_Error &operator=(System_Error &&) noexcept;

    ~System_Error();

    char const *what() const noexcept override;

  private:
    char what_string_[2048];

    void addLocationToMessage(std::source_location const &location) noexcept;
};

}    // namespace Linter
