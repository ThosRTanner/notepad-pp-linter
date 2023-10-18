#include "stdafx.h"
#include "Settings.h"

#include "DomDocument.h"
#include "HandleWrapper.h"
#include "SystemError.h"

#if __cplusplus >= 201703L
#include <filesystem>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <sstream>

namespace Linter
{

    Settings::Settings(wchar_t const *settings_xml) : m_settings_xml(settings_xml), m_alpha(-1), m_color(-1)
#if __cplusplus < 201703L
          ,
          m_last_update_time(0)
#endif
    {
    }

    void Settings::refresh()
    {
#if __cplusplus >= 201703L
        auto const last_write_time{std::filesystem::last_write_time(m_settings_xml)};
#else
        //This is majorly horrible as I have to open a handle to the file
        uint64_t last_write_time;
        {
            HandleWrapper const handle{CreateFileW(m_settings_xml.c_str(),
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr)};
            if (! GetFileTime(handle, nullptr, nullptr, reinterpret_cast<FILETIME *>(&last_write_time)))
            {
                throw SystemError();
            }
        }
#endif
        if (last_write_time != m_last_update_time)
        {
            read_settings();
            m_last_update_time = last_write_time;
        }
    }

    void Settings::read_settings()
    {
        m_alpha = -1;
        m_color = -1;
        m_linters.clear();

        DomDocument XMLDocument(m_settings_xml);
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
            if (element->getAttribute(bstr_t(L"alpha"), &value) == S_OK)
            {
                int alpha = 0;
                if (value.bstrVal)
                {
                    std::wstringstream data{std::wstring(value.bstrVal, SysStringLen(value.bstrVal))};
                    data >> alpha;
                }
                m_alpha = alpha;
            }

            if (element->getAttribute(bstr_t(L"color"), &value) == S_OK)
            {
                unsigned int color(0);
                if (value.bstrVal)
                {
                    std::wstringstream data{std::wstring(value.bstrVal, SysStringLen(value.bstrVal))};
                    data >> std::hex >> color;
                }

                // reverse colors for scintilla's LE order
                m_color = 0;
                for (int i = 0; i < 3; i += 1)
                {
                    m_color = (m_color << 8) | (color & 0xff);
                    color >>= 8;
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

                element->getAttribute(bstr_t(L"extension"), &value);
                linter.m_extension = value.bstrVal;

                element->getAttribute(bstr_t(L"command"), &value);
                linter.m_command = value.bstrVal;

                element->getAttribute(bstr_t(L"stdin"), &value);
                linter.m_useStdin = value.boolVal != 0; //Is that strictly necessary?

                m_linters.push_back(linter);
            }
        }
    }

}    // namespace Linter
