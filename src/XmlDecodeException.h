#pragma once
#include <exception>

struct IXMLDOMParseError;

namespace Linter
{

class XmlDecodeException : public std::exception
{
  public:
    explicit XmlDecodeException(IXMLDOMParseError &);

    XmlDecodeException(XmlDecodeException const &) noexcept;
    XmlDecodeException(XmlDecodeException &&) noexcept;
    XmlDecodeException &operator=(XmlDecodeException const &) noexcept;
    XmlDecodeException &operator=(XmlDecodeException &&) noexcept;
    ~XmlDecodeException();

    /** Returns user-readable string describing error */
    char const *what() const noexcept override;

    /** Line in xml file at which error occured */
    long line() const noexcept
    {
        return line_;
    }

    /** Column in line in xml file at which error occured */
    long column() const noexcept
    {
        return column_;
    }

  private:
    // value of what()
    char what_string_[2048];

    // Line at which error detected
    long line_;

    // Column at which error detected
    long column_;
};

}    // namespace Linter
