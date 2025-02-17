#include "Dom_Document.h"

#include "System_Error.h"
#include "XML_Decode_Error.h"

#include <atlcomcli.h>
#include <comutil.h>
#include <intsafe.h>
#include <minwindef.h>    //For FALSE
#include <msxml.h>
#include <msxml6.h>

#include <string>

#include <iostream>

namespace Linter
{

Linter::Dom_Document::Dom_Document(
    std::filesystem::path const &xml_file, CComPtr<IXMLDOMSchemaCollection2> &schemas)
{
    init();

    {
        // Try parsing new form of xml file with xsd file
        std::filesystem::path new_xml(xml_file.parent_path());
        new_xml.append("linterv2.xml");
        CComVariant value2{new_xml.c_str()};

        CComPtr<IXMLDOMDocument2> pXD;
        HRESULT hr = pXD.CoCreateInstance(__uuidof(DOMDocument60)); //You need 60 or you can't attatch a schema cache.
        if (! SUCCEEDED(hr))
        {
            throw System_Error(hr, "Can't create DOMDocument");
        }
        pXD->put_async(VARIANT_FALSE);
        pXD->put_validateOnParse(VARIANT_TRUE);
        pXD->put_resolveExternals(VARIANT_TRUE);

        // Assign the schema cache to the DOMDocument's schemas collection.
        hr = pXD->putref_schemas(CComVariant(schemas));
        if (! SUCCEEDED(hr))
        {
            throw System_Error(hr, "Can't use schema collection");
        }        

        VARIANT_BOOL res;
        hr = pXD->load(value2, &res);
        if (! SUCCEEDED(hr) || res == VARIANT_FALSE)
        {
            std::cerr << "Bad things\n";
            CComPtr<IXMLDOMParseError> error;
            pXD->get_parseError(&error);
            XML_Decode_Error xerr(*(error.p));
            (void)xerr;

            /*
            try
            {
                checkLoadResults(res, hr);
            }
            catch (std::exception const&)
            {

            }
            */
        }
        else
        {
            std::cout << "boogie\n";
        }
    }

    CComVariant value{xml_file.c_str()};
    VARIANT_BOOL resultCode = FALSE;
    HRESULT const hr = document_->load(value, &resultCode);

    checkLoadResults(resultCode, hr);
}

Dom_Document::Dom_Document(std::string const &xml)
{
    init();

    VARIANT_BOOL resultCode = FALSE;
    HRESULT const hr =
        document_->loadXML(static_cast<_bstr_t>(xml.c_str()), &resultCode);

    checkLoadResults(resultCode, hr);
}

Dom_Document::~Dom_Document() = default;

CComPtr<IXMLDOMNodeList> Dom_Document::getNodeList(std::string const &xpath)
{
    CComPtr<IXMLDOMNodeList> nodes;
    HRESULT const hr =
        document_->selectNodes(static_cast<_bstr_t>(xpath.c_str()), &nodes);
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't execute XPath " + xpath);
    }
    return nodes;
}

void Dom_Document::init()
{
    HRESULT hr = document_.CoCreateInstance(__uuidof(DOMDocument60));
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't create IID_IXMLDOMDocument2");
    }

    hr = document_->put_async(VARIANT_FALSE);
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr, "Can't XMLDOMDocument2::put_async");
    }
}

void Dom_Document::checkLoadResults(VARIANT_BOOL resultcode, HRESULT hr)
{
    if (! SUCCEEDED(hr))
    {
        throw System_Error(hr);
    }
    if (resultcode == VARIANT_FALSE)
    {
        CComPtr<IXMLDOMParseError> error;
        document_->get_parseError(&error);
        throw XML_Decode_Error(*(error.p));
    }
}

}    // namespace Linter
