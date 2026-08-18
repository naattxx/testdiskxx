#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
#include "windows.h"
#elif __has_include("unistd.h")
#include <unistd.h>
#endif
#include "utils.hpp"

auto isAdmin() -> bool
{
    bool isAdmin = false;
#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
    HANDLE tokenHandle = nullptr;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tokenHandle))
    {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);

        if (GetTokenInformation(tokenHandle, TokenElevation, &elevation, sizeof(elevation), &size))
        {
            isAdmin = elevation.TokenIsElevated != 0;
        }
    }

    if (tokenHandle)
        CloseHandle(tokenHandle);

#elif __has_include("unistd.h")
    isAdmin = geteuid() == 0;
#endif

    return isAdmin;
}
