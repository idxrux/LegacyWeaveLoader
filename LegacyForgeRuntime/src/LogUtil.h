#pragma once

namespace LogUtil
{
    // Must be called once at startup with the runtime DLL's directory (with trailing backslash)
    void SetBaseDir(const char* baseDir);

    // Appends a line to legacyforge.log in the base directory
    void Log(const char* fmt, ...);
}
