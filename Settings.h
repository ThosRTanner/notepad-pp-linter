#pragma once

#if __cplusplus >= 201703L
#include <filesystem>
#endif
#include <string>
#include <vector>

namespace Linter
{
    class Settings
    {
      public:
        explicit Settings(wchar_t const *settings_xml);

        struct Linter
        {
            std::wstring m_extension;
            std::wstring m_command;
            bool m_useStdin = false;
        };

        /** Returns the alpha mask for the 'squiggle' or -1 if not set */
        int alpha() const noexcept
        {
            return m_alpha;
        }

        /** Returns the colour for the 'squiggle' or -1 if not set */
        int color() const noexcept
        {
            return m_color;
        }

        /** Return an iterator to the linters */
        std::vector<Linter>::const_iterator begin() const noexcept
        {
            return m_linters.cbegin();
        }

        /** Returns true if there are no linters to run */
        bool empty() const noexcept
        {
            return m_linters.empty();
        }

        /** Return an iterator to the linters */
        std::vector<Linter>::const_iterator end() const noexcept
        {
            return m_linters.cend();
        }

        /** Reread settings if they've changed */
        void refresh();

    private:
        void read_settings();

        std::wstring m_settings_xml;

        int m_alpha = -1;
        int m_color = -1;
#if __cplusplus >= 201703L
        std::filesystem::file_time_type m_last_update_time;
#else
        uint64_t m_last_update_time = 0;
#endif
        std::vector<Linter> m_linters;
    };

}    // namespace Linter
