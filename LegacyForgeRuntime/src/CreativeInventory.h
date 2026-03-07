#pragma once
#include <vector>

class SymbolResolver;

struct PendingCreativeItem
{
    int itemId;
    int count;
    int auxValue;
    int groupIndex;
};

namespace CreativeInventory
{
    void AddPending(int itemId, int count, int auxValue, int groupIndex);
    bool ResolveSymbols(SymbolResolver& resolver);
    void InjectItems();

    extern void* pCategoryGroups;
    extern void* pItemInstanceCtor;
    extern void* pSharedPtrCtor;
    extern void* pVectorPushBack;

    extern std::vector<PendingCreativeItem> s_pendingItems;
}
