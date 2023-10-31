#pragma once
#include <string>

#if __cplusplus >= 202002L
#include <source_location>
#else
#include <cstdint>
#endif

namespace Linter
{
#if __cplusplus >= 202002L
    using SourceLocation = std::source_location;
#else
    struct SourceLocation
    {
        struct Location
        {
            constexpr std::uint_least32_t line() const noexcept
            {
                return 0;
            }

            const char *file_name() const noexcept
            {
                return "";
            }

            constexpr const char *function_name() const noexcept
            {
                return "";
            }
        };

        static Location current() noexcept
        {
            return {};
        }
    };
#endif

    using SourceLocationCurrent = decltype(SourceLocation::current());
}    // namespace Linter
