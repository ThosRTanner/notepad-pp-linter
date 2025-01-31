#pragma once

#include "Handle_Wrapper.h"

namespace Linter
{

class FilePipe
{
  public:
    struct Pipe
    {
        Handle_Wrapper reader;
        Handle_Wrapper writer;
    };

    static Pipe create();
    static void detachFromParent(Handle_Wrapper const &handle);
};

}    // namespace Linter
