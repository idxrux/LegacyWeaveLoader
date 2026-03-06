#include "SymbolResolver.h"
#include <cstdio>
#include <cstring>

#pragma comment(lib, "dbghelp.lib")

// Exact MSVC-decorated symbol names from the game's PDB.
// These are verified against the actual Minecraft.Client.pdb.
static const char* SYM_RUN_STATIC_CTORS = "?MinecraftWorld_RunStaticCtors@@YAXXZ";
static const char* SYM_MINECRAFT_TICK   = "?tick@Minecraft@@QEAAX_N0@Z";
static const char* SYM_MINECRAFT_INIT   = "?init@Minecraft@@QEAAXXZ";
static const char* SYM_EXIT_GAME        = "?ExitGame@CConsoleMinecraftApp@@UEAAXXZ";

bool SymbolResolver::Initialize()
{
    m_process = GetCurrentProcess();

    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_DEBUG);

    if (!SymInitialize(m_process, nullptr, TRUE))
    {
        printf("[LegacyForge] SymInitialize failed (error %lu)\n", GetLastError());
        return false;
    }

    m_initialized = true;
    return true;
}

void* SymbolResolver::Resolve(const char* decoratedName)
{
    if (!m_initialized) return nullptr;

    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)];
    memset(buffer, 0, sizeof(buffer));

    SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    if (SymFromName(m_process, decoratedName, symbol))
    {
        return reinterpret_cast<void*>(symbol->Address);
    }

    printf("[LegacyForge] SymFromName failed for '%s' (error %lu)\n", decoratedName, GetLastError());
    return nullptr;
}

bool SymbolResolver::ResolveGameFunctions()
{
    pRunStaticCtors = Resolve(SYM_RUN_STATIC_CTORS);
    pMinecraftTick  = Resolve(SYM_MINECRAFT_TICK);
    pMinecraftInit  = Resolve(SYM_MINECRAFT_INIT);
    pExitGame       = Resolve(SYM_EXIT_GAME);

    if (pRunStaticCtors) printf("[LegacyForge] RunStaticCtors     @ %p\n", pRunStaticCtors);
    else                 printf("[LegacyForge] MISSING: MinecraftWorld_RunStaticCtors\n");

    if (pMinecraftTick)  printf("[LegacyForge] Minecraft::tick    @ %p\n", pMinecraftTick);
    else                 printf("[LegacyForge] MISSING: Minecraft::tick\n");

    if (pMinecraftInit)  printf("[LegacyForge] Minecraft::init    @ %p\n", pMinecraftInit);
    else                 printf("[LegacyForge] MISSING: Minecraft::init\n");

    if (pExitGame)       printf("[LegacyForge] ExitGame           @ %p\n", pExitGame);
    else                 printf("[LegacyForge] MISSING: CConsoleMinecraftApp::ExitGame\n");

    // RunStaticCtors, tick, and init are critical. ExitGame is optional (graceful shutdown).
    return pRunStaticCtors && pMinecraftTick && pMinecraftInit;
}

void SymbolResolver::Cleanup()
{
    if (m_initialized)
    {
        SymCleanup(m_process);
        m_initialized = false;
    }
}
