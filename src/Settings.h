#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Linter
{

class Settings
{
  public:
    Settings(std::filesystem::path const &settings_xml);

    struct Linter
    {
        std::wstring extension_;
        std::wstring command_;
        std::wstring args_;
        bool use_stdin_ = false;
    };

    /** Returns the alpha mask for the 'squiggle' or -1 if not set */
    int fill_alpha() const noexcept
    {
        return fill_alpha_;
    }

    /** Returns the colour for the 'squiggle' or -1 if not set */
    int fg_colour() const noexcept
    {
        return fg_colour_;
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

    int fill_alpha_ = -1;
    int fg_colour_ = -1;
    std::filesystem::file_time_type last_update_time_;
    std::vector<Linter> linters_;
};

}    // namespace Linter
