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

        //begin function that returns const ref to first linter
        //end function that returns const ref to last linter

        /** Returns the alpha mask for the 'squiggle' or -1 if not set */
        int alpha() const
        {
            return alpha_;
        }

        /** Returns the colour for the 'squiggle' or -1 if not set */
        int color() const
        {
            return color_;
        }

        /** Return an iterator to the linters */
        std::vector<Linter>::const_iterator begin() const
        {
            return linters_.cbegin();
        }

        /** Returns true if there are no linters to run */
        bool empty() const
        {
            return linters_.empty();
        }

        /** Return an iterator to the linters */
        std::vector<Linter>::const_iterator end() const
        {
            return linters_.cend();
        }

        /** Reread settings if they've changed */
        void refresh();

    private:
        void read_settings();

        std::wstring settings_xml_;

        int alpha_;
        int color_;
#if __cplusplus >= 201703L
        std::filesystem::file_time_type last_update_time_;
#else
        uint64_t last_update_time_;
#endif
        std::vector<Linter> linters_;
    };

}    // namespace Linter
