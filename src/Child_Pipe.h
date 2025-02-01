#pragma once

#include "Handle_Wrapper.h"

namespace Linter
{

class Child_Pipe
{
  public:
    constexpr Handle_Wrapper const &reader() const noexcept
    {
        return pipes_.reader_;
    };

    constexpr Handle_Wrapper const &writer() const noexcept
    {
        return pipes_.writer_;
    };

    static Child_Pipe create_output_pipe();
    static Child_Pipe create_input_pipe();

  private:
    struct Pipes
    {
        Handle_Wrapper reader_;
        Handle_Wrapper writer_;
    };

    Child_Pipe();
    static Child_Pipe::Pipes create_pipes();
    static void detach(Handle_Wrapper const &);

    Pipes pipes_;

};

}    // namespace Linter
