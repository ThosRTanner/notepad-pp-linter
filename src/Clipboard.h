#pragma once

#include <minwindef.h>
#include <windef.h>

#include <string>

namespace Linter
{

class Clipboard
{
  public:
    explicit Clipboard(HWND self);

    Clipboard(Clipboard const &) = delete;
    Clipboard(Clipboard &&) = delete;

    Clipboard operator=(Clipboard const &) = delete;
    Clipboard operator=(Clipboard &&) = delete;

    ~Clipboard();

    void empty();

    void copy(std::wstring const &str);

  private:
    HGLOBAL mem_handle_ = nullptr;
};

}    // namespace Linter
