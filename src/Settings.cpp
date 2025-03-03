#include "Settings.h"

#include "Dom_Document.h"
#include "Dom_Node.h"
#include "Dom_Node_List.h"
#include "Linter.h"
#include "System_Error.h"

#include <atlcomcli.h>
#include <comutil.h>
#include <intsafe.h>
#include <msxml.h>
#include <msxml6.h>
#include <oleauto.h>

#include <chrono>
#include <filesystem>
#include <sstream>

namespace Linter
{

Settings::Settings(::Linter::Linter const &linter) :
    settings_xml_(
        linter.get_plugin_config_dir().append(linter.get_name() + L".xml")
    ),
    settings_xsd_(linter.get_module_path().replace_extension(".xsd"))
{
    // Create a schema cache and our xsd to it.
    auto hr = settings_schema_.CoCreateInstance(__uuidof(XMLSchemaCache60));
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't create XMLSchemaCache60");
    }

    CComVariant xsd{settings_xsd_.c_str()};
    hr = settings_schema_->add(bstr_t(""), xsd);
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't add to schema pool");
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
    auto const last_write_time{std::filesystem::last_write_time(settings_xml_)};
    if (last_write_time != last_update_time_)
    {
        read_settings();
        last_update_time_ = last_write_time;
    }
}

void Settings::read_settings()
{
    fill_alpha_ = -1;
    fg_colour_ = -1;
    linters_.clear();
    message_colours_.clear();
    message_colours_[L"default"] = RGB(0, 0, 0);        // Black
    message_colours_[L"error"] = RGB(255, 0, 0);        // Red
    message_colours_[L"warning"] = RGB(255, 127, 0);    // Orange

    Dom_Document settings{settings_xml_, settings_schema_};

    std::optional<Dom_Node> messages = settings.get_node("//messages");
    if (messages.has_value())
    {
        // Set up any custom message colours.
        Dom_Node_List types = messages->get_node_list("./*");
        for (auto const type : types)
        {
            message_colours_[type.get_name()] = read_colour_node(type);
        }
    }

    for (auto const linter : settings.get_node_list("//linter"))
    {
        std::vector<std::wstring> extensions;
        for (auto const extension_node : linter.get_node_list(".//extension"))
        {
            CComVariant extension = extension_node.get_typed_value();
            extensions.push_back(extension.bstrVal);
        }

        for (auto const command_node : linter.get_node_list(".//command"))
        {
            Dom_Node program_node{command_node.get_node(".//program")};
            CComVariant program{program_node.get_typed_value()};

            Dom_Node args_node{command_node.get_node(".//args")};
            CComVariant args{args_node.get_typed_value()};

            // FIXME Deal with STDIN somehow

            for (auto const &extension : extensions)
            {
                linters_.push_back({
                    .extension = extension,
                    .command =
                        {.program = program.bstrVal, .args = args.bstrVal}
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
        auto value = colour.get_typed_value();
        // This always comes back as a BSTR. I have no idea why.
        std::wstringstream data{
            std::wstring(value.bstrVal, SysStringLen(value.bstrVal))
        };
        uint32_t val;
        data >> val;
        bgr = (bgr >> 8) | (val << 16);
    }
    return bgr;
}

/*
    CComPtr<IXMLDOMNodeList> styleNode{settings.getNodeList("//style")};
    LONG nodes;
HRESULT hr = styleNode->get_length(&nodes);
if (! SUCCEEDED(hr))
{
    throw System_Error(hr, "Can't get XPath style length");
}

if (nodes != 0)
{
    CComPtr<IXMLDOMNode> node;
    hr = styleNode->nextNode(&node);
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't read style node");
    }
    CComQIPtr<IXMLDOMElement> element(node);
    CComVariant value;
    if (element->getAttribute(static_cast<bstr_t>(L"alpha"), &value)
        == S_OK)
    {
        int alphaVal{0};
        if (value.bstrVal)
        {
            std::wstringstream data{
                std::wstring(value.bstrVal, SysStringLen(value.bstrVal))
            };
            data >> alphaVal;
        }
        fill_alpha_ = alphaVal;
    }

    if (element->getAttribute(static_cast<bstr_t>(L"color"), &value)
        == S_OK)
    {
        unsigned int colorVal{0};
        if (value.bstrVal)
        {
            std::wstringstream data{
                std::wstring(value.bstrVal, SysStringLen(value.bstrVal))
            };
            data >> std::hex >> colorVal;
        }

        // reverse colors for scintilla's LE order
        fg_colour_ = 0;
        for (int i = 0; i < 3; i += 1)
        {
            fg_colour_ = (fg_colour_ << 8) | (colorVal & 0xff);
            colorVal >>= 8;
        }
    }
}
*/

}    // namespace Linter
