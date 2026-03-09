#pragma once

#include <string>

namespace WorldIdRemap
{
    void SetTagNewTagSymbol(void* fnPtr);
    void EnsureMissingPlaceholders();
    void TagModdedItemInstance(void* itemInstancePtr, void* compoundTagPtr);
    void RemapItemInstanceFromTag(void* itemInstancePtr, void* compoundTagPtr);
}
