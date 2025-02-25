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
    settings_xml_(linter.get_plugin_config_dir().append(linter.get_name() + L".xml")),
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

    Dom_Document settings{settings_xml_, settings_schema_};

    // FIXME getNodeList should return a Dom_Node_List
    Dom_Node_List linters{settings.getNodeList("//linter")};

    for (auto linter : linters)
    {
        Dom_Node_List extension_nodes{linter.get_node_list(".//extension")};
        std::vector<std::wstring> extensions;
        for (auto const extension_node : extension_nodes)
        {
            CComVariant extension = extension_node.get_typed_value();
            extensions.push_back(extension.bstrVal);
        }

        // FIXME xsd validation shouldn't allow empty command or extension list

        Dom_Node_List command_nodes{linter.get_node_list(".//command")};
        for (auto const command_node : command_nodes)
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
