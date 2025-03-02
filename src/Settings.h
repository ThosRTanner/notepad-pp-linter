#pragma once

#include <MsXml6.h>
#include <atlcomcli.h>

#include <filesystem>
#include <string>
#include <vector>

namespace Linter
{

class Linter;

class Settings
{
  public:
    explicit Settings(Linter const &linter);

    struct Linter
    {
        std::wstring extension;
        struct Command
        {
            std::filesystem::path program;
            std::wstring args;
            // Remove use_stdin_. supply %LINTER_TARGET% if args (or not)
            bool use_stdin = false;
        } command;
    };

    /** Returns the configuration path */
    std::filesystem::path const &settings_file() const noexcept
    {
        return settings_xml_;
    }

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

    // configuration file
    std::filesystem::path const settings_xml_;

    // xsd file
    std::filesystem::path const settings_xsd_;

    // Processed schema
    CComPtr<IXMLDOMSchemaCollection2> settings_schema_;

    int fill_alpha_ = -1;
    int fg_colour_ = -1;
    std::filesystem::file_time_type last_update_time_;
    std::vector<Linter> linters_;
};

}    // namespace Linter
