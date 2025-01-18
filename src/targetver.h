#pragma once

// The following macros define the minimum required platform.  The minimum required platform
// is the earliest version of Windows, Internet Explorer etc. that has the necessary features to run
// your application.  The macros work by enabling all features available on platform versions up to and
// including the version specified.

//These seem to be a bit out of date.
#define NTDDI_VERSION NTDDI_VISTA
#define _WIN32_WINNT _WIN32_WINNT_VISTA
//#define NTDDI_VERSION NTDDI_WIN7
//#define _WIN32_WINNT _WIN32_WINNT_WIN7
#include <sdkddkver.h>
