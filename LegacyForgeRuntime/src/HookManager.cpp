#include "HookManager.h"
#include "GameHooks.h"
#include "SymbolResolver.h"
#include "CreativeInventory.h"
#include "MainMenuOverlay.h"
#include <MinHook.h>
#include <cstdio>

bool HookManager::Install(const SymbolResolver& symbols)
{
    if (MH_Initialize() != MH_OK)
    {
        printf("[LegacyForge] MH_Initialize failed\n");
        return false;
    }

    // Hook MinecraftWorld_RunStaticCtors
    if (symbols.pRunStaticCtors)
    {
        if (MH_CreateHook(symbols.pRunStaticCtors,
                          reinterpret_cast<void*>(&GameHooks::Hooked_RunStaticCtors),
                          reinterpret_cast<void**>(&GameHooks::Original_RunStaticCtors)) != MH_OK)
        {
            printf("[LegacyForge] Failed to hook RunStaticCtors\n");
            return false;
        }
        printf("[LegacyForge] Hooked RunStaticCtors\n");
    }

    // Hook Minecraft::tick
    if (symbols.pMinecraftTick)
    {
        if (MH_CreateHook(symbols.pMinecraftTick,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MinecraftTick),
                          reinterpret_cast<void**>(&GameHooks::Original_MinecraftTick)) != MH_OK)
        {
            printf("[LegacyForge] Failed to hook Minecraft::tick\n");
            return false;
        }
        printf("[LegacyForge] Hooked Minecraft::tick\n");
    }

    // Hook Minecraft::init
    if (symbols.pMinecraftInit)
    {
        if (MH_CreateHook(symbols.pMinecraftInit,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MinecraftInit),
                          reinterpret_cast<void**>(&GameHooks::Original_MinecraftInit)) != MH_OK)
        {
            printf("[LegacyForge] Failed to hook Minecraft::init\n");
            return false;
        }
        printf("[LegacyForge] Hooked Minecraft::init\n");
    }

    // Hook CConsoleMinecraftApp::ExitGame
    if (symbols.pExitGame)
    {
        if (MH_CreateHook(symbols.pExitGame,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ExitGame),
                          reinterpret_cast<void**>(&GameHooks::Original_ExitGame)) != MH_OK)
        {
            printf("[LegacyForge] Warning: Failed to hook ExitGame (shutdown hook unavailable)\n");
        }
        else
        {
            printf("[LegacyForge] Hooked ExitGame\n");
        }
    }

    // Hook IUIScene_CreativeMenu::staticCtor
    if (symbols.pCreativeStaticCtor)
    {
        CreativeInventory::ResolveSymbols(const_cast<SymbolResolver&>(symbols));

        if (MH_CreateHook(symbols.pCreativeStaticCtor,
                          reinterpret_cast<void*>(&GameHooks::Hooked_CreativeStaticCtor),
                          reinterpret_cast<void**>(&GameHooks::Original_CreativeStaticCtor)) != MH_OK)
        {
            printf("[LegacyForge] Warning: Failed to hook CreativeStaticCtor\n");
        }
        else
        {
            printf("[LegacyForge] Hooked CreativeStaticCtor\n");
        }
    }

    // Hook UIScene_MainMenu::customDraw (sets main-menu detection flag)
    if (symbols.pMainMenuCustomDraw)
    {
        if (MH_CreateHook(symbols.pMainMenuCustomDraw,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MainMenuCustomDraw),
                          reinterpret_cast<void**>(&GameHooks::Original_MainMenuCustomDraw)) != MH_OK)
        {
            printf("[LegacyForge] Warning: Failed to hook MainMenuCustomDraw\n");
        }
        else
        {
            printf("[LegacyForge] Hooked MainMenuCustomDraw\n");
        }
    }

    // Hook C4JRender::Present (draws branding overlay just before frame present)
    if (symbols.pPresent)
    {
        MainMenuOverlay::ResolveSymbols(const_cast<SymbolResolver&>(symbols));

        if (MH_CreateHook(symbols.pPresent,
                          reinterpret_cast<void*>(&GameHooks::Hooked_Present),
                          reinterpret_cast<void**>(&GameHooks::Original_Present)) != MH_OK)
        {
            printf("[LegacyForge] Warning: Failed to hook C4JRender::Present\n");
        }
        else
        {
            printf("[LegacyForge] Hooked C4JRender::Present\n");
        }
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
    {
        printf("[LegacyForge] MH_EnableHook(MH_ALL_HOOKS) failed\n");
        return false;
    }

    printf("[LegacyForge] All hooks installed and enabled\n");
    return true;
}

void HookManager::Cleanup()
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}
