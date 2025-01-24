#pragma once
#include <exception>
#include <source_location>
#include <string>

namespace Linter
{
    class SystemError : public std::exception
    {

      public:
        /** Creates an exception object from the current system error */
        explicit SystemError(
            std::source_location const &location =
                std::source_location::current()
        ) noexcept;

        /** Creates an exception object from the current system error, appends string */
        explicit SystemError(
            std::string const &,
            std::source_location const &location =
                std::source_location::current()
        ) noexcept;

        /** Creates an exception object given a system error number */
        explicit SystemError(
            DWORD err,
            std::source_location const &location =
                std::source_location::current()
        ) noexcept;

        /** Creates an exception object from specified error with addition information string */
        SystemError(
            DWORD err, std::string const &,
            std::source_location const &location =
                std::source_location::current()
        ) noexcept;

        /** Creates an exception object given an HRESULT */
        explicit SystemError(
            HRESULT err,
            std::source_location const &location =
                std::source_location::current()
        ) noexcept;

        /** Creates an exception object from specified error with addition information string */
        SystemError(
            HRESULT err, std::string const &,
            std::source_location const &location =
                std::source_location::current()
        ) noexcept;

        SystemError(SystemError const &) noexcept;
        SystemError(SystemError &&) noexcept;
        SystemError &operator=(SystemError const &) noexcept;
        SystemError &operator=(SystemError &&) noexcept;

        ~SystemError();

        char const *what() const noexcept override;

      private:
        char m_buff[2048];

        void addLocationToMessage(std::source_location const &location
        ) noexcept;
    };
}    // namespace Linter
