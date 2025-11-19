#pragma once
#include <exception>

// clang-tidy is being silly here
// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
struct IXMLDOMParseError;

namespace Linter
{

class XML_Decode_Error : public std::exception
{
  public:
    explicit XML_Decode_Error(IXMLDOMParseError &);

    XML_Decode_Error(XML_Decode_Error const &) noexcept;
    XML_Decode_Error(XML_Decode_Error &&) noexcept;
    XML_Decode_Error &operator=(XML_Decode_Error const &) noexcept;
    XML_Decode_Error &operator=(XML_Decode_Error &&) noexcept;
    ~XML_Decode_Error() override;

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
