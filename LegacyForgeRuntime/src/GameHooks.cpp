#include "GameHooks.h"
#include "DotNetHost.h"
#include "CreativeInventory.h"
#include "MainMenuOverlay.h"
#include "ModStrings.h"
#include "ModAtlas.h"
#include "LogUtil.h"
#include <Windows.h>
#include <string>
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
    GetString_fn          Original_GetString = nullptr;
    GetResourceAsStream_fn Original_GetResourceAsStream = nullptr;
    LoadUVs_fn             Original_LoadUVs = nullptr;
    RegisterIcon_fn        Original_RegisterIcon = nullptr;

    void __fastcall Hooked_LoadUVs(void* thisPtr)
    {
        LogUtil::Log("[LegacyForge] Hooked_LoadUVs: ENTER (textureMap=%p)", thisPtr);
        if (Original_LoadUVs)
            Original_LoadUVs(thisPtr);
        LogUtil::Log("[LegacyForge] Hooked_LoadUVs: original returned, creating mod icons");
        ModAtlas::CreateModIcons(thisPtr);
        LogUtil::Log("[LegacyForge] Hooked_LoadUVs: DONE");
    }

    static int s_registerIconCallCount = 0;

    void* __fastcall Hooked_RegisterIcon(void* thisPtr, const std::wstring& name)
    {
        s_registerIconCallCount++;
        void* modIcon = ModAtlas::LookupModIcon(name);
        if (modIcon)
        {
            LogUtil::Log("[LegacyForge] registerIcon #%d: '%ls' -> MOD ICON %p",
                         s_registerIconCallCount, name.c_str(), modIcon);
            return modIcon;
        }
        void* result = Original_RegisterIcon ? Original_RegisterIcon(thisPtr, name) : nullptr;
        if (s_registerIconCallCount <= 30 || !result)
        {
            LogUtil::Log("[LegacyForge] registerIcon #%d: '%ls' -> vanilla %p",
                         s_registerIconCallCount, name.c_str(), result);
        }
        return result;
    }

    void* Hooked_GetResourceAsStream(const void* fileName)
    {
        const std::wstring* path = static_cast<const std::wstring*>(fileName);
        if (ModAtlas::HasModTextures() && Original_GetResourceAsStream && path)
        {
            std::string terrainPath = ModAtlas::GetMergedTerrainPath();
            std::string itemsPath = ModAtlas::GetMergedItemsPath();
            if (!terrainPath.empty() && path->find(L"terrain.png") != std::wstring::npos)
            {
                std::wstring ourPath(terrainPath.begin(), terrainPath.end());
                LogUtil::Log("[LegacyForge] getResourceAsStream: redirecting terrain.png to merged atlas");
                return Original_GetResourceAsStream(&ourPath);
            }
            if (!itemsPath.empty() && path->find(L"items.png") != std::wstring::npos)
            {
                std::wstring ourPath(itemsPath.begin(), itemsPath.end());
                LogUtil::Log("[LegacyForge] getResourceAsStream: redirecting items.png to merged atlas");
                return Original_GetResourceAsStream(&ourPath);
            }
        }
        return Original_GetResourceAsStream ? Original_GetResourceAsStream(fileName) : nullptr;
    }

    const wchar_t* Hooked_GetString(int id)
    {
        if (ModStrings::IsModId(id))
        {
            const wchar_t* modStr = ModStrings::Get(id);
            if (modStr)
                return modStr;
        }
        return Original_GetString ? Original_GetString(id) : L"";
    }

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
        char baseDir[MAX_PATH] = { 0 };
        GetModuleFileNameA(nullptr, baseDir, MAX_PATH);
        std::string base(baseDir);
        size_t pos = base.find_last_of("\\/");
        if (pos != std::string::npos) base.resize(pos + 1);
        std::string gameResPath = base + "Common\\res\\TitleUpdate\\res";

        HMODULE hMod = nullptr;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&Hooked_MinecraftInit, &hMod) && hMod)
        {
            char dllPath[MAX_PATH] = { 0 };
            if (GetModuleFileNameA(hMod, dllPath, MAX_PATH))
            {
                std::string dllDir(dllPath);
                size_t dllPos = dllDir.find_last_of("\\/");
                if (dllPos != std::string::npos)
                {
                    dllDir.resize(dllPos + 1);
                    std::string modsPath = dllDir + "mods";
                    ModAtlas::BuildAtlases(modsPath, gameResPath);
                    goto atlas_done;
                }
            }
        }
        ModAtlas::BuildAtlases(base + "mods", gameResPath);
    atlas_done:

        // Overwrite the game's atlas PNGs with our merged versions BEFORE init
        // loads them. The originals are backed up and restored after init.
        ModAtlas::InstallAtlasFiles(gameResPath);

        Original_MinecraftInit(thisPtr);

        // After init, vanilla icons have their source-image pointer (field_0x48)
        // fully populated. Copy it to our mod icons so getSourceHeight() works.
        ModAtlas::FixupModIcons();

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
