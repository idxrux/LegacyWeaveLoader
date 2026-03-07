#include "NativeExports.h"
#include "IdRegistry.h"
#include "CreativeInventory.h"
#include "GameObjectFactory.h"
#include "LogUtil.h"
#include <Windows.h>
#include <cstring>
#include <string>

extern "C"
{

int native_register_block(
    const char* namespacedId,
    int materialId,
    float hardness,
    float resistance,
    int soundType,
    const char* iconName,
    float lightEmission,
    int lightBlock)
{
    if (!namespacedId) return -1;

    int id = IdRegistry::Instance().Register(IdRegistry::Type::Block, namespacedId);
    if (id < 0)
    {
        LogUtil::Log("[LegacyForge] Failed to allocate block ID for '%s'", namespacedId);
        return -1;
    }

    LogUtil::Log("[LegacyForge] Registered block '%s' -> ID %d (hardness=%.1f, resistance=%.1f)",
                 namespacedId, id, hardness, resistance);

    // Convert icon name from UTF-8 to wide string
    std::wstring wIcon;
    if (iconName)
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, iconName, -1, nullptr, 0);
        wIcon.resize(len > 0 ? len - 1 : 0);
        MultiByteToWideChar(CP_UTF8, 0, iconName, -1, &wIcon[0], len);
    }

    if (!GameObjectFactory::CreateTile(id, materialId, hardness, resistance,
                                        soundType, wIcon.empty() ? nullptr : wIcon.c_str()))
    {
        LogUtil::Log("[LegacyForge] Warning: failed to create game Tile for block '%s' id=%d", namespacedId, id);
    }

    return id;
}

int native_register_item(
    const char* namespacedId,
    int maxStackSize,
    int maxDamage,
    const char* iconName)
{
    if (!namespacedId) return -1;

    int id = IdRegistry::Instance().Register(IdRegistry::Type::Item, namespacedId);
    if (id < 0)
    {
        LogUtil::Log("[LegacyForge] Failed to allocate item ID for '%s'", namespacedId);
        return -1;
    }

    LogUtil::Log("[LegacyForge] Registered item '%s' -> ID %d (stack=%d, durability=%d)",
                 namespacedId, id, maxStackSize, maxDamage);

    std::wstring wIcon;
    if (iconName && iconName[0])
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, iconName, -1, nullptr, 0);
        wIcon.resize(len > 0 ? len - 1 : 0);
        MultiByteToWideChar(CP_UTF8, 0, iconName, -1, &wIcon[0], len);
    }

    if (!GameObjectFactory::CreateItem(id, maxStackSize, wIcon.empty() ? nullptr : wIcon.c_str()))
    {
        LogUtil::Log("[LegacyForge] Warning: failed to create game Item for '%s' id=%d", namespacedId, id);
    }

    return id;
}

int native_register_entity(
    const char* namespacedId,
    float width,
    float height,
    int trackingRange)
{
    if (!namespacedId) return -1;

    int id = IdRegistry::Instance().Register(IdRegistry::Type::Entity, namespacedId);
    if (id < 0)
    {
        LogUtil::Log("[LegacyForge] Failed to allocate entity ID for '%s'", namespacedId);
        return -1;
    }

    LogUtil::Log("[LegacyForge] Registered entity '%s' -> ID %d (%.1fx%.1f)",
                 namespacedId, id, width, height);

    return id;
}

void native_add_shaped_recipe(
    const char* resultId,
    int resultCount,
    const char* pattern,
    const char* ingredientIds)
{
    LogUtil::Log("[LegacyForge] Added shaped recipe: %dx %s", resultCount, resultId);
}

void native_add_furnace_recipe(
    const char* inputId,
    const char* outputId,
    float xp)
{
    LogUtil::Log("[LegacyForge] Added furnace recipe: %s -> %s (%.1f xp)", inputId, outputId, xp);
}

void native_log(const char* message, int level)
{
    if (message)
        LogUtil::Log("%s", message);
}

int native_get_block_id(const char* namespacedId)
{
    if (!namespacedId) return -1;
    return IdRegistry::Instance().GetNumericId(IdRegistry::Type::Block, namespacedId);
}

int native_get_item_id(const char* namespacedId)
{
    if (!namespacedId) return -1;
    return IdRegistry::Instance().GetNumericId(IdRegistry::Type::Item, namespacedId);
}

int native_get_entity_id(const char* namespacedId)
{
    if (!namespacedId) return -1;
    return IdRegistry::Instance().GetNumericId(IdRegistry::Type::Entity, namespacedId);
}

void native_subscribe_event(const char* eventName, void* managedFnPtr)
{
    LogUtil::Log("[LegacyForge] Event subscription: %s", eventName ? eventName : "(null)");
}

void native_add_to_creative(int numericId, int count, int auxValue, int groupIndex)
{
    CreativeInventory::AddPending(numericId, count, auxValue, groupIndex);
}

} // extern "C"
