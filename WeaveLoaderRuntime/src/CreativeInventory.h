#pragma once
#include <vector>

class SymbolResolver;

struct PendingCreativeItem
{
    int itemId;
    int count;
    int auxValue;
    int groupIndex;
    int insertMode;
};

namespace CreativeInventory
{
    enum InsertMode
    {
        InsertAppend = 0,
        InsertPrepend = 1,
        InsertBefore = 2,
        InsertAfter = 3
    };

    void AddPending(int itemId, int count, int auxValue, int groupIndex);
    void AddPendingEx(int itemId, int count, int auxValue, int groupIndex, int insertMode, int anchorId, int anchorAux);
    void SetCreativeReady();
    bool ResolveSymbols(SymbolResolver& resolver);
    void InjectItems();
    void UpdateTabPageCounts();

    extern void* pCategoryGroups;
    extern void* pItemInstanceCtor;
    extern void* pSharedPtrCtor;
    extern void* pVectorPushBack;
    extern void* pSpecs;

    extern std::vector<PendingCreativeItem> s_pendingItems;
}
