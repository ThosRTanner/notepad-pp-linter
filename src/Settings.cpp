#include "Settings.h"

#include "DomDocument.h"
#include "SystemError.h"

#include <atlcomcli.h>
#include <comutil.h>
#include <intsafe.h>
#include <msxml.h>
#include <oleauto.h>

#include <chrono>
#include <filesystem>
#include <sstream>

namespace Linter
{

Settings::Settings(std::filesystem::path const &settings_xml) :
    settings_xml_(settings_xml)
{
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
    alpha_ = -1;
    colour_ = -1;
    linters_.clear();

    DomDocument settings{settings_xml_};
    CComPtr<IXMLDOMNodeList> styleNode{settings.getNodeList("//style")};

    LONG nodes;
    HRESULT hr = styleNode->get_length(&nodes);
    if (! SUCCEEDED(hr))
    {
        throw SystemError(hr, "Can't get XPath style length");
    }

    if (nodes != 0)
    {
        CComPtr<IXMLDOMNode> node;
        hr = styleNode->nextNode(&node);
        if (! SUCCEEDED(hr))
        {
            throw SystemError(hr, "Can't read style node");
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
            alpha_ = alphaVal;
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
            colour_ = 0;
            for (int i = 0; i < 3; i += 1)
            {
                colour_ = (colour_ << 8) | (colorVal & 0xff);
                colorVal >>= 8;
            }
        }
    }

    // <error line="12" column="19" severity="error" message="Unexpected
    // identifier" source="jscs" />
    CComPtr<IXMLDOMNodeList> XMLNodeList{settings.getNodeList("//linter")};

    hr = XMLNodeList->get_length(&nodes);
    if (! SUCCEEDED(hr))
    {
        throw SystemError(hr, "Can't get XPath length");
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
}

}    // namespace Linter
