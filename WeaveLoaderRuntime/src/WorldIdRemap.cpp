#include "WorldIdRemap.h"

#include "GameObjectFactory.h"
#include "IdRegistry.h"
#include "LogUtil.h"
#include "ModStrings.h"
#include "PdbParser.h"

#include <Windows.h>
#include <cstddef>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>

namespace
{
    constexpr wchar_t kNamespaceTagKey[] = L"weaveloader:id";
    constexpr char kMissingBlockId[] = "weaveloader:missing_block";
    constexpr char kMissingItemId[] = "weaveloader:missing_item";
    constexpr int kMissingBlockNumericId = 255;
    constexpr int kMissingItemNumericId = 31999;
    constexpr int kMissingBlockDescriptionId = 900000;
    constexpr int kMissingItemDescriptionId = 900001;
    constexpr ptrdiff_t kItemIdOffset = 0x20;
    constexpr ptrdiff_t kItemTagOffset = 0x28;
    constexpr unsigned char kTagStringId = 8;
    constexpr unsigned char kTagCompoundId = 10;

    using TagNewTag_fn = void* (__fastcall *)(unsigned char type, const std::wstring& name);

    struct CompoundTagLayout
    {
        void* vtable;
        std::wstring name;
        std::unordered_map<std::wstring, void*> tags;
    };

    struct StringTagLayout
    {
        void* vtable;
        std::wstring name;
        std::wstring data;
    };

    std::once_flag s_symbolInitOnce;
    TagNewTag_fn s_tagNewTag = nullptr;
    bool s_missingPlaceholdersReady = false;

    static bool IsVanillaId(const std::string& namespacedId)
    {
        return namespacedId.rfind("minecraft:", 0) == 0;
    }

    static std::string ResolveNamespacedItemId(int numericId)
    {
        std::string namespacedId = IdRegistry::Instance().GetStringId(IdRegistry::Type::Item, numericId);
        if (!namespacedId.empty())
            return namespacedId;

        // Block-backed inventory entries are TileItems whose stored item ID matches the block ID.
        return IdRegistry::Instance().GetStringId(IdRegistry::Type::Block, numericId);
    }

    static int ResolveNumericItemId(const std::string& namespacedId, int oldNumericId)
    {
        if (namespacedId.empty())
            return -1;

        const bool wasKnownAsBlock = !IdRegistry::Instance().GetStringId(IdRegistry::Type::Block, oldNumericId).empty();
        const bool wasKnownAsItem = !IdRegistry::Instance().GetStringId(IdRegistry::Type::Item, oldNumericId).empty();

        if (wasKnownAsBlock && !wasKnownAsItem)
        {
            int blockId = IdRegistry::Instance().GetNumericId(IdRegistry::Type::Block, namespacedId);
            if (blockId >= 0)
                return blockId;
        }

        int itemId = IdRegistry::Instance().GetNumericId(IdRegistry::Type::Item, namespacedId);
        if (itemId >= 0)
            return itemId;

        int blockId = IdRegistry::Instance().GetNumericId(IdRegistry::Type::Block, namespacedId);
        if (blockId >= 0)
            return blockId;

        if (wasKnownAsBlock)
            return IdRegistry::Instance().GetMissingFallback(IdRegistry::Type::Block);

        return IdRegistry::Instance().GetMissingFallback(IdRegistry::Type::Item);
    }

    static bool IsReadableRange(const void* ptr, size_t bytes);

    static CompoundTagLayout* GetItemTag(void* itemInstancePtr)
    {
        if (!itemInstancePtr || !IsReadableRange(static_cast<const char*>(itemInstancePtr) + kItemTagOffset, sizeof(void*)))
            return nullptr;
        return *reinterpret_cast<CompoundTagLayout**>(static_cast<char*>(itemInstancePtr) + kItemTagOffset);
    }

    static CompoundTagLayout* EnsureItemTag(void* itemInstancePtr)
    {
        CompoundTagLayout* tag = GetItemTag(itemInstancePtr);
        if (tag || !s_tagNewTag)
            return tag;

        void* createdTag = s_tagNewTag(kTagCompoundId, L"");
        if (!createdTag)
            return nullptr;

        *reinterpret_cast<void**>(static_cast<char*>(itemInstancePtr) + kItemTagOffset) = createdTag;
        return reinterpret_cast<CompoundTagLayout*>(createdTag);
    }

    static bool TryGetCompoundString(CompoundTagLayout* compoundTag, const std::wstring& key, std::wstring* outValue)
    {
        if (!compoundTag || !outValue)
            return false;

        const auto it = compoundTag->tags.find(key);
        if (it == compoundTag->tags.end() || !it->second)
            return false;

        const auto* stringTag = reinterpret_cast<const StringTagLayout*>(it->second);
        *outValue = stringTag->data;
        return true;
    }

    static bool PutCompoundString(CompoundTagLayout* compoundTag, const std::wstring& key, const std::wstring& value)
    {
        if (!compoundTag)
            return false;

        const auto it = compoundTag->tags.find(key);
        if (it != compoundTag->tags.end() && it->second)
        {
            auto* stringTag = reinterpret_cast<StringTagLayout*>(it->second);
            stringTag->data = value;
            return true;
        }

        if (!s_tagNewTag)
            return false;

        void* createdTag = s_tagNewTag(kTagStringId, key);
        if (!createdTag)
            return false;

        auto* stringTag = reinterpret_cast<StringTagLayout*>(createdTag);
        stringTag->data = value;
        compoundTag->tags[key] = createdTag;
        return true;
    }

    static bool IsReadableRange(const void* ptr, size_t bytes)
    {
        if (!ptr || bytes == 0)
            return false;

        MEMORY_BASIC_INFORMATION mbi{};
        if (VirtualQuery(ptr, &mbi, sizeof(mbi)) != sizeof(mbi))
            return false;
        if (mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS || (mbi.Protect & PAGE_GUARD))
            return false;

        const DWORD readMask = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
                               PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
        if ((mbi.Protect & readMask) == 0)
            return false;

        const uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
        const uintptr_t end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        return p + bytes >= p && p + bytes <= end;
    }

    static void ResolveSymbolsOnce()
    {
        std::call_once(s_symbolInitOnce, []() {
            if (s_tagNewTag)
                return;

            const uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
            uint32_t newTagRvaByName = PdbParser::FindSymbolRVAByName("Tag::newTag");
            uint32_t newTagRvaByDecorated = PdbParser::FindSymbolRVA(
                "?newTag@Tag@@SAPEAV1@EAEBV?$basic_string@_WU?$char_traits@_W@std@@V?$allocator@_W@2@@std@@@Z");
            uint32_t newTagRva = newTagRvaByName;
            if (newTagRva == 0)
                newTagRva = newTagRvaByDecorated;
            if (newTagRva)
                s_tagNewTag = reinterpret_cast<TagNewTag_fn>(base + newTagRva);
        });
    }
}

namespace WorldIdRemap
{
    void SetTagNewTagSymbol(void* fnPtr)
    {
        s_tagNewTag = reinterpret_cast<TagNewTag_fn>(fnPtr);
    }

    void EnsureMissingPlaceholders()
    {
        if (s_missingPlaceholdersReady)
            return;

        IdRegistry::Instance().RegisterVanilla(IdRegistry::Type::Block, kMissingBlockNumericId, kMissingBlockId);
        IdRegistry::Instance().RegisterVanilla(IdRegistry::Type::Item, kMissingItemNumericId, kMissingItemId);

        ModStrings::Register(kMissingBlockDescriptionId, L"Missing Block");
        GameObjectFactory::CreateTile(
            kMissingBlockNumericId,
            1,
            1.0f,
            1.0f,
            1,
            L"bedrock",
            0.0f,
            15,
            kMissingBlockDescriptionId);
        IdRegistry::Instance().SetMissingFallback(IdRegistry::Type::Block, kMissingBlockNumericId);

        ModStrings::Register(kMissingItemDescriptionId, L"Missing Item");
        GameObjectFactory::CreateItem(
            kMissingItemNumericId,
            64,
            0,
            L"apple",
            kMissingItemDescriptionId);
        IdRegistry::Instance().SetMissingFallback(IdRegistry::Type::Item, kMissingItemNumericId);

        s_missingPlaceholdersReady = true;
        LogUtil::Log("[WeaveLoader] Missing content placeholders ready: block=%d item=%d",
                     IdRegistry::Instance().GetMissingFallback(IdRegistry::Type::Block),
                     IdRegistry::Instance().GetMissingFallback(IdRegistry::Type::Item));
    }

    void TagModdedItemInstance(void* itemInstancePtr, void* compoundTagPtr)
    {
        ResolveSymbolsOnce();
        if (!itemInstancePtr)
            return;
        if (!IsReadableRange(static_cast<const char*>(itemInstancePtr) + kItemIdOffset, sizeof(int)))
            return;
        (void)compoundTagPtr;

        const int itemId = *reinterpret_cast<const int*>(static_cast<const char*>(itemInstancePtr) + kItemIdOffset);
        const std::string namespacedId = ResolveNamespacedItemId(itemId);
        if (namespacedId.empty() || IsVanillaId(namespacedId))
            return;

        CompoundTagLayout* itemTag = EnsureItemTag(itemInstancePtr);
        if (!itemTag)
            return;

        // Never let placeholder IDs overwrite a previously saved real namespace.
        if (namespacedId == kMissingItemId || namespacedId == kMissingBlockId)
        {
            std::wstring existingNamespace;
            if (TryGetCompoundString(itemTag, kNamespaceTagKey, &existingNamespace) && !existingNamespace.empty())
                return;
        }

        const std::wstring namespacedWide(namespacedId.begin(), namespacedId.end());
        if (!PutCompoundString(itemTag, kNamespaceTagKey, namespacedWide))
            return;
    }

    void RemapItemInstanceFromTag(void* itemInstancePtr, void* compoundTagPtr)
    {
        ResolveSymbolsOnce();
        if (!itemInstancePtr)
            return;
        (void)compoundTagPtr;
        if (!IsReadableRange(static_cast<const char*>(itemInstancePtr) + kItemIdOffset, sizeof(int)))
            return;

        const int oldItemId = *reinterpret_cast<const int*>(static_cast<const char*>(itemInstancePtr) + kItemIdOffset);
        CompoundTagLayout* itemTag = GetItemTag(itemInstancePtr);
        if (!itemTag)
            return;

        std::wstring namespacedWide;
        if (!TryGetCompoundString(itemTag, kNamespaceTagKey, &namespacedWide) || namespacedWide.empty())
            return;

        const std::string namespacedId(namespacedWide.begin(), namespacedWide.end());
        int numericId = ResolveNumericItemId(namespacedId, oldItemId);
        if (numericId >= 0)
            *reinterpret_cast<int*>(static_cast<char*>(itemInstancePtr) + kItemIdOffset) = numericId;
    }
}
