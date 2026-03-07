#pragma once
#include <cstdint>

namespace PdbParser
{
    bool Open(const char* pdbPath);

    // Returns the RVA for a decorated symbol name, or 0 on failure.
    uint32_t FindSymbolRVA(const char* decoratedName);

    // Logs all symbols whose name contains the given substring (for debugging).
    void DumpMatching(const char* substring);

    void Close();
}
