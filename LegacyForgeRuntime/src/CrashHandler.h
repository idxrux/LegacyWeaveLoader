#pragma once
#include <Windows.h>

namespace CrashHandler
{
    // Installs the vectored exception handler. Safe to call from DllMain.
    void Install(HMODULE runtimeModule);
}
