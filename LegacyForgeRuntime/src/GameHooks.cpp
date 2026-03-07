#include "GameHooks.h"
#include "DotNetHost.h"
#include "CreativeInventory.h"
#include "MainMenuOverlay.h"
#include "LogUtil.h"
#include <cstdio>
#include <cstring>

namespace GameHooks
{
    RunStaticCtors_fn     Original_RunStaticCtors = nullptr;
    MinecraftTick_fn      Original_MinecraftTick = nullptr;
    MinecraftInit_fn      Original_MinecraftInit = nullptr;
    ExitGame_fn           Original_ExitGame = nullptr;
    CreativeStaticCtor_fn Original_CreativeStaticCtor = nullptr;
    MainMenuCustomDraw_fn Original_MainMenuCustomDraw = nullptr;
    Present_fn            Original_Present = nullptr;
    OutputDebugStringA_fn Original_OutputDebugStringA = nullptr;

    void Hooked_RunStaticCtors()
    {
        LogUtil::Log("[LegacyForge] Hook: RunStaticCtors -- calling PreInit");
        DotNetHost::CallPreInit();

        Original_RunStaticCtors();

        LogUtil::Log("[LegacyForge] Hook: RunStaticCtors complete -- calling Init");
        DotNetHost::CallInit();
    }

    void __fastcall Hooked_MinecraftTick(void* thisPtr, bool bFirst, bool bUpdateTextures)
    {
        Original_MinecraftTick(thisPtr, bFirst, bUpdateTextures);

        if (bFirst)
        {
            DotNetHost::CallTick();
        }
    }

    void __fastcall Hooked_MinecraftInit(void* thisPtr)
    {
        Original_MinecraftInit(thisPtr);

        LogUtil::Log("[LegacyForge] Hook: Minecraft::init complete -- calling PostInit");
        DotNetHost::CallPostInit();
    }

    void __fastcall Hooked_ExitGame(void* thisPtr)
    {
        LogUtil::Log("[LegacyForge] Hook: ExitGame -- calling Shutdown");
        DotNetHost::CallShutdown();

        Original_ExitGame(thisPtr);
    }

    void Hooked_CreativeStaticCtor()
    {
        LogUtil::Log("[LegacyForge] Hook: CreativeStaticCtor -- building vanilla creative lists");
        Original_CreativeStaticCtor();

        LogUtil::Log("[LegacyForge] Hook: CreativeStaticCtor -- injecting modded items");
        CreativeInventory::InjectItems();
    }

    void __fastcall Hooked_MainMenuCustomDraw(void* thisPtr, void* region)
    {
        MainMenuOverlay::NotifyOnMainMenu();
        Original_MainMenuCustomDraw(thisPtr, region);
    }

    void __fastcall Hooked_Present(void* thisPtr)
    {
        MainMenuOverlay::RenderBranding();
        Original_Present(thisPtr);
    }

    void WINAPI Hooked_OutputDebugStringA(const char* lpOutputString)
    {
        if (lpOutputString && lpOutputString[0] != '\0')
        {
            // Strip trailing newlines/carriage returns for clean log output
            size_t len = strlen(lpOutputString);
            while (len > 0 && (lpOutputString[len - 1] == '\n' || lpOutputString[len - 1] == '\r'))
                len--;

            if (len > 0)
                LogUtil::LogGameOutput(lpOutputString, len);
        }

        Original_OutputDebugStringA(lpOutputString);
    }
}
