#pragma once

#include <unordered_map>
#include <string>
#include <mutex>

/// <summary>
/// Stores mod-registered display names for blocks and items.
/// Maps description IDs (allocated from MOD_DESC_ID_BASE) to wide strings.
/// Hooked into app.GetString() so the game displays mod names.
/// </summary>
namespace ModStrings
{
    constexpr int MOD_DESC_ID_BASE = 10000;

    void Register(int descriptionId, const wchar_t* value);
    const wchar_t* Get(int descriptionId);
    int AllocateId();
    bool IsModId(int id);
}
