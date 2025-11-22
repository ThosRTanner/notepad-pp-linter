#include "Settings.h"

#include "Dom_Document.h"
#include "Dom_Node.h"
#include "Dom_Node_List.h"
#include "Indicator.h"
#include "Linter.h"
#include "Menu_Entry.h"
#include "System_Error.h"

#include "notepad++/PluginInterface.h"

#include <atlcomcli.h>
#include <comutil.h>
#include <intsafe.h>
#include <msxml6.h>
#include <wingdi.h>     // For RGB
#include <winuser.h>    // For VK_...

#include <chrono>
#include <cwctype>
#include <exception>
#include <filesystem>
#include <list>
#include <optional>
#include <regex>
#include <sstream>
#include <vector>

namespace Linter
{

namespace
{

auto default_message_colours()
{
    static std::unordered_map<std::wstring, uint32_t> const colours{
        {L"default", RGB(0,   255, 255)}, // Cyan
        {L"error",   RGB(255, 0,   0)  }, // Red
        {L"warning", RGB(255, 127, 0)  }  // Orange
    };
    return colours;
}

}    // namespace

Settings::Settings(::Linter::Linter const &linter) :
    settings_xml_(
        linter.get_plugin_config_dir().append(linter.get_name() + L".xml")
    ),
    settings_xsd_(linter.get_module_path().replace_extension(".xsd")),
    message_colours_(default_message_colours())
{
    // Create a schema cache and our xsd to it.
    auto hres = settings_schema_.CoCreateInstance(__uuidof(XMLSchemaCache60));
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres, "Can't create XMLSchemaCache60");
    }

    CComVariant const xsd{settings_xsd_.c_str()};
    hres = settings_schema_->add(bstr_t(""), xsd);
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres, "Can't add to schema pool");
    }

    // A note: We try to read the settings at this point and quietly ignore
    // errors if we can't. This isn't great, but not sure where I can put
    // errors.
    try
    {
        refresh();
    }
    catch (std::exception const &err)
    {
        // Nothing we can usefully do.
        std::ignore = err;
    }
}

Settings::~Settings() = default;

uint32_t Settings::get_message_colour(std::wstring const &colour) const noexcept
{
    // Note: This won't throw despite the compiler warnings, because the
    // comparison function is nothrow
#pragma warning(suppress : 26447)
    auto val = message_colours_.find(colour);
    if (val == message_colours_.end())
    {
#pragma warning(suppress : 26447)
        val = message_colours_.find(L"default");
    }
    return val->second;
}

void Settings::refresh()
{
    if (not std::filesystem::exists(settings_xml_))
    {
        return;
    }
    auto const last_write_time{std::filesystem::last_write_time(settings_xml_)};
    if (last_write_time != last_update_time_)
    {
        read_settings();
        last_update_time_ = last_write_time;
    }
}

ShortcutKey const *Settings::get_shortcut_key(Menu_Entry entry) const
{
    auto const res = menu_entries_.find(entry);
    if (res == menu_entries_.end())
    {
        return nullptr;
    }
    return &(res->second);
}

uint32_t Settings::read_colour_node(Dom_Node const &node)
{
    // get the r, g, b values and merge them into What Scintalla Expects
    Dom_Node_List const colours = node.get_node_list("./*");
    uint32_t bgr = 0;
    for (auto const colour : colours)
    {
        // This always comes back as a BSTR. I have no idea why.
        std::wstringstream data{colour.get_value()};
        uint32_t val;    // NOLINT(cppcoreguidelines-init-variables)
        data >> val;
        bgr = (bgr >> 8) | (val << 16);
    }
    return bgr;
}

void Settings::read_settings()
{
    Dom_Document const settings{settings_xml_, settings_schema_};
    read_indicator(settings);
    read_messages(settings);
    read_shortcuts(settings);
    read_linters(settings);
    read_variables(settings);
    read_misc(settings);
}

void Settings::read_indicator(Dom_Document const &settings)
{
    indicator_.read_config(settings.get_node("//indicator"));
}

void Settings::read_messages(Dom_Document const &settings)
{
    message_colours_ = default_message_colours();

    auto const messages = settings.get_node("//messages");
    if (not messages.has_value())
    {
        return;
    }

    // Set up any custom message colours.
    for (auto const type : messages->get_node_list("./*"))
    {
        message_colours_[type.get_name()] = read_colour_node(type);
    }
}

void Settings::read_shortcuts(Dom_Document const &settings)
{
    menu_entries_.clear();
    auto const shortcuts = settings.get_node("//shortcuts");
    if (not shortcuts.has_value())
    {
        return;
    }

#define MAP_KEY(s)  \
    {               \
        L#s, VK_##s \
    }
#undef DELETE
    static std::unordered_map<std::wstring, int> const key_mappings{
        MAP_KEY(F1),
        MAP_KEY(F2),
        MAP_KEY(F3),
        MAP_KEY(F4),
        MAP_KEY(F5),
        MAP_KEY(F6),
        MAP_KEY(F7),
        MAP_KEY(F8),
        MAP_KEY(F9),
        MAP_KEY(F10),
        MAP_KEY(F11),
        MAP_KEY(F12),
        MAP_KEY(NUMPAD0),
        MAP_KEY(NUMPAD1),
        MAP_KEY(NUMPAD2),
        MAP_KEY(NUMPAD3),
        MAP_KEY(NUMPAD4),
        MAP_KEY(NUMPAD5),
        MAP_KEY(NUMPAD6),
        MAP_KEY(NUMPAD7),
        MAP_KEY(NUMPAD8),
        MAP_KEY(NUMPAD9),
        MAP_KEY(INSERT),
        MAP_KEY(DELETE),
        MAP_KEY(HOME),
        MAP_KEY(END),
        {L"PAGE UP",   VK_PRIOR},
        {L"PAGE DOWN", VK_NEXT }
    };
#undef MAP_KEY

    // Set up any shortcuts
    for (auto const menu_entry : shortcuts->get_node_list("./*"))
    {
        ShortcutKey key = {
            ._isCtrl = menu_entry.get_optional_node("ctrl").has_value(),
            ._isAlt = menu_entry.get_optional_node("alt").has_value(),
            ._isShift = menu_entry.get_optional_node("shift").has_value()
        };
        // get the key as well.
        auto const key_name = menu_entry.get_value();
        if (key_name.size() == 1)
        {
#pragma warning(suppress : 26472)
            key._key = static_cast<UCHAR>(std::towupper(*key_name.begin()));
        }
        else
        {
            key._key = static_cast<UCHAR>(key_mappings.at(key_name));
        }
        menu_entries_[get_menu_entry_from_element(menu_entry.get_name())] = key;
    }
}

void Settings::read_linters(Dom_Document const &settings)
{
    linters_.clear();
    for (auto const linter : settings.get_node_list("//linter"))
    {
        std::vector<std::wstring> extensions;
        for (auto const extension_node : linter.get_node_list(".//extension"))
        {
            extensions.push_back(extension_node.get_value());
        }

        for (auto const command_node : linter.get_node_list(".//command"))
        {
            Settings::Command cmd = read_command(command_node);
            cmd.use_stdin =
                cmd.args.find(L"%LINTER_TARGET%") == std::string::npos;

            for (auto const &extension : extensions)
            {
                linters_.push_back({.extension = extension, .command = cmd});
            }
        }
    }
}

void Settings::read_variables(Dom_Document const &settings)
{
    variables_.clear();

    for (auto const variable : settings.get_node_list("//variable"))
    {
        Dom_Node const name_node{variable.get_node(".//name")};
        std::wstring const name{name_node.get_value()};
        Dom_Node const command_node{variable.get_node(".//command")};
        Command const cmd = read_command(command_node);
        variables_.emplace_back(name, cmd);
    }
}

void Settings::read_misc(Dom_Document const &settings)
{
    // Read the enabled state
    enabled_ = true;
    if (settings.get_node("//disabled").has_value())
    {
        enabled_ = false;
    }
}

Settings::Command Settings::read_command(Dom_Node const &command_node)
{
    // Expects either 'cmdline' or 'program' and 'args'
    // A note: That isn't currently supported in the .xml, because
    // windows is arcane enough without the strange quoting rules of the
    // command line.
    std::wstring program;
    std::wstring args;
    if (auto const command = command_node.get_optional_node(".//cmdline"))
    {
        args = command->get_value();
    }
    else
    {
        Dom_Node const program_node{command_node.get_node(".//program")};
        program = program_node.get_value();

        Dom_Node const args_node{command_node.get_node(".//args")};
        args = args_node.get_value();
    }
    // It appears microsoft completely ignores the xs:token type, so we
    // have to do this ourselves.
    args = std::regex_replace(args, std::wregex(LR"((^\s+)|(\s+$))"), L"");
    args = std::regex_replace(args, std::wregex(LR"(\s+)"), L" ");
    // Should really only do this for linter commands but...
    if (args.ends_with(L"%%"))
    {
        args = args.substr(0, args.size() - 2) + L"\"%LINTER_TARGET%\"";
    }
    return Command({.program = program, .args = args});
}

}    // namespace Linter
