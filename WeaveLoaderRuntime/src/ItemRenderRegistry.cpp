#include "ItemRenderRegistry.h"
#include "LogUtil.h"
#include <array>
#include <mutex>
#include <unordered_map>

namespace
{
    struct ItemRenderEntry
    {
        std::array<ItemDisplayTransformNative, ItemDisplay_ContextCount> transforms{};
        std::array<uint8_t, ItemDisplay_ContextCount> hasTransform{};
        ManagedItemRenderFn renderer = nullptr;
    };

    std::unordered_map<int, ItemRenderEntry> g_entries;
    std::mutex g_mutex;
}

void ItemRenderRegistry::RegisterDisplayTransform(int itemId, int context, const ItemDisplayTransformNative& transform)
{
    if (itemId < 0)
        return;
    if (context < 0 || context >= ItemDisplay_ContextCount)
        return;

    std::lock_guard<std::mutex> guard(g_mutex);
    ItemRenderEntry& entry = g_entries[itemId];
    entry.transforms[context] = transform;
    entry.hasTransform[context] = 1;
    LogUtil::Log("[WeaveLoader] ItemRenderRegistry: display transform %d registered for item %d", context, itemId);
}

bool ItemRenderRegistry::TryGetDisplayTransform(int itemId, int context, ItemDisplayTransformNative& outTransform)
{
    if (itemId < 0)
        return false;
    if (context < 0 || context >= ItemDisplay_ContextCount)
        return false;

    std::lock_guard<std::mutex> guard(g_mutex);
    auto it = g_entries.find(itemId);
    if (it == g_entries.end())
        return false;
    if (!it->second.hasTransform[context])
        return false;
    outTransform = it->second.transforms[context];
    return true;
}

void ItemRenderRegistry::RegisterCustomRenderer(int itemId, ManagedItemRenderFn fn)
{
    if (itemId < 0)
        return;
    std::lock_guard<std::mutex> guard(g_mutex);
    g_entries[itemId].renderer = fn;
    LogUtil::Log("[WeaveLoader] ItemRenderRegistry: custom renderer registered for item %d", itemId);
}

ManagedItemRenderFn ItemRenderRegistry::GetCustomRenderer(int itemId)
{
    if (itemId < 0)
        return nullptr;
    std::lock_guard<std::mutex> guard(g_mutex);
    auto it = g_entries.find(itemId);
    if (it == g_entries.end())
        return nullptr;
    return it->second.renderer;
}
