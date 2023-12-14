#pragma once
#include <exception>

struct IXMLDOMParseError;

namespace Linter
{
    class XmlDecodeException : public std::exception
    {
      public:
        explicit XmlDecodeException(IXMLDOMParseError *);

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
            return m_line;
        }

        /** Column in line in xml file at which error occured */
        long column() const noexcept
        {
            return m_column;
        }

      private:
        //value of what()
        char m_buff[2048];

        //Line at which error detected
        long m_line;

        //Column at which error detected
        long m_column;
    };
}    // namespace Linter
