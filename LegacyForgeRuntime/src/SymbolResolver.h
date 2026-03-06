#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>

class SymbolResolver
{
public:
    bool Initialize();
    bool ResolveGameFunctions();
    void Cleanup();

    void* Resolve(const char* functionName);

    void* pRunStaticCtors = nullptr;    // MinecraftWorld_RunStaticCtors
    void* pMinecraftTick = nullptr;     // Minecraft::tick(bool, bool)
    void* pMinecraftInit = nullptr;     // Minecraft::init()
    void* pExitGame = nullptr;          // CConsoleMinecraftApp::ExitGame()

private:
    HANDLE m_process = nullptr;
    bool m_initialized = false;
};
