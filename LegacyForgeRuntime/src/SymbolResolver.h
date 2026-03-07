#pragma once
#include <Windows.h>

class SymbolResolver
{
public:
    bool Initialize();
    bool ResolveGameFunctions();
    void Cleanup();

    void* Resolve(const char* decoratedName);

    void* pRunStaticCtors = nullptr;       // MinecraftWorld_RunStaticCtors
    void* pMinecraftTick = nullptr;        // Minecraft::tick(bool, bool)
    void* pMinecraftInit = nullptr;        // Minecraft::init()
    void* pExitGame = nullptr;             // CConsoleMinecraftApp::ExitGame()
    void* pCreativeStaticCtor = nullptr;   // IUIScene_CreativeMenu::staticCtor()
    void* pMainMenuCustomDraw = nullptr;   // UIScene_MainMenu::customDraw()
    void* pPresent = nullptr;              // C4JRender::Present()

private:
    uintptr_t m_moduleBase = 0;
    bool m_initialized = false;
};
