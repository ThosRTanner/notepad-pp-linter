#pragma once

#include <atlcomcli.h>
#include <msxml6.h>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Linter
{
class Dom_Node;
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

    /** Return the list of linters */
    std::vector<Linter> const &linters() const noexcept
    {
        return linters_;
    }

    /** Get a message colour */
    uint32_t get_message_colour(std::wstring const &colour) const noexcept;

    /** Reread settings if they've changed */
    void refresh();

  private:
    void read_settings();

    uint32_t read_colour_node(Dom_Node const &node);

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
    std::unordered_map<std::wstring, uint32_t> message_colours_;
};

}    // namespace Linter
