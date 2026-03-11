#include "CreativeInventory.h"
#include "SymbolResolver.h"
#include "PdbParser.h"
#include "LogUtil.h"
#include <cstring>
#include <algorithm>

namespace CreativeInventory
{

void* pCategoryGroups = nullptr;
void* pItemInstanceCtor = nullptr;
void* pSharedPtrCtor = nullptr;
void* pVectorPushBack = nullptr;
void* pSpecs = nullptr;

std::vector<PendingCreativeItem> s_pendingItems;
static bool s_didInjectItems = false;
static bool s_creativeReady = false;

static const int CREATIVE_GROUP_COUNT = 15;
static const int SIZEOF_MSVC_VECTOR = 24;
static const int SIZEOF_MSVC_SHARED_PTR = 16;
static const int ITEMINSTANCE_ALLOC_SIZE = 256;

// TabSpec field offsets (MSVC x64 layout, derived from IUIScene_CreativeMenu.h)
// LPCWSTR m_icon              @ 0  (8)
// int m_descriptionId         @ 8  (4)
// int m_staticGroupsCount     @ 12 (4)
// ECreative_Inventory_Groups* @ 16 (8)
// int m_dynamicGroupsCount    @ 24 (4) + 4 pad
// ECreative_Inventory_Groups* @ 32 (8)
// int m_debugGroupsCount      @ 40 (4) + 4 pad
// ECreative_Inventory_Groups* @ 48 (8)
// uint m_pages                @ 56 (4)
// uint m_staticPerPage        @ 60 (4)
// uint m_staticItems          @ 64 (4)
// uint m_debugItems           @ 68 (4)
static const int TABSPEC_STATIC_GROUPS_COUNT_OFF = 12;
static const int TABSPEC_STATIC_GROUPS_A_OFF     = 16;
static const int TABSPEC_PAGES_OFF               = 56;
static const int TABSPEC_STATIC_ITEMS_OFF        = 64;
static const int TAB_COUNT = 8;
static const int COLUMNS = 10;
static const int ROWS = 5;

typedef void (__fastcall *ItemInstanceCtor_fn)(void* thisPtr, int id, int count, int auxValue);
typedef void (__fastcall *SharedPtrCtor_fn)(void* sharedPtrThis, void* rawItemPtr);
typedef void (__fastcall *VectorPushBackMove_fn)(void* vectorThis, void* sharedPtrRvalueRef);

void AddPending(int itemId, int count, int auxValue, int groupIndex)
{
    AddPendingEx(itemId, count, auxValue, groupIndex, InsertAppend, -1, -1);
}

void AddPendingEx(int itemId, int count, int auxValue, int groupIndex, int insertMode, int anchorId, int anchorAux)
{
    (void)anchorId;
    (void)anchorAux;
    s_pendingItems.push_back({ itemId, count, auxValue, groupIndex, insertMode });
    LogUtil::Log("[WeaveLoader] Queued creative item: id=%d group=%d mode=%d",
                 itemId, groupIndex, insertMode);

    if (s_creativeReady)
    {
        InjectItems();
        UpdateTabPageCounts();
    }
}

void SetCreativeReady()
{
    s_creativeReady = true;
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

    pSpecs = resolver.Resolve(
        "?specs@IUIScene_CreativeMenu@@1PEAPEAUTabSpec@1@EA");

    if (pCategoryGroups)  LogUtil::Log("[WeaveLoader] categoryGroups       @ %p", pCategoryGroups);
    else                  LogUtil::Log("[WeaveLoader] MISSING: categoryGroups");

    if (pItemInstanceCtor) LogUtil::Log("[WeaveLoader] ItemInstance ctor     @ %p", pItemInstanceCtor);
    else                   LogUtil::Log("[WeaveLoader] MISSING: ItemInstance(int,int,int)");

    if (pSharedPtrCtor)   LogUtil::Log("[WeaveLoader] shared_ptr<II> ctor   @ %p", pSharedPtrCtor);
    else                  LogUtil::Log("[WeaveLoader] MISSING: shared_ptr<ItemInstance>(ItemInstance*)");

    if (pVectorPushBack)  LogUtil::Log("[WeaveLoader] vector::push_back     @ %p", pVectorPushBack);
    else                  LogUtil::Log("[WeaveLoader] MISSING: vector<shared_ptr<II>>::push_back");

    if (pSpecs)           LogUtil::Log("[WeaveLoader] specs                 @ %p", pSpecs);
    else
    {
        LogUtil::Log("[WeaveLoader] MISSING: specs (page counts won't be updated)");
        PdbParser::DumpMatching("specs@IUIScene_CreativeMenu");
    }

    return pCategoryGroups && pItemInstanceCtor && pSharedPtrCtor;
}

static size_t ReadVectorSize(char* vec)
{
    char* first = *reinterpret_cast<char**>(vec);
    char* last  = *reinterpret_cast<char**>(vec + 8);
    if (!first || last <= first) return 0;
    return static_cast<size_t>((last - first) / SIZEOF_MSVC_SHARED_PTR);
}

static char* VectorBegin(char* vec) { return *reinterpret_cast<char**>(vec); }
static char* VectorEnd(char* vec) { return *reinterpret_cast<char**>(vec + 8); }
static char* VectorCap(char* vec) { return *reinterpret_cast<char**>(vec + 16); }

static void SetVectorPointers(char* vec, char* begin, char* end, char* cap)
{
    *reinterpret_cast<char**>(vec) = begin;
    *reinterpret_cast<char**>(vec + 8) = end;
    *reinterpret_cast<char**>(vec + 16) = cap;
}

static bool EnsureVectorCapacity(char* vec, size_t desiredCount)
{
    char* begin = VectorBegin(vec);
    char* end = VectorEnd(vec);
    char* cap = VectorCap(vec);
    size_t size = begin ? static_cast<size_t>((end - begin) / SIZEOF_MSVC_SHARED_PTR) : 0;
    size_t capacity = begin ? static_cast<size_t>((cap - begin) / SIZEOF_MSVC_SHARED_PTR) : 0;

    if (capacity >= desiredCount)
        return true;

    size_t newCap = capacity ? capacity * 2 : 4;
    if (newCap < desiredCount)
        newCap = desiredCount;

    char* newBuf = reinterpret_cast<char*>(::operator new(newCap * SIZEOF_MSVC_SHARED_PTR));
    if (!newBuf)
        return false;

    if (begin && size > 0)
        memcpy(newBuf, begin, size * SIZEOF_MSVC_SHARED_PTR);

    if (begin)
        ::operator delete(begin);

    SetVectorPointers(vec, newBuf, newBuf + size * SIZEOF_MSVC_SHARED_PTR, newBuf + newCap * SIZEOF_MSVC_SHARED_PTR);
    return true;
}

static bool InsertSharedPtrAt(char* vec, const void* sharedPtrBuf, size_t index)
{
    size_t size = ReadVectorSize(vec);
    if (index > size) index = size;

    if (!EnsureVectorCapacity(vec, size + 1))
        return false;

    char* begin = VectorBegin(vec);
    char* end = VectorEnd(vec);
    char* insertPos = begin + index * SIZEOF_MSVC_SHARED_PTR;
    size_t tailBytes = static_cast<size_t>(end - insertPos);
    memmove(insertPos + SIZEOF_MSVC_SHARED_PTR, insertPos, tailBytes);
    memcpy(insertPos, sharedPtrBuf, SIZEOF_MSVC_SHARED_PTR);
    SetVectorPointers(vec, begin, end + SIZEOF_MSVC_SHARED_PTR, VectorCap(vec));
    return true;
}

static bool InsertItemInstance(char* vec, const PendingCreativeItem& item, size_t insertIndex,
                               ItemInstanceCtor_fn ctorFn, SharedPtrCtor_fn spCtorFn)
{
    size_t sizeBefore = ReadVectorSize(vec);

    void* rawItem = ::operator new(ITEMINSTANCE_ALLOC_SIZE);
    memset(rawItem, 0, ITEMINSTANCE_ALLOC_SIZE);
    ctorFn(rawItem, item.itemId, item.count, item.auxValue);

    // Verify ItemInstance vtable was set (first 8 bytes should be non-null)
    void* vtable = *reinterpret_cast<void**>(rawItem);
    LogUtil::Log("[WeaveLoader] ItemInstance(%d,%d,%d) @ %p, vtable=%p",
                 item.itemId, item.count, item.auxValue, rawItem, vtable);

    char spBuf[16];
    memset(spBuf, 0, sizeof(spBuf));
    spCtorFn(spBuf, rawItem);

    // Log shared_ptr contents (ptr + control block)
    void* spPtr = *reinterpret_cast<void**>(spBuf);
    void* spCtrl = *reinterpret_cast<void**>(spBuf + 8);
    LogUtil::Log("[WeaveLoader] shared_ptr: ptr=%p ctrl=%p", spPtr, spCtrl);

    bool inserted = InsertSharedPtrAt(vec, spBuf, insertIndex);
    size_t sizeAfter = ReadVectorSize(vec);
    if (!inserted)
    {
        LogUtil::Log("[WeaveLoader] Failed to insert creative item id=%d into group %d",
                     item.itemId, item.groupIndex);
        return false;
    }

    LogUtil::Log("[WeaveLoader] Injected item id=%d into creative group %d "
                 "(vector @ %p, size: %zu -> %zu, index=%zu)",
                 item.itemId, item.groupIndex, vec, sizeBefore, sizeAfter, insertIndex);
    return true;
}

void UpdateTabPageCounts()
{
    if (!s_didInjectItems)
    {
        LogUtil::Log("[WeaveLoader] Skipping tab page count update (no mod items injected)");
        return;
    }

    if (!pSpecs || !pCategoryGroups)
    {
        LogUtil::Log("[WeaveLoader] Cannot update tab page counts: specs=%p categoryGroups=%p",
                     pSpecs, pCategoryGroups);
        return;
    }

    void** specsArray = *reinterpret_cast<void***>(pSpecs);
    if (!specsArray)
    {
        LogUtil::Log("[WeaveLoader] specs pointer is null, TabSpec array not yet allocated");
        return;
    }

    char* groups = reinterpret_cast<char*>(pCategoryGroups);

    for (int tabIdx = 0; tabIdx < TAB_COUNT; ++tabIdx)
    {
        char* tab = reinterpret_cast<char*>(specsArray[tabIdx]);
        if (!tab) continue;

        int staticGroupsCount = *reinterpret_cast<int*>(tab + TABSPEC_STATIC_GROUPS_COUNT_OFF);
        int* staticGroupsA = *reinterpret_cast<int**>(tab + TABSPEC_STATIC_GROUPS_A_OFF);
        if (!staticGroupsA || staticGroupsCount <= 0) continue;

        unsigned int totalItems = 0;
        for (int i = 0; i < staticGroupsCount; ++i)
        {
            int groupIdx = staticGroupsA[i];
            if (groupIdx < 0 || groupIdx >= CREATIVE_GROUP_COUNT) continue;
            totalItems += static_cast<unsigned int>(
                ReadVectorSize(groups + groupIdx * SIZEOF_MSVC_VECTOR));
        }

        unsigned int oldItems = *reinterpret_cast<unsigned int*>(tab + TABSPEC_STATIC_ITEMS_OFF);
        unsigned int oldPages = *reinterpret_cast<unsigned int*>(tab + TABSPEC_PAGES_OFF);

        *reinterpret_cast<unsigned int*>(tab + TABSPEC_STATIC_ITEMS_OFF) = totalItems;

        int totalRows = (totalItems + COLUMNS - 1) / COLUMNS;
        int newPages = totalRows - ROWS + 1;
        if (newPages < 1) newPages = 1;
        *reinterpret_cast<unsigned int*>(tab + TABSPEC_PAGES_OFF) = static_cast<unsigned int>(newPages);

        if (totalItems != oldItems)
        {
            LogUtil::Log("[WeaveLoader] Tab %d: staticItems %u -> %u, pages %u -> %u",
                         tabIdx, oldItems, totalItems, oldPages, static_cast<unsigned int>(newPages));
        }
    }
}

void InjectItems()
{
    if (!pCategoryGroups || !pItemInstanceCtor || !pSharedPtrCtor)
    {
        LogUtil::Log("[WeaveLoader] Cannot inject creative items: missing symbols");
        return;
    }

    if (s_pendingItems.empty())
    {
        if (s_didInjectItems)
            LogUtil::Log("[WeaveLoader] No creative items to inject (pending empty; already injected)");
        else
            LogUtil::Log("[WeaveLoader] No creative items to inject (none queued)");
        return;
    }

    s_didInjectItems = true;
    auto ctorFn = reinterpret_cast<ItemInstanceCtor_fn>(pItemInstanceCtor);
    auto spCtorFn = reinterpret_cast<SharedPtrCtor_fn>(pSharedPtrCtor);
    // categoryGroups is a static array of vectors (vector<...> categoryGroups[15]).
    // pCategoryGroups is the address of the first vector; no dereference needed.
    char* groups = reinterpret_cast<char*>(pCategoryGroups);

    std::vector<PendingCreativeItem> grouped[CREATIVE_GROUP_COUNT];
    for (auto& item : s_pendingItems)
    {
        if (item.groupIndex < 0 || item.groupIndex >= CREATIVE_GROUP_COUNT)
        {
            LogUtil::Log("[WeaveLoader] Skipping creative item id=%d: invalid group %d",
                         item.itemId, item.groupIndex);
            continue;
        }
        grouped[item.groupIndex].push_back(item);
    }

    size_t injectedCount = 0;
    for (int groupIndex = 0; groupIndex < CREATIVE_GROUP_COUNT; ++groupIndex)
    {
        auto& pending = grouped[groupIndex];
        if (pending.empty())
            continue;

        char* vec = groups + groupIndex * SIZEOF_MSVC_VECTOR;
        size_t prependIndex = 0;
        for (auto& item : pending)
        {
            size_t insertIndex = ReadVectorSize(vec);
            if (item.insertMode == InsertPrepend)
            {
                insertIndex = prependIndex;
                ++prependIndex;
            }

            if (InsertItemInstance(vec, item, insertIndex, ctorFn, spCtorFn))
                ++injectedCount;
            else
                LogUtil::Log("[WeaveLoader] Failed to insert creative item id=%d in group %d",
                             item.itemId, groupIndex);
        }
    }

    LogUtil::Log("[WeaveLoader] Injected %zu items into creative inventory", injectedCount);
    s_pendingItems.clear();
}

} // namespace CreativeInventory
