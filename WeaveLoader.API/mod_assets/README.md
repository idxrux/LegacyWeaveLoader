# WeaveLoader.API Mod Assets

Placeholder textures for the missing block and missing item (unresolved mod content).
Stored in `mod_assets/` to avoid conflict with the `Assets/` C# source folder (Windows case-insensitivity).
Copied to `assets/weaveloader.api/textures/block/` and `assets/weaveloader.api/textures/item/` in the build output.

- **Blocks:** `mod_assets/weaveloader.api/textures/block/missing_block.png` → icon `weaveloader.api:block/missing_block`
- **Items:** `mod_assets/weaveloader.api/textures/item/missing_item.png` → icon `weaveloader.api:item/missing_item`

These are used when a world contains blocks or items from mods that are no longer installed.
