#include "LogUtil.h"
#include <cstdio>
#include <cstdarg>
#include <string>

static std::string s_logPath;

namespace LogUtil
{

void SetBaseDir(const char* baseDir)
{
    s_logPath = std::string(baseDir) + "legacyforge.log";
}

void Log(const char* fmt, ...)
{
    // Also print to stdout for console visibility
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");

    if (s_logPath.empty()) return;

    FILE* f = nullptr;
    fopen_s(&f, s_logPath.c_str(), "a");
    if (f)
    {
        va_list args2;
        va_start(args2, fmt);
        vfprintf(f, fmt, args2);
        va_end(args2);
        fprintf(f, "\n");
        fclose(f);
    }
}

} // namespace LogUtil
