#pragma once

#include "HandleWrapper.h"

namespace Linter
{

class FilePipe
{
  public:
    struct Pipe
    {
        HandleWrapper reader;
        HandleWrapper writer;
    };

    static Pipe create();
    static void detachFromParent(HandleWrapper const &handle);
};

}    // namespace Linter
