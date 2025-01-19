#include "stdafx.h"
#include "Settings.h"

#include "DomDocument.h"
#include "HandleWrapper.h"
#include "SystemError.h"

#include <filesystem>

#include <sstream>

Linter::Settings::Settings(std::wstring const &settings_xml) :
    settings_xml_(settings_xml)
{
}

void Linter::Settings::refresh()
{
    auto const last_write_time{std::filesystem::last_write_time(settings_xml_)};
    if (last_write_time != last_update_time_)
    {
        read_settings();
        last_update_time_ = last_write_time;
    }
}

void Linter::Settings::read_settings()
{
    alpha_ = -1;
    colour_ = -1;
    linters_.clear();

    DomDocument XMLDocument(settings_xml_);
    CComPtr<IXMLDOMNodeList> styleNode{XMLDocument.getNodeList("//style")};

    LONG uLength;
    HRESULT hr = styleNode->get_length(&uLength);
    if (!SUCCEEDED(hr))
    {
        throw SystemError(hr, "Can't get XPath style length");
    }

    if (uLength != 0)
    {
        CComPtr<IXMLDOMNode> node;
        hr = styleNode->nextNode(&node);
        if (!SUCCEEDED(hr))
        {
            throw SystemError(hr, "Can't read style node");
        }
        CComQIPtr<IXMLDOMElement> element(node);
        CComVariant value;
        if (element->getAttribute(static_cast<bstr_t>(L"alpha"), &value) == S_OK)
        {
            int alphaVal{0};
            if (value.bstrVal)
            {
                std::wstringstream data{std::wstring(value.bstrVal, SysStringLen(value.bstrVal))};
                data >> alphaVal;
            }
            alpha_ = alphaVal;
        }

        if (element->getAttribute(static_cast<bstr_t>(L"color"), &value) == S_OK)
        {
            unsigned int colorVal{0};
            if (value.bstrVal)
            {
                std::wstringstream data{std::wstring(value.bstrVal, SysStringLen(value.bstrVal))};
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

    // <error line="12" column="19" severity="error" message="Unexpected identifier" source="jscs" />
    CComPtr<IXMLDOMNodeList> XMLNodeList{XMLDocument.getNodeList("//linter")};

    hr = XMLNodeList->get_length(&uLength);
    if (!SUCCEEDED(hr))
    {
        throw SystemError(hr, "Can't get XPath length");
    }

    for (int iIndex = 0; iIndex < uLength; iIndex++)
    {
        CComPtr<IXMLDOMNode> node;
        hr = XMLNodeList->nextNode(&node);
        if (SUCCEEDED(hr) && node != nullptr)
        {
            CComQIPtr<IXMLDOMElement> element(node);
            Linter linter;
            CComVariant value;

            element->getAttribute(static_cast<bstr_t>(L"extension"), &value);
            linter.m_extension = value.bstrVal;

            element->getAttribute(static_cast<bstr_t>(L"command"), &value);
            linter.m_command = value.bstrVal;

            element->getAttribute(static_cast<bstr_t>(L"stdin"), &value);
            linter.m_useStdin = value.boolVal != 0;    //Is that strictly necessary?

            linters_.push_back(linter);
        }
    }
}
