#include "Settings.h"

#include "Dom_Document.h"
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

namespace
{
std::filesystem::path get_module_path(HMODULE module)
{
    wchar_t module_path[_MAX_PATH + 1];
    // FIXME we should really check for an error here.
    // FIXME this should be a method in the plugin class
    GetModuleFileName(
        module, &module_path[0], sizeof(module_path) / sizeof(module_path[0])
    );
    std::filesystem::path xsd_file(module_path);
    xsd_file.replace_extension(".xsd");
    return xsd_file;
}

}    // namespace

Settings::Settings(::Linter::Linter const &linter) :
    settings_xml_(linter.get_plugin_config_dir().append(L"linter++.xml")),
    settings_xsd_(get_module_path(linter.module()).replace_extension(".xsd"))
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

class Dom_Node_List
{
  public:
    class iterator
    {
      public:
        iterator(CComPtr<IXMLDOMNodeList> node_list, LONG item) noexcept :
            node_list_(node_list),
            item_(item)
        {
        }

        iterator operator++() noexcept
        {
            item_ += 1;
            return *this;
        }

        bool operator!=(iterator const &other) const noexcept
        {
            return node_list_ != other.node_list_ || item_ != other.item_;
        }

        CComPtr<IXMLDOMNode> operator*() const
        {
            CComPtr<IXMLDOMNode> node;
            auto const hr = node_list_->get_item(item_, &node);
            if (! SUCCEEDED(hr))
            {
                throw System_Error(hr, "Can't get linter detail");
            }
            return node;
        }

      private:
        CComPtr<IXMLDOMNodeList> node_list_;
        LONG item_;
    };

    Dom_Node_List(CComPtr<IXMLDOMNodeList> node_list) : node_list_(node_list)
    {
        HRESULT const hr = node_list->get_length(&num_items_);
        if (! SUCCEEDED(hr))
        {
            throw System_Error(hr, "Can't get number of items in node list");
        }
    }

    iterator begin() const noexcept
    {
        return iterator(node_list_, 0);
    }

    iterator end() const noexcept
    {
        return iterator(node_list_, num_items_);
    }

  private:
    CComPtr<IXMLDOMNodeList> node_list_;
    LONG num_items_;
};

void Settings::read_settings()
{
    fill_alpha_ = -1;
    fg_colour_ = -1;
    linters_.clear();

    Dom_Document settings{settings_xml_, settings_schema_};

    CComPtr<IXMLDOMNodeList> linters{settings.getNodeList("//linter")};

    for (auto linter : Dom_Node_List(linters))
    {
        CComPtr<IXMLDOMNodeList> extension_nodes;
        auto hr = linter->selectNodes(
            static_cast<bstr_t>(L".//extension"), &extension_nodes
        );
        if (! SUCCEEDED(hr))
        {
            throw System_Error(hr, "Can't get extensions");
        }
        std::vector<std::wstring> extensions;
        for (auto extension_node : Dom_Node_List(extension_nodes))
        {
            CComVariant extension;
            hr = extension_node->get_nodeTypedValue(&extension);
            if (! SUCCEEDED(hr))
            {
                throw System_Error(hr, "Can't get extension value");
            }
            extensions.push_back(extension.bstrVal);
        }

        // FIXME xsd validation shouldn't allow empty command or extension list

        struct Command
        {
            std::wstring program;
            std::wstring args;
        };
        std::vector<Command> commands;
        CComPtr<IXMLDOMNodeList> command_nodes;
        hr = linter->selectNodes(
            static_cast<bstr_t>(L".//command"), &command_nodes
        );
        if (! SUCCEEDED(hr))
        {
            throw System_Error(hr, "Can't get extensions");
        }
        for (auto command_node : Dom_Node_List(command_nodes))
        {
            CComPtr<IXMLDOMNode> program_node;
            hr = command_node->selectSingleNode(
                static_cast<bstr_t>(L".//program"), &program_node
            );
            if (! SUCCEEDED(hr))
            {
                throw System_Error(hr, "Can't get program");
            }

            CComVariant command;
            hr = program_node->get_nodeTypedValue(&command);
            if (! SUCCEEDED(hr))
            {
                throw System_Error(hr, "Can't get command value");
            }

            CComPtr<IXMLDOMNode> args_node;
            hr = command_node->selectSingleNode(
                static_cast<bstr_t>(L".//args"), &args_node
            );
            if (! SUCCEEDED(hr))
            {
                throw System_Error(hr, "Can't get args");
            }

            CComVariant args;
            hr = args_node->get_nodeTypedValue(&args);
            if (! SUCCEEDED(hr))
            {
                throw System_Error(hr, "Can't get args value");
            }
            // commands.push_back(extension.bstrVal);
        }
    }
}

/* commands:

<commands>
<command>
  <program>C:\Users\Dad\Dad\Roaming\npm\csslint.cmd</program>
  <args>--format=checkstyle-xml</args>
</command>
</commands>
*/
/*CComQIPtr<IXMLDOMElement> element(node);
Linter linter;
CComVariant value;

element->getAttribute(static_cast<bstr_t>(L"extension"), &value);
linter.extension_ = value.bstrVal;

element->getAttribute(static_cast<bstr_t>(L"command"), &value);
linter.command_ = value.bstrVal;

element->getAttribute(static_cast<bstr_t>(L"stdin"), &value);
linter.use_stdin_ = value.boolVal;

linters_.push_back(linter);
*/

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

CComPtr<IXMLDOMNodeList> XMLNodeList{settings.getNodeList("//linter")};

hr = XMLNodeList->get_length(&nodes);
if (! SUCCEEDED(hr))
{
    throw System_Error(hr, "Can't get XPath length");
}

for (LONG entry = 0; entry < nodes; entry++)
{
    CComPtr<IXMLDOMNode> node;
    hr = XMLNodeList->nextNode(&node);
    if (SUCCEEDED(hr) && node != nullptr)
    {
        CComQIPtr<IXMLDOMElement> element(node);
        Linter linter;
        CComVariant value;

        element->getAttribute(static_cast<bstr_t>(L"extension"), &value);
        linter.extension_ = value.bstrVal;

        element->getAttribute(static_cast<bstr_t>(L"command"), &value);
        linter.command_ = value.bstrVal;

        element->getAttribute(static_cast<bstr_t>(L"stdin"), &value);
        linter.use_stdin_ = value.boolVal;

        linters_.push_back(linter);
    }
}
*/

}    // namespace Linter
