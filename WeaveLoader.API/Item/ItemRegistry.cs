using System.Collections.Generic;

namespace WeaveLoader.API.Item;

/// <summary>
/// Represents an item that has been registered with the game engine.
/// </summary>
public class RegisteredItem
{
    /// <summary>The namespaced string ID (e.g. "mymod:ruby").</summary>
    public Identifier StringId { get; }

    /// <summary>The numeric ID allocated by the engine.</summary>
    public int NumericId { get; }

    internal RegisteredItem(Identifier id, int numericId)
    {
        StringId = id;
        NumericId = numericId;
    }
}

/// <summary>
/// Item registration via the WeaveLoader registry.
/// Accessed through <see cref="Registry.Item"/>.
/// </summary>
public static class ItemRegistry
{
    private static readonly object s_lock = new();
    private static readonly Dictionary<int, Identifier> s_idByNumeric = new();

    private enum ToolKind
    {
        Pickaxe = 1,
        Shovel = 2,
        Hoe = 3,
        Sword = 4,
        Axe = 5,
    }

    private static Identifier? ResolveCustomMaterialId(Item managedItem)
    {
        if (managedItem is ToolItem toolItem && toolItem.CustomMaterialId is Identifier customMaterialId)
            return customMaterialId;

        if (managedItem is PickaxeItem pickaxeItem && pickaxeItem.CustomTierId is Identifier legacyPickaxeTierId)
            return legacyPickaxeTierId;

        return null;
    }

    private static ToolMaterialDefinition? ResolveToolMaterial(Identifier itemId, Item managedItem)
    {
        Identifier? customMaterialId = ResolveCustomMaterialId(managedItem);
        if (customMaterialId == null)
            return null;

        Identifier resolvedMaterialId = (Identifier)customMaterialId;

        if (!ToolMaterialRegistry.TryGetDefinition(resolvedMaterialId, out ToolMaterialDefinition? definition) || definition == null)
            throw new InvalidOperationException($"Unknown tool material '{resolvedMaterialId}' for item '{itemId}'.");

        return definition;
    }

    private static void ConfigureToolMaterial(Identifier itemId, int numericId, ToolKind toolKind, ToolMaterialDefinition? material, ItemProperties properties)
    {
        if (material == null && properties.AttackDamageValue <= 0.0f)
            return;

        int configured = NativeInterop.native_configure_custom_tool_item(
            numericId,
            (int)toolKind,
            material?.HarvestLevelValue ?? 0,
            material?.DestroySpeedValue ?? 1.0f,
            properties.AttackDamageValue);

        if (configured == 0)
            throw new InvalidOperationException($"Failed to configure custom tool material for item '{itemId}'.");
    }

    /// <summary>
    /// Register a new item with the game engine.
    /// </summary>
    /// <param name="id">Namespaced identifier (e.g. "mymod:ruby").</param>
    /// <param name="properties">Item properties built with <see cref="ItemProperties"/>.</param>
    /// <returns>A handle to the registered item.</returns>
    public static RegisteredItem Register(Identifier id, ItemProperties properties)
    {
        return RegisterInternal(id, properties, null);
    }

    /// <summary>
    /// Register a managed custom item implementation.
    /// </summary>
    /// <param name="id">Namespaced identifier (e.g. "mymod:ruby_pickaxe").</param>
    /// <param name="item">Managed item instance that can override callbacks.</param>
    /// <param name="properties">Item properties built with <see cref="ItemProperties"/>.</param>
    /// <returns>A handle to the registered item.</returns>
    public static RegisteredItem Register(Identifier id, Item item, ItemProperties properties)
    {
        return RegisterInternal(id, properties, item);
    }

    private static RegisteredItem RegisterInternal(Identifier id, ItemProperties properties, Item? managedItem)
    {
        Assets.ModelResolver.ApplyItemModel(id, properties);
        int numericId;
        if (managedItem is PickaxeItem pickaxeItem)
        {
            ToolMaterialDefinition? material = ResolveToolMaterial(id, pickaxeItem);
            ToolTier nativeTier = material?.BaseTierValue ?? pickaxeItem.Tier;
            int maxDamage = properties.MaxDamageValue;

            numericId = NativeInterop.native_register_pickaxe_item(
                id.ToString(),
                (int)nativeTier,
                maxDamage,
                properties.IconValue,
                properties.NameValue?.Resolve() ?? "");

            if (numericId >= 0)
                ConfigureToolMaterial(id, numericId, ToolKind.Pickaxe, material, properties);
        }
        else if (managedItem is ShovelItem shovelItem)
        {
            ToolMaterialDefinition? material = ResolveToolMaterial(id, shovelItem);
            numericId = NativeInterop.native_register_shovel_item(
                id.ToString(),
                (int)(material?.BaseTierValue ?? shovelItem.Tier),
                properties.MaxDamageValue,
                properties.IconValue,
                properties.NameValue?.Resolve() ?? "");

            if (numericId >= 0)
                ConfigureToolMaterial(id, numericId, ToolKind.Shovel, material, properties);
        }
        else if (managedItem is HoeItem hoeItem)
        {
            ToolMaterialDefinition? material = ResolveToolMaterial(id, hoeItem);
            numericId = NativeInterop.native_register_hoe_item(
                id.ToString(),
                (int)(material?.BaseTierValue ?? hoeItem.Tier),
                properties.MaxDamageValue,
                properties.IconValue,
                properties.NameValue?.Resolve() ?? "");

            if (numericId >= 0)
                ConfigureToolMaterial(id, numericId, ToolKind.Hoe, material, properties);
        }
        else if (managedItem is AxeItem axeItem)
        {
            ToolMaterialDefinition? material = ResolveToolMaterial(id, axeItem);
            numericId = NativeInterop.native_register_axe_item(
                id.ToString(),
                (int)(material?.BaseTierValue ?? axeItem.Tier),
                properties.MaxDamageValue,
                properties.IconValue,
                properties.NameValue?.Resolve() ?? "");

            if (numericId >= 0)
                ConfigureToolMaterial(id, numericId, ToolKind.Axe, material, properties);
        }
        else if (managedItem is SwordItem swordItem)
        {
            ToolMaterialDefinition? material = ResolveToolMaterial(id, swordItem);
            numericId = NativeInterop.native_register_sword_item(
                id.ToString(),
                (int)(material?.BaseTierValue ?? swordItem.Tier),
                properties.MaxDamageValue,
                properties.IconValue,
                properties.NameValue?.Resolve() ?? "");

            if (numericId >= 0)
                ConfigureToolMaterial(id, numericId, ToolKind.Sword, material, properties);
        }
        else
        {
            numericId = NativeInterop.native_register_item(
                id.ToString(),
                properties.MaxStackSizeValue,
                properties.MaxDamageValue,
                properties.IconValue,
                properties.NameValue?.Resolve() ?? "");
        }

        if (numericId < 0)
        {
            Logger.Error($"Failed to register item '{id}'. No free IDs or invalid parameters.");
            throw new InvalidOperationException($"Failed to register item '{id}'. No free IDs or invalid parameters.");
        }

        if (properties.DisplayTransforms != null && properties.DisplayTransforms.Count > 0)
        {
            foreach (var entry in properties.DisplayTransforms)
            {
                NativeInterop.native_register_item_display_transform(numericId, (int)entry.Key, entry.Value);
            }
        }

        if (properties.RendererValue != null)
            ManagedItemRendererDispatcher.Register(numericId, properties.RendererValue);
        if (properties.HandEquippedValue.HasValue)
            NativeInterop.native_set_item_hand_equipped(numericId, properties.HandEquippedValue.Value ? 1 : 0);

        if (properties.CreativeTabValue != CreativeTab.None)
        {
            bool added = false;
            if (properties.CreativePlacementValue.HasValue)
            {
                CreativePlacement placement = properties.CreativePlacementValue.Value;
                if (placement.Insert == CreativeInsert.Prepend)
                {
                    try
                    {
                        NativeInterop.native_add_to_creative_ex(
                            numericId, 1, 0, (int)properties.CreativeTabValue,
                            (int)placement.Insert, -1, -1);
                    }
                    catch (DllNotFoundException e)
                    {
                        Logger.Error($"Creative add failed for item '{id}': {e.Message}. Check that WeaveLoaderRuntime is present.");
                        throw;
                    }
                    catch (EntryPointNotFoundException e)
                    {
                        Logger.Error($"Creative add failed for item '{id}': {e.Message}. API/runtime mismatch.");
                        throw;
                    }
                    added = true;
                }
            }

            if (!added)
            {
                try
                {
                    NativeInterop.native_add_to_creative(numericId, 1, 0, (int)properties.CreativeTabValue);
                }
                catch (DllNotFoundException e)
                {
                    Logger.Error($"Creative add failed for item '{id}': {e.Message}. Check that WeaveLoaderRuntime is present.");
                    throw;
                }
                catch (EntryPointNotFoundException e)
                {
                    Logger.Error($"Creative add failed for item '{id}': {e.Message}. API/runtime mismatch.");
                    throw;
                }
            }

            Logger.Debug($"Item '{id}' added to creative tab {properties.CreativeTabValue}");
        }
        else
        {
            Logger.Debug($"Item '{id}' not added to creative (CreativeTab.None)");
        }

        if (managedItem != null)
        {
            ManagedItemDispatcher.RegisterItem(id, numericId, managedItem);
            Logger.Debug($"Managed item dispatcher mapped '{id}' -> numeric ID {numericId} ({managedItem.GetType().FullName})");
        }

        Logger.Debug($"Registered item '{id}' -> numeric ID {numericId}");
        lock (s_lock)
        {
            s_idByNumeric[numericId] = id;
        }
        return new RegisteredItem(id, numericId);
    }

    internal static bool TryGetIdentifier(int numericId, out Identifier id)
    {
        lock (s_lock)
        {
            return s_idByNumeric.TryGetValue(numericId, out id);
        }
    }
}
