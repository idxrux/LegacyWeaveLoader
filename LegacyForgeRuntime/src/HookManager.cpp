#include "HookManager.h"
#include "GameHooks.h"
#include "ModAtlas.h"
#include "SymbolResolver.h"
#include "CreativeInventory.h"
#include "MainMenuOverlay.h"
#include "GameObjectFactory.h"
#include "LogUtil.h"
#include <MinHook.h>

bool HookManager::Install(const SymbolResolver& symbols)
{
    if (MH_Initialize() != MH_OK)
    {
        LogUtil::Log("[LegacyForge] MH_Initialize failed");
        return false;
    }

    if (symbols.pRunStaticCtors)
    {
        if (MH_CreateHook(symbols.pRunStaticCtors,
                          reinterpret_cast<void*>(&GameHooks::Hooked_RunStaticCtors),
                          reinterpret_cast<void**>(&GameHooks::Original_RunStaticCtors)) != MH_OK)
        {
            LogUtil::Log("[LegacyForge] Failed to hook RunStaticCtors");
            return false;
        }
        LogUtil::Log("[LegacyForge] Hooked RunStaticCtors");
    }

    if (symbols.pMinecraftTick)
    {
        if (MH_CreateHook(symbols.pMinecraftTick,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MinecraftTick),
                          reinterpret_cast<void**>(&GameHooks::Original_MinecraftTick)) != MH_OK)
        {
            LogUtil::Log("[LegacyForge] Failed to hook Minecraft::tick");
            return false;
        }
        LogUtil::Log("[LegacyForge] Hooked Minecraft::tick");
    }

    if (symbols.pMinecraftInit)
    {
        if (MH_CreateHook(symbols.pMinecraftInit,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MinecraftInit),
                          reinterpret_cast<void**>(&GameHooks::Original_MinecraftInit)) != MH_OK)
        {
            LogUtil::Log("[LegacyForge] Failed to hook Minecraft::init");
            return false;
        }
        LogUtil::Log("[LegacyForge] Hooked Minecraft::init");
    }

    if (symbols.pExitGame)
    {
        if (MH_CreateHook(symbols.pExitGame,
                          reinterpret_cast<void*>(&GameHooks::Hooked_ExitGame),
                          reinterpret_cast<void**>(&GameHooks::Original_ExitGame)) != MH_OK)
        {
            LogUtil::Log("[LegacyForge] Warning: Failed to hook ExitGame (shutdown hook unavailable)");
        }
        else
        {
            LogUtil::Log("[LegacyForge] Hooked ExitGame");
        }
    }

    GameObjectFactory::ResolveSymbols(const_cast<SymbolResolver&>(symbols));

    if (symbols.pLoadUVs && symbols.pSimpleIconCtor && symbols.pOperatorNew)
    {
        ModAtlas::SetInjectSymbols(symbols.pSimpleIconCtor, symbols.pOperatorNew);
        if (MH_CreateHook(symbols.pLoadUVs,
                          reinterpret_cast<void*>(&GameHooks::Hooked_LoadUVs),
                          reinterpret_cast<void**>(&GameHooks::Original_LoadUVs)) != MH_OK)
        {
            LogUtil::Log("[LegacyForge] Warning: Failed to hook PreStitchedTextureMap::loadUVs (mod textures may not appear)");
        }
        else
        {
            LogUtil::Log("[LegacyForge] Hooked PreStitchedTextureMap::loadUVs (mod texture injection)");
        }

        if (symbols.pRegisterIcon)
        {
            if (MH_CreateHook(symbols.pRegisterIcon,
                              reinterpret_cast<void*>(&GameHooks::Hooked_RegisterIcon),
                              reinterpret_cast<void**>(&GameHooks::Original_RegisterIcon)) != MH_OK)
            {
                LogUtil::Log("[LegacyForge] Warning: Failed to hook PreStitchedTextureMap::registerIcon");
            }
            else
            {
                LogUtil::Log("[LegacyForge] Hooked PreStitchedTextureMap::registerIcon (mod icon lookup)");
                // Pass the trampoline to ModAtlas so it can look up vanilla icons
                // for copying internal state (field_0x48 source image pointer).
                ModAtlas::SetRegisterIconFn(GameHooks::Original_RegisterIcon);
            }
        }
    }
    else if (symbols.pLoadUVs)
    {
        LogUtil::Log("[LegacyForge] Mod texture injection unavailable: SimpleIcon/operator new not resolved");
    }

    if (symbols.pCreativeStaticCtor)
    {
        CreativeInventory::ResolveSymbols(const_cast<SymbolResolver&>(symbols));

        if (MH_CreateHook(symbols.pCreativeStaticCtor,
                          reinterpret_cast<void*>(&GameHooks::Hooked_CreativeStaticCtor),
                          reinterpret_cast<void**>(&GameHooks::Original_CreativeStaticCtor)) != MH_OK)
        {
            LogUtil::Log("[LegacyForge] Warning: Failed to hook CreativeStaticCtor");
        }
        else
        {
            LogUtil::Log("[LegacyForge] Hooked CreativeStaticCtor");
        }
    }

    if (symbols.pMainMenuCustomDraw)
    {
        if (MH_CreateHook(symbols.pMainMenuCustomDraw,
                          reinterpret_cast<void*>(&GameHooks::Hooked_MainMenuCustomDraw),
                          reinterpret_cast<void**>(&GameHooks::Original_MainMenuCustomDraw)) != MH_OK)
        {
            LogUtil::Log("[LegacyForge] Warning: Failed to hook MainMenuCustomDraw");
        }
        else
        {
            LogUtil::Log("[LegacyForge] Hooked MainMenuCustomDraw");
        }
    }

    if (symbols.pPresent)
    {
        MainMenuOverlay::ResolveSymbols(const_cast<SymbolResolver&>(symbols));

        if (MH_CreateHook(symbols.pPresent,
                          reinterpret_cast<void*>(&GameHooks::Hooked_Present),
                          reinterpret_cast<void**>(&GameHooks::Original_Present)) != MH_OK)
        {
            LogUtil::Log("[LegacyForge] Warning: Failed to hook C4JRender::Present");
        }
        else
        {
            LogUtil::Log("[LegacyForge] Hooked C4JRender::Present");
        }
    }

    if (symbols.pGetString)
    {
        if (MH_CreateHook(symbols.pGetString,
                          reinterpret_cast<void*>(&GameHooks::Hooked_GetString),
                          reinterpret_cast<void**>(&GameHooks::Original_GetString)) != MH_OK)
        {
            LogUtil::Log("[LegacyForge] Warning: Failed to hook CMinecraftApp::GetString (mod names unavailable)");
        }
        else
        {
            LogUtil::Log("[LegacyForge] Hooked CMinecraftApp::GetString (mod localization)");
        }
    }

    if (symbols.pGetResourceAsStream)
    {
        if (MH_CreateHook(symbols.pGetResourceAsStream,
                          reinterpret_cast<void*>(&GameHooks::Hooked_GetResourceAsStream),
                          reinterpret_cast<void**>(&GameHooks::Original_GetResourceAsStream)) != MH_OK)
        {
            LogUtil::Log("[LegacyForge] Warning: Failed to hook InputStream::getResourceAsStream (mod atlas unavailable)");
        }
        else
        {
            LogUtil::Log("[LegacyForge] Hooked InputStream::getResourceAsStream (mod textures)");
        }
    }

    {
        void* pOutputDbgStr = reinterpret_cast<void*>(
            GetProcAddress(GetModuleHandleA("kernel32.dll"), "OutputDebugStringA"));
        if (pOutputDbgStr)
        {
            if (MH_CreateHook(pOutputDbgStr,
                              reinterpret_cast<void*>(&GameHooks::Hooked_OutputDebugStringA),
                              reinterpret_cast<void**>(&GameHooks::Original_OutputDebugStringA)) != MH_OK)
            {
                LogUtil::Log("[LegacyForge] Warning: Failed to hook OutputDebugStringA");
            }
            else
            {
                LogUtil::Log("[LegacyForge] Hooked OutputDebugStringA (game log capture)");
            }
        }
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
    {
        LogUtil::Log("[LegacyForge] MH_EnableHook(MH_ALL_HOOKS) failed");
        return false;
    }

    LogUtil::Log("[LegacyForge] All hooks installed and enabled");
    return true;
}

void HookManager::Cleanup()
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}
