#include "Settings.h"

#include "Dom_Document.h"
#include "Dom_Node.h"
#include "Dom_Node_List.h"
#include "Indicator.h"
#include "Linter.h"
#include "System_Error.h"

#include "notepad++/PluginInterface.h"

#include <atlcomcli.h>
#include <comutil.h>
#include <intsafe.h>
#include <msxml.h>
#include <msxml6.h>
#include <oleauto.h>

#include <chrono>
#include <cwctype>
#include <filesystem>
#include <locale>
#include <sstream>

namespace Linter
{

namespace
{
std::unordered_map<std::wstring, uint32_t> default_message_colours = {
    {L"default", RGB(0,   255, 255)}, // Cyan
    {L"error",   RGB(255, 0,   0)  }, // Red
    {L"warning", RGB(255, 127, 0)  }  // Orange
};
}

Settings::Settings(::Linter::Linter const &linter) :
    settings_xml_(
        linter.get_plugin_config_dir().append(linter.get_name() + L".xml")
    ),
    settings_xsd_(linter.get_module_path().replace_extension(".xsd")),
    message_colours_(default_message_colours)
{
    // Create a schema cache and our xsd to it.
    auto hr = settings_schema_.CoCreateInstance(__uuidof(XMLSchemaCache60));
    if (not SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't create XMLSchemaCache60");
    }

    CComVariant xsd{settings_xsd_.c_str()};
    hr = settings_schema_->add(bstr_t(""), xsd);
    if (not SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't add to schema pool");
    }

    //A note: We try to read the settings at this point and quietly ignore errors
    //if we can't. This isn't great, but not sure where I can put errors.
    try
    {
        refresh();
    }
    catch (std::exception const&)
    {
        //Nothing we can usefully do.
    }
}

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
    auto res = menu_entries_.find(entry);
    if (res == menu_entries_.end())
    {
        return nullptr;
    }
    return &(res->second);
}

void Settings::read_settings()
{
    Dom_Document settings{settings_xml_, settings_schema_};
    read_indicator(settings);
    read_messages(settings);
    read_shortcuts(settings);
    read_linters(settings);
}

void Settings::read_indicator(Dom_Document const &settings)
{
    indicator_.read_config(settings.get_node("//indicator"));
}

void Settings::read_messages(Dom_Document const &settings)
{
    message_colours_ = default_message_colours;

    std::optional<Dom_Node> messages = settings.get_node("//messages");
    if (not messages.has_value())
    {
        return;
    }

    // Set up any custom message colours.
    Dom_Node_List types = messages->get_node_list("./*");
    for (auto const type : types)
    {
        message_colours_[type.get_name()] = read_colour_node(type);
    }
}

void Settings::read_shortcuts(Dom_Document const &settings)
{
    menu_entries_.clear();
    std::optional<Dom_Node> shortcuts = settings.get_node("//shortcuts");
    if (not shortcuts.has_value())
    {
        return;
    }

#define MAP_KEY(s) {L#s, VK_##s}
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
        MAP_KEY(F10),
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
    Dom_Node_List menu_entries = shortcuts->get_node_list("./*");
    for (auto const menu_entry : menu_entries)
    {
        ShortcutKey key = {
            ._isCtrl = menu_entry.get_optional_node("ctrl").has_value(),
            ._isAlt = menu_entry.get_optional_node("alt").has_value(),
            ._isShift = menu_entry.get_optional_node("shift").has_value()
        };
        // get the key as well.
        auto const k = menu_entry.get_value();
        if (k.size() == 1)
        {
#pragma warning(suppress : 26472)
            key._key = static_cast<UCHAR>(std::toupper(*k.begin()));
        }
        else
        {
            key._key = static_cast<UCHAR>(key_mappings.at(k));
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
            Dom_Node const program_node{command_node.get_node(".//program")};
            std::wstring const program{program_node.get_value()};

            Dom_Node const args_node{command_node.get_node(".//args")};
            std::wstring const args{args_node.get_value()};

            bool const use_stdin =
                args.find(L"%LINTER_TARGET%") == std::string::npos
                and not args.ends_with(L"%%");

            for (auto const &extension : extensions)
            {
                linters_.push_back({
                    .extension = extension,
                    .command =
                        {.program = program,
                                  .args = args,
                                  .use_stdin = use_stdin}
                });
            }
        }
    }
}

uint32_t Settings::read_colour_node(Dom_Node const &node)
{
    // get the r, g, b values and merge them into What Scintalla Expects
    Dom_Node_List colours = node.get_node_list("./*");
    uint32_t bgr = 0;
    for (auto const colour : colours)
    {
        // This always comes back as a BSTR. I have no idea why.
        auto value = colour.get_value();
        std::wstringstream data{colour.get_value()};
        uint32_t val;
        data >> val;
        bgr = (bgr >> 8) | (val << 16);
    }
    return bgr;
}

}    // namespace Linter
