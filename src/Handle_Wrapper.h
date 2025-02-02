#pragma once

#include <winnt.h>    // For HANDLE

#include <string>

namespace Linter
{

class Handle_Wrapper
{
  public:
    explicit Handle_Wrapper(HANDLE h);
    Handle_Wrapper(Handle_Wrapper const &) = delete;
    Handle_Wrapper(Handle_Wrapper &&other) noexcept;
    Handle_Wrapper &operator=(Handle_Wrapper const &) = delete;
    Handle_Wrapper &operator=(Handle_Wrapper &&other) = delete;
    ~Handle_Wrapper();

    void close() const noexcept;

    operator HANDLE() const noexcept;

    /** Write a string to the handle
     *
     * @param str - string to write
     */
    void writeFile(std::string const &str) const;

    /** Read the entire file */
    std::string readFile() const;

  private:
    mutable HANDLE handle_;
};

}    // namespace Linter
