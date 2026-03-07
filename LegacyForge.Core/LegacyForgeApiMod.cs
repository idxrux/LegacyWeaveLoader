using LegacyForge.API;

namespace LegacyForge.Core;

/// <summary>
/// Built-in mod representing the LegacyForge API. Counts in the mod list and appears
/// when mods/LegacyForge.API/ exists. Does not run lifecycle hooks.
/// </summary>
[Mod("legacyforge.api", Name = "LegacyForge API", Version = "1.0.0", Author = "LegacyForge",
     Description = "Mod API and shared types")]
internal sealed class LegacyForgeApiMod : IMod
{
    public void OnInitialize() { }
}
