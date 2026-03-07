#include "CreativeInventory.h"
#include "SymbolResolver.h"
#include "LogUtil.h"
#include <cstring>

namespace CreativeInventory
{

void* pCategoryGroups = nullptr;
void* pItemInstanceCtor = nullptr;
void* pSharedPtrCtor = nullptr;
void* pVectorPushBack = nullptr;

std::vector<PendingCreativeItem> s_pendingItems;

static const int CREATIVE_GROUP_COUNT = 15;
static const int SIZEOF_MSVC_VECTOR = 24;
static const int SIZEOF_MSVC_SHARED_PTR = 16;
static const int ITEMINSTANCE_ALLOC_SIZE = 256;

typedef void (__fastcall *ItemInstanceCtor_fn)(void* thisPtr, int id, int count, int auxValue);
typedef void (__fastcall *SharedPtrCtor_fn)(void* sharedPtrThis, void* rawItemPtr);
typedef void (__fastcall *VectorPushBackMove_fn)(void* vectorThis, void* sharedPtrRvalueRef);

void AddPending(int itemId, int count, int auxValue, int groupIndex)
{
    s_pendingItems.push_back({ itemId, count, auxValue, groupIndex });
    LogUtil::Log("[LegacyForge] Queued creative item: id=%d group=%d", itemId, groupIndex);
}

bool ResolveSymbols(SymbolResolver& resolver)
{
    pCategoryGroups = resolver.Resolve(
        "?categoryGroups@IUIScene_CreativeMenu@@1PAV?$vector@V?$shared_ptr@VItemInstance@@@std@@"
        "V?$allocator@V?$shared_ptr@VItemInstance@@@std@@@2@@std@@A");

    pItemInstanceCtor = resolver.Resolve("??0ItemInstance@@QEAA@HHH@Z");

    pSharedPtrCtor = resolver.Resolve(
        "??$?0VItemInstance@@$0A@@?$shared_ptr@VItemInstance@@@std@@QEAA@PEAVItemInstance@@@Z");

    pVectorPushBack = resolver.Resolve(
        "?push_back@?$vector@V?$shared_ptr@VItemInstance@@@std@@V?$allocator@V?$shared_ptr@VItemInstance@@@std@@@2@@std@@"
        "QEAAX$$QEAV?$shared_ptr@VItemInstance@@@2@@Z");

    if (pCategoryGroups)  LogUtil::Log("[LegacyForge] categoryGroups       @ %p", pCategoryGroups);
    else                  LogUtil::Log("[LegacyForge] MISSING: categoryGroups");

    if (pItemInstanceCtor) LogUtil::Log("[LegacyForge] ItemInstance ctor     @ %p", pItemInstanceCtor);
    else                   LogUtil::Log("[LegacyForge] MISSING: ItemInstance(int,int,int)");

    if (pSharedPtrCtor)   LogUtil::Log("[LegacyForge] shared_ptr<II> ctor   @ %p", pSharedPtrCtor);
    else                  LogUtil::Log("[LegacyForge] MISSING: shared_ptr<ItemInstance>(ItemInstance*)");

    if (pVectorPushBack)  LogUtil::Log("[LegacyForge] vector::push_back     @ %p", pVectorPushBack);
    else                  LogUtil::Log("[LegacyForge] MISSING: vector<shared_ptr<II>>::push_back");

    return pCategoryGroups && pItemInstanceCtor && pSharedPtrCtor && pVectorPushBack;
}

void InjectItems()
{
    if (!pCategoryGroups || !pItemInstanceCtor || !pSharedPtrCtor || !pVectorPushBack)
    {
        LogUtil::Log("[LegacyForge] Cannot inject creative items: missing symbols");
        return;
    }

    if (s_pendingItems.empty())
    {
        LogUtil::Log("[LegacyForge] No creative items to inject");
        return;
    }

    auto ctorFn = reinterpret_cast<ItemInstanceCtor_fn>(pItemInstanceCtor);
    auto spCtorFn = reinterpret_cast<SharedPtrCtor_fn>(pSharedPtrCtor);
    auto pushFn = reinterpret_cast<VectorPushBackMove_fn>(pVectorPushBack);

    char* groups = reinterpret_cast<char*>(pCategoryGroups);

    for (auto& item : s_pendingItems)
    {
        if (item.groupIndex < 0 || item.groupIndex >= CREATIVE_GROUP_COUNT)
        {
            LogUtil::Log("[LegacyForge] Skipping creative item id=%d: invalid group %d",
                         item.itemId, item.groupIndex);
            continue;
        }

        void* rawItem = ::operator new(ITEMINSTANCE_ALLOC_SIZE);
        memset(rawItem, 0, ITEMINSTANCE_ALLOC_SIZE);
        ctorFn(rawItem, item.itemId, item.count, item.auxValue);

        char spBuf[16];
        memset(spBuf, 0, sizeof(spBuf));
        spCtorFn(spBuf, rawItem);

        char* vec = groups + item.groupIndex * SIZEOF_MSVC_VECTOR;
        pushFn(vec, spBuf);

        LogUtil::Log("[LegacyForge] Injected item id=%d into creative group %d",
                     item.itemId, item.groupIndex);
    }

    LogUtil::Log("[LegacyForge] Injected %zu items into creative inventory", s_pendingItems.size());
    s_pendingItems.clear();
}

} // namespace CreativeInventory
