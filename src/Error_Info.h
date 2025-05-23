#pragma once

#include <string>

namespace Linter
{

struct Error_Info
{
    enum Mode
    {
        Standard,          // Detected by a linter
        Bad_Linter_XML,    // Couldn't parse linter++.xml
        Bad_Output,        // Couldn't parse linter output
        Stderr_Found,      // Information found in stderr
        Exception,         // Unexpected exception when processing file
        Other              // Other exception when processing
    } mode_;
    int line_ = 0;
    int column_ = 0;
    std::wstring message_;
    std::wstring severity_ = L"error";
    std::wstring tool_;
    std::wstring command_;
    std::string stdout_;
    std::string stderr_;
};

}    // namespace Linter
