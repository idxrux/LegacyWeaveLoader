#include "ModStrings.h"
#include "LogUtil.h"
#include <vector>
#include <cstring>

namespace ModStrings
{
    static std::mutex s_mutex;
    static std::unordered_map<int, std::wstring> s_strings;
    static int s_nextId = MOD_DESC_ID_BASE;

    void Register(int descriptionId, const wchar_t* value)
    {
        if (!value) return;
        std::lock_guard<std::mutex> lock(s_mutex);
        s_strings[descriptionId] = value;
        LogUtil::Log("[LegacyForge] ModStrings: registered id=%d -> %ls", descriptionId, value);
    }

    const wchar_t* Get(int descriptionId)
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_strings.find(descriptionId);
        if (it != s_strings.end())
            return it->second.c_str();
        return nullptr;
    }

    int AllocateId()
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_nextId++;
    }

    bool IsModId(int id)
    {
        return id >= MOD_DESC_ID_BASE;
    }
}
