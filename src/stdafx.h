// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN    // Exclude rarely-used stuff from Windows headers
#define NOMINMAX               // Exclude min and max

// Windows Header Files:
#include <windows.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS    // some CString constructors will
                                              // be explicit

// #include <atlcomcli.h>
// #include <comdef.h>
