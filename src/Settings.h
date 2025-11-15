#pragma once

#include "Indicator.h"
#include "Menu_Entry.h" // IWYU pragma: keep
// This would be better than the above. Slightly
// IWYU pragma: no_forward_declare Menu_Entry

#include "notepad++/PluginInterface.h"

#include <atlcomcli.h>
#include <msxml6.h>

#include <cstdint>    // for uint32_t
#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>    // for pair
#include <vector>

namespace Linter
{

class Dom_Document;
class Dom_Node;
class Linter;

class Settings
{
  public:
    explicit Settings(Linter const &linter);

    Settings(Settings const &) = delete;
    Settings &operator=(Settings const &) = delete;
    Settings(Settings &&) = delete;
    Settings &operator=(Settings &&) = delete;

    ~Settings();

    struct Command
    {
        std::filesystem::path program;
        std::wstring args;
        bool use_stdin = false;
    };

    struct Linter
    {
        std::wstring extension;
        Command command;
    };

    using Variable = std::pair<std::wstring, Command>;

    /** Returns the configuration path */
    std::filesystem::path const &settings_file() const noexcept
    {
        return settings_xml_;
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

    ShortcutKey const *get_shortcut_key(Menu_Entry) const;

    Indicator const &indicator() const noexcept
    {
        return indicator_;
    }

    std::vector<Variable> const &get_variables() const noexcept
    {
        return variables_;
    }

    bool enabled() const noexcept
    {
        return enabled_;
    }

    static uint32_t read_colour_node(Dom_Node const &node);

  private:
    void read_settings();

    /** Process <indicator> XML element */
    void read_indicator(Dom_Document const &settings);

    /** Process <messages> XML element */
    void read_messages(Dom_Document const &settings);

    /** Process <shortcuts> XML element */
    void read_shortcuts(Dom_Document const &settings);

    /** Process <linters> XML element */
    void read_linters(Dom_Document const &settings);

    /** Process <variables> XML element */
    void read_variables(Dom_Document const &settings);

    /** Process <misc> XML element */
    void read_misc(Dom_Document const &settings);

    /** Process <command> XML element */
    static Command read_command(Dom_Node const &command_node);

    // configuration file
    std::filesystem::path const settings_xml_;

    // xsd file
    std::filesystem::path const settings_xsd_;

    // Processed schema
    CComPtr<IXMLDOMSchemaCollection2> settings_schema_;

    // Last time linter++.xml was updated.
    std::filesystem::file_time_type last_update_time_;

    // The list of linters
    std::vector<Linter> linters_;

    // Custom message colours
    std::unordered_map<std::wstring, uint32_t> message_colours_;

    // Shortcut keys for the menu.
    std::unordered_map<Menu_Entry, ShortcutKey> menu_entries_;

    // Error indicator
    Indicator indicator_;

    // List of variables
    std::vector<Variable> variables_;

    // Startup enabled or not
    bool enabled_{true};
};

}    // namespace Linter
