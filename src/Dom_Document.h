#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <atlcomcli.h>
#include <msxml6.h>
#include <winnt.h>    //For HRESULT
#include <wtypes.h>

namespace Linter
{
class Dom_Node;
class Dom_Node_List;

class Dom_Document
{
    /** Important note:
     * The wstring constructor takes a filename.
     * The string constructor takes an xml string.
     */

  public:
    /** Creates an XML document from the supplied filename and validates against
     * xsd */
    Dom_Document(std::filesystem::path const &, CComPtr<IXMLDOMSchemaCollection2> &);

    /** Creates an XML document from the supplied UTF8 string */
    explicit Dom_Document(std::string const &xml);

    Dom_Document(Dom_Document const &) = delete;
    Dom_Document(Dom_Document &&) = delete;
    Dom_Document &operator=(Dom_Document const &) = delete;
    Dom_Document &operator=(Dom_Document &&) = delete;

    ~Dom_Document();

    /** Get list of nodes selected by supplied XPATH */
    Dom_Node_List get_node_list(std::string const &xpath) const;

    /* Get a single node from selected XPATH */
    std::optional<Dom_Node> get_node(std::string const &xpath) const;

  private:
    /** Set up the dom interface */
    void init();

    /* Check the result of doing a load, die if it didn't complete */
    void checkLoadResults(VARIANT_BOOL resultcode, HRESULT hr) const;

    CComPtr<IXMLDOMDocument2> document_;
};

}    // namespace Linter
