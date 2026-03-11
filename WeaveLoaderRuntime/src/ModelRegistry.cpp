#include "ModelRegistry.h"
#include "LogUtil.h"
#include <unordered_map>
#include <mutex>

namespace
{
    std::unordered_map<int, std::vector<ModelBox>> g_models;
    std::mutex g_mutex;
}

void ModelRegistry::RegisterBlockModel(int blockId, const ModelBox* boxes, int count)
{
    if (blockId < 0 || !boxes || count <= 0)
        return;

    std::vector<ModelBox> data;
    data.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; i++)
        data.push_back(boxes[i]);

    {
        std::lock_guard<std::mutex> guard(g_mutex);
        g_models[blockId] = std::move(data);
    }

    LogUtil::Log("[WeaveLoader] ModelRegistry: registered %d box(es) for block %d", count, blockId);
}

bool ModelRegistry::TryGetModel(int blockId, const std::vector<ModelBox>*& outBoxes)
{
    std::lock_guard<std::mutex> guard(g_mutex);
    auto it = g_models.find(blockId);
    if (it == g_models.end())
        return false;
    outBoxes = &it->second;
    return true;
}
