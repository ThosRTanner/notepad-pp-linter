#include "stdafx.h"
#include "XmlParser.h"
#include "encoding.h"

#include <msxml6.h>
#pragma comment(lib, "msxml6.lib")

#define RELEASE(iface)    \
    if (iface)            \
    {                     \
        iface->Release(); \
        iface = NULL;     \
    }

std::vector<XmlParser::Error> XmlParser::getErrors(const std::string &xml)
{
    std::vector<XmlParser::Error> errors;
    IXMLDOMNodeList *XMLNodeList(NULL);
    IXMLDOMDocument2 *XMLDocument(NULL);
    HRESULT hr;

    try
    {
        hr = CoCreateInstance(__uuidof(DOMDocument), NULL, CLSCTX_SERVER, IID_IXMLDOMDocument2, (LPVOID *)(&XMLDocument));
        if (!SUCCEEDED(hr) || !XMLDocument)
        {
            throw ::Linter::Exception("Linter: Can't create IID_IXMLDOMDocument2");
        }

        hr = XMLDocument->put_async(VARIANT_FALSE);
        if (!SUCCEEDED(hr))
        {
            throw ::Linter::Exception("Linter: Can't XMLDOMDocument2::put_async");
        }

        const std::wstring &string = Encoding::toUnicode(xml);
        BSTR bstrValue(bstr_t(string.c_str()));

        VARIANT_BOOL resultCode = FALSE;
        hr = XMLDocument->loadXML(bstrValue, &resultCode);
        if (!SUCCEEDED(hr) || (resultCode != VARIANT_TRUE))
        {
            throw ::Linter::Exception("Linter: Invalid output format. Only checkstyle-compatible output allowed.");
        }

        // <error line="12" column="19" severity="error" message="Unexpected identifier" source="jscs" />
        hr = XMLDocument->selectNodes(bstr_t(L"//error"), &XMLNodeList);
        if (!SUCCEEDED(hr))
        {
            throw ::Linter::Exception("Linter: Can't execute XPath //error");
        }

        //Why do we need unlength if we're using nextNode?
        LONG uLength;

        hr = XMLNodeList->get_length(&uLength);
        if (!SUCCEEDED(hr))
        {
            throw ::Linter::Exception("Linter: Can't get XPath //error length");
        }
        for (int iIndex = 0; iIndex < uLength; iIndex++)
        {
            IXMLDOMNode *node;
            hr = XMLNodeList->nextNode(&node);
            if (!SUCCEEDED(hr))
            {
                throw ::Linter::Exception("Linter: Can't get next XPath element");
            }
            CComQIPtr<IXMLDOMElement> element(node);
            if (!SUCCEEDED(hr) && element)
            {
                throw ::Linter::Exception("Linter: XPath Element error");
            }
            Error error;
            CComVariant value;

            element->getAttribute(bstr_t(L"line"), &value);
            error.m_line = _wtoi(value.bstrVal);

            element->getAttribute(bstr_t(L"column"), &value);
            error.m_column = _wtoi(value.bstrVal);

            element->getAttribute(bstr_t(L"message"), &value);
            error.m_message = value.bstrVal;

            errors.push_back(error);
            RELEASE(node);
        }
        RELEASE(XMLNodeList);
        RELEASE(XMLDocument);
    }
    catch (std::exception const &)
    {
        RELEASE(XMLDocument);
        RELEASE(XMLNodeList);

        throw;
    }

    return errors;
}

XmlParser::Settings XmlParser::getLinters(std::wstring file)
{
    XmlParser::Settings settings;
    IXMLDOMNodeList *XMLNodeList(NULL), *styleNode(NULL);
    IXMLDOMDocument2 *XMLDocument(NULL);
    HRESULT hr;
    LONG uLength;

    try
    {
        hr = CoCreateInstance(__uuidof(DOMDocument), NULL, CLSCTX_SERVER, IID_IXMLDOMDocument2, (LPVOID *)(&XMLDocument));
        if (!SUCCEEDED(hr) || !XMLDocument)
        {
            throw ::Linter::Exception("Linter: Can't create IID_IXMLDOMDocument2");
        }

        hr = XMLDocument->put_async(VARIANT_FALSE);
        if (!SUCCEEDED(hr))
        {
            throw ::Linter::Exception("Linter: XMLDOMDocument2::put_async error.");
        }

        BSTR bstrValue(bstr_t(file.c_str()));
        CComVariant value(bstrValue);

            short resultCode = FALSE;
            hr = XMLDocument->load(value, &resultCode);
            if (!SUCCEEDED(hr) || (resultCode != VARIANT_TRUE))
            {
                throw ::Linter::Exception("Linter: linter.xml load error. Check file format.");
            }
        }

        hr = XMLDocument->selectNodes(bstr_t(L"//style"), &styleNode);
        if (!SUCCEEDED(hr))
        {
            throw ::Linter::Exception("Linter: Can't execute XPath //style");
        }

        //Why do we need to get the length if we're going to use nextNode?
        hr = styleNode->get_length(&uLength);
        if (!SUCCEEDED(hr))
        {
            throw ::Linter::Exception("Linter: Can't get XPath root length");
        }

        if (uLength)
        {
            IXMLDOMNode *node;
            hr = styleNode->nextNode(&node);
            if (SUCCEEDED(hr))
            {
                CComQIPtr<IXMLDOMElement> element(node);

                CComVariant alpha;
                if (element->getAttribute(bstr_t(L"alpha"), &alpha) == S_OK)
                {
                    int alpha = 0;
                    if (value.bstrVal)
                    {
                        std::wstringstream data{std::wstring(value.bstrVal, SysStringLen(value.bstrVal))};
                        data >> alpha;
                    }
                    settings.m_alpha = 0;
                }

                if (element->getAttribute(bstr_t(L"color"), &alpha) == S_OK)
                {
                    unsigned int color(0);
                    if (value.bstrVal)
                    {
                        std::wstringstream data{std::wstring(value.bstrVal, SysStringLen(value.bstrVal))};
                        data >> std::hex >> color;
                    }

                    // reverse colors for scintilla's LE order
                    settings.m_color = color & 0xFF;

                    settings.m_color <<= 8;
                    color >>= 8;
                    settings.m_color |= color & 0xFF;

                    settings.m_color <<= 8;
                    color >>= 8;
                    settings.m_color |= color & 0xFF;
                }
            }
        }

        // <error line="12" column="19" severity="error" message="Unexpected identifier" source="jscs" />
        hr = XMLDocument->selectNodes(bstr_t(L"//linter"), &XMLNodeList);
        if (!SUCCEEDED(hr))
        {
            throw ::Linter::Exception("Linter: Can't execute XPath //linter");
        }

        //Why do we need to get the length if we're going to use nextNode?
        hr = XMLNodeList->get_length(&uLength);
        if (!SUCCEEDED(hr))
        {
            throw ::Linter::Exception("Linter: Can't get XPath length");
        }

        for (int iIndex = 0; iIndex < uLength; iIndex++)
        {
            IXMLDOMNode *node;
            hr = XMLNodeList->nextNode(&node);
            if (SUCCEEDED(hr) && node != nullptr)
            {
                CComQIPtr<IXMLDOMElement> element(node);
                Linter linter;
                CComVariant extension;

                    element->getAttribute(bstr_t(L"extension"), &extension);
                    linter.m_extension = extension.bstrVal;

                    element->getAttribute(bstr_t(L"command"), &extension);
                    linter.m_command = extension.bstrVal;

                    element->getAttribute(bstr_t(L"stdin"), &extension);
                    linter.m_useStdin = !!extension.boolVal;

                settings.m_linters.push_back(linter);
                RELEASE(node);
            }
        }
        RELEASE(XMLNodeList);
        RELEASE(XMLDocument);
    }
    catch (std::exception const &)
    {
        RELEASE(XMLDocument);
        RELEASE(XMLNodeList);
        throw;
    }

    return settings;
}
