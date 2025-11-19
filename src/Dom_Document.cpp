#include "Dom_Document.h"

#include "Dom_Node.h"
#include "Dom_Node_List.h"
#include "System_Error.h"
#include "XML_Decode_Error.h"

#include <atlcomcli.h>
#include <comutil.h>
#include <intsafe.h>
#include <minwindef.h>    //For FALSE
#include <msxml.h>
#include <msxml6.h>
#include <winerror.h>

#include <optional>
#include <string>

namespace Linter
{

Linter::Dom_Document::Dom_Document(
    std::filesystem::path const &xml_file,
    CComPtr<IXMLDOMSchemaCollection2> &schemas
)
{
    init();

    document_->put_validateOnParse(VARIANT_TRUE);
    // Not sure what this does but it doesn't seem to be  necessary.
    // pXD->put_resolveExternals(VARIANT_TRUE);

    // Treat whitespace according to the schema.
    HRESULT hres = document_->put_preserveWhiteSpace(FALSE);
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres, "Can't set preserveWhiteSpace");
    }

    // Assign the schema cache to the DOMDocument's schemas collection.
    hres = document_->putref_schemas(CComVariant(schemas));
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres, "Can't use schema collection");
    }

    CComVariant const value{xml_file.c_str()};
    VARIANT_BOOL resultCode = FALSE;
    hres = document_->load(value, &resultCode);

    checkLoadResults(resultCode, hres);
}

Dom_Document::Dom_Document(std::string const &xml)
{
    init();

    VARIANT_BOOL resultCode = FALSE;
    HRESULT const hres =
        document_->loadXML(static_cast<_bstr_t>(xml.c_str()), &resultCode);

    checkLoadResults(resultCode, hres);
}

Dom_Document::~Dom_Document() = default;

Dom_Node_List Dom_Document::get_node_list(std::string const &xpath) const
{
    CComPtr<IXMLDOMNodeList> nodes;
    HRESULT const hres =
        document_->selectNodes(static_cast<_bstr_t>(xpath.c_str()), &nodes);
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres, "Can't execute XPath " + xpath);
    }
    return Dom_Node_List(nodes);
}

std::optional<Dom_Node> Dom_Document::get_node(std::string const &xpath) const
{
    CComPtr<IXMLDOMNode> node;
    HRESULT const hres =
        document_->selectSingleNode(static_cast<_bstr_t>(xpath.c_str()), &node);
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres, "Can't execute XPath " + xpath);
    }
    if (hres == S_FALSE)
    {
        return std::nullopt;
    }
    return Dom_Node(node);
}

void Dom_Document::init()
{
    HRESULT hres = document_.CoCreateInstance(__uuidof(DOMDocument60));
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres, "Can't create IID_IXMLDOMDocument2");
    }

    hres = document_->put_async(VARIANT_FALSE);
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres, "Can't XMLDOMDocument2::put_async");
    }
}

//NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Dom_Document::checkLoadResults(VARIANT_BOOL resultcode, HRESULT hres) const
{
    if (not SUCCEEDED(hres))
    {
        throw System_Error(hres);
    }
    if (resultcode == VARIANT_FALSE)
    {
        CComPtr<IXMLDOMParseError> error;
        document_->get_parseError(&error);
        throw XML_Decode_Error(*(error.p));
    }
}

}    // namespace Linter
