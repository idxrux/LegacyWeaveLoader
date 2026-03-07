#include "CreativeInventory.h"
#include "SymbolResolver.h"
#include <cstdio>
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
    printf("[LegacyForge] Queued creative item: id=%d group=%d\n", itemId, groupIndex);
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

    if (pCategoryGroups)  printf("[LegacyForge] categoryGroups       @ %p\n", pCategoryGroups);
    else                  printf("[LegacyForge] MISSING: categoryGroups\n");

    if (pItemInstanceCtor) printf("[LegacyForge] ItemInstance ctor     @ %p\n", pItemInstanceCtor);
    else                   printf("[LegacyForge] MISSING: ItemInstance(int,int,int)\n");

    if (pSharedPtrCtor)   printf("[LegacyForge] shared_ptr<II> ctor   @ %p\n", pSharedPtrCtor);
    else                  printf("[LegacyForge] MISSING: shared_ptr<ItemInstance>(ItemInstance*)\n");

    if (pVectorPushBack)  printf("[LegacyForge] vector::push_back     @ %p\n", pVectorPushBack);
    else                  printf("[LegacyForge] MISSING: vector<shared_ptr<II>>::push_back\n");

    return pCategoryGroups && pItemInstanceCtor && pSharedPtrCtor && pVectorPushBack;
}

void InjectItems()
{
    if (!pCategoryGroups || !pItemInstanceCtor || !pSharedPtrCtor || !pVectorPushBack)
    {
        printf("[LegacyForge] Cannot inject creative items: missing symbols\n");
        return;
    }

    if (s_pendingItems.empty())
    {
        printf("[LegacyForge] No creative items to inject\n");
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
            printf("[LegacyForge] Skipping creative item id=%d: invalid group %d\n",
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

        printf("[LegacyForge] Injected item id=%d into creative group %d\n",
               item.itemId, item.groupIndex);
    }

    printf("[LegacyForge] Injected %zu items into creative inventory\n", s_pendingItems.size());
    s_pendingItems.clear();
}

} // namespace CreativeInventory
