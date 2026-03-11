using WeaveLoader.API;

namespace WeaveLoader.API.Item;

/// <summary>
/// Fluent builder for defining item properties.
/// </summary>
public class ItemProperties
{
    internal int MaxStackSizeValue = 64;
    internal int MaxDamageValue = 0;
    internal float AttackDamageValue = 0.0f;
    internal string IconValue = "";
    internal string? ModelValue;
    internal CreativeTab CreativeTabValue = CreativeTab.None;
    internal CreativePlacement? CreativePlacementValue;
    internal Text? NameValue;

    public ItemProperties MaxStackSize(int size) { MaxStackSizeValue = size; return this; }
    /// <summary>
    /// Icon name in the items atlas. Use Java-style names like "examplemod:item/ruby"
    /// from assets/examplemod/textures/item/ruby.png, or vanilla names like "diamond", "ingotIron".
    /// </summary>
    public ItemProperties Icon(string iconName) { IconValue = iconName; return this; }
    /// <summary>
    /// Optional Java-style model name (e.g. "examplemod:item/ruby").
    /// When provided, WeaveLoader will read assets/&lt;namespace&gt;/models/item/&lt;name&gt;.json
    /// and use its texture for the item icon.
    /// </summary>
    public ItemProperties Model(string modelName) { ModelValue = modelName; return this; }

    /// <summary>
    /// Set max damage for a tool/armor item. Setting this to a positive value
    /// makes the item damageable with a durability bar.
    /// </summary>
    public ItemProperties MaxDamage(int damage) { MaxDamageValue = damage; MaxStackSizeValue = 1; return this; }
    /// <summary>Override the native attack damage value for tool items.</summary>
    public ItemProperties AttackDamage(float damage) { AttackDamageValue = damage; return this; }
    public ItemProperties InCreativeTab(CreativeTab tab) { CreativeTabValue = tab; return this; }
    public ItemProperties CreativePlacement(CreativePlacement placement) { CreativePlacementValue = placement; return this; }
    public ItemProperties Prepend() { CreativePlacementValue = global::WeaveLoader.API.CreativePlacement.Prepend(); return this; }
    /// <summary>Display name shown in-game (e.g. "Ruby"). Used for localization.</summary>
    public ItemProperties Name(string displayName) { NameValue = Text.Literal(displayName); return this; }
    /// <summary>Localized display name using a language key (e.g. "item.examplemod.ruby").</summary>
    public ItemProperties Name(Text text) { NameValue = text; return this; }
}
