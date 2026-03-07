# ExampleMod Assets

## Language files

Language files live in `assets/lang/` with the format `{locale}.lang` (e.g. `en-GB.lang`, `de-DE.lang`).

**Current API:** Use `BlockProperties.Name()` and `ItemProperties.Name()` when registering blocks and items. These set the display name shown in-game. The ModLoader hooks into the game's string lookup so your names appear correctly.

**Future:** Multi-locale support may load from these `.lang` files. Format: `key=value` per line, with `#` for comments.

## Textures

Mod textures are supported via the dynamic atlas system. Place PNG files in:

- **Blocks:** `assets/blocks/{name}.png` → icon `{modid}:{name}` (e.g. `ruby_ore.png` → `examplemod:ruby_ore`)
- **Items:** `assets/items/{name}.png` → icon `{modid}:{name}` (e.g. `ruby.png` → `examplemod:ruby`)

The mod ID is derived from the mod folder name (lowercase, hyphens removed). Use the namespaced icon in `BlockProperties.Icon()` and `ItemProperties.Icon()`:

```csharp
.Icon("examplemod:ruby_ore")  // block from assets/blocks/ruby_ore.png
.Icon("examplemod:ruby")      // item from assets/items/ruby.png
```

Textures must be 16×16 pixels (or any size; they are scaled). For vanilla icons, use names like `gold_ore`, `diamond`, etc.
