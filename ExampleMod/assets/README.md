# ExampleMod Assets

## Language files

Language files live in `assets/examplemod/lang/` with the format `{locale}.lang` (e.g. `en-GB.lang`, `de-DE.lang`).

Use `Text.Translatable()` with `BlockProperties.Name()` / `ItemProperties.Name()` to pull localized strings
from these `.lang` files. Format: `key=value` per line, with `#` for comments.

Example:

```csharp
.Name(Text.Translatable("item.examplemod.ruby"))
.Name(Text.Literal("Ruby")) // literal fallback if you don't want localization
```

## Textures

Mod textures use Java-style asset paths. Place PNG files in:

- **Blocks:** `assets/examplemod/textures/block/{name}.png` → icon `examplemod:block/{name}`
- **Items:** `assets/examplemod/textures/item/{name}.png` → icon `examplemod:item/{name}`

Use the Java-style icon in `BlockProperties.Icon()` and `ItemProperties.Icon()`:

```csharp
.Icon("examplemod:block/ruby_ore")  // block from assets/examplemod/textures/block/ruby_ore.png
.Icon("examplemod:item/ruby")       // item from assets/examplemod/textures/item/ruby.png
```

Textures must be 16×16 pixels (or any size; they are scaled).

## Models (Java-style)

Block and item models are supported using Java-style JSON assets:

- **Blocks:** `assets/examplemod/models/block/{name}.json`
- **Items:** `assets/examplemod/models/item/{name}.json`
- **Entities (future):** `assets/examplemod/models/entity/{name}.json`

The `examplemod` namespace should match your mod ID (lowercase).
