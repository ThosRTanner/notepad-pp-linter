#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Linter
{

class Settings
{
  public:
    Settings(std::wstring const &settings_xml);

    struct Linter
    {
        std::wstring extension_;
        std::wstring command_;
        bool use_stdin_ = false;
    };

    /** Returns the alpha mask for the 'squiggle' or -1 if not set */
    int alpha() const noexcept
    {
        return alpha_;
    }

    /** Returns the colour for the 'squiggle' or -1 if not set */
    int color() const noexcept
    {
        return colour_;
    }

    /** Return an iterator to the linters */
    std::vector<Linter>::const_iterator begin() const noexcept
    {
        return linters_.cbegin();
    }

    /** Returns true if there are no linters to run */
    bool empty() const noexcept
    {
        return linters_.empty();
    }

    /** Return an iterator to the linters */
    std::vector<Linter>::const_iterator end() const noexcept
    {
        return linters_.cend();
    }

    /** Reread settings if they've changed */
    void refresh();

  private:
    void read_settings();

    std::wstring settings_xml_;

    int alpha_ = -1;
    int colour_ = -1;
    std::filesystem::file_time_type last_update_time_;
    std::vector<Linter> linters_;
};

}    // namespace Linter
