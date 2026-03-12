#pragma once

#include <cstdint>

struct ItemDisplayTransformNative
{
    float rotX;
    float rotY;
    float rotZ;
    float transX;
    float transY;
    float transZ;
    float scaleX;
    float scaleY;
    float scaleZ;
};

enum ItemDisplayContextNative : int
{
    ItemDisplay_Gui = 0,
    ItemDisplay_Ground = 1,
    ItemDisplay_Fixed = 2,
    ItemDisplay_Head = 3,
    ItemDisplay_FirstPersonRightHand = 4,
    ItemDisplay_FirstPersonLeftHand = 5,
    ItemDisplay_ThirdPersonRightHand = 6,
    ItemDisplay_ThirdPersonLeftHand = 7,
    ItemDisplay_ContextCount = 8
};

using ManagedItemRenderFn = int(__cdecl *)(const void* args, int sizeBytes);

namespace ItemRenderRegistry
{
    void RegisterDisplayTransform(int itemId, int context, const ItemDisplayTransformNative& transform);
    bool TryGetDisplayTransform(int itemId, int context, ItemDisplayTransformNative& outTransform);
    void RegisterCustomRenderer(int itemId, ManagedItemRenderFn fn);
    ManagedItemRenderFn GetCustomRenderer(int itemId);
}
