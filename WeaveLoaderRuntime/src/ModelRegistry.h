#pragma once

#include <vector>

struct ModelBox
{
    float x0;
    float y0;
    float z0;
    float x1;
    float y1;
    float z1;
};

namespace ModelRegistry
{
    void RegisterBlockModel(int blockId, const ModelBox* boxes, int count);
    bool TryGetModel(int blockId, const std::vector<ModelBox>*& outBoxes);
}
