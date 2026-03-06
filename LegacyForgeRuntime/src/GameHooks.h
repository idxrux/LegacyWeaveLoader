#pragma once
#include <cstdint>

/// Function pointer typedefs matching the game's actual function signatures.
/// x64 MSVC uses __fastcall-like convention (this in rcx, args in rdx/r8/r9).
///
/// Verified against Minecraft.Client.pdb:
///   ?MinecraftWorld_RunStaticCtors@@YAXXZ          -- void __cdecl ()
///   ?tick@Minecraft@@QEAAX_N0@Z                   -- void __thiscall (bool, bool)
///   ?init@Minecraft@@QEAAXXZ                      -- void __thiscall ()
///   ?ExitGame@CConsoleMinecraftApp@@UEAAXXZ        -- void __thiscall ()

typedef void (*RunStaticCtors_fn)();
typedef void (__fastcall *MinecraftTick_fn)(void* thisPtr, bool bFirst, bool bUpdateTextures);
typedef void (__fastcall *MinecraftInit_fn)(void* thisPtr);
typedef void (__fastcall *ExitGame_fn)(void* thisPtr);

namespace GameHooks
{
    extern RunStaticCtors_fn Original_RunStaticCtors;
    extern MinecraftTick_fn  Original_MinecraftTick;
    extern MinecraftInit_fn  Original_MinecraftInit;
    extern ExitGame_fn       Original_ExitGame;

    void Hooked_RunStaticCtors();
    void __fastcall Hooked_MinecraftTick(void* thisPtr, bool bFirst, bool bUpdateTextures);
    void __fastcall Hooked_MinecraftInit(void* thisPtr);
    void __fastcall Hooked_ExitGame(void* thisPtr);
}
