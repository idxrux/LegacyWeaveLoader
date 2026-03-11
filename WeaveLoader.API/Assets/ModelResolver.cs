using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;

namespace WeaveLoader.API.Assets;

internal static class ModelResolver
{
    private sealed class ModelData
    {
        public string IconName = "";
        public List<ModelBox> Boxes = new();
    }

    private enum ModelKind
    {
        Block,
        Item
    }

    internal static void ApplyBlockModel(Identifier id, Block.BlockProperties properties)
    {
        if (!ShouldResolveModel(properties.ModelValue, properties.IconValue, ModelKind.Block))
            return;

        if (TryLoadModel(id, properties.ModelValue, ModelKind.Block, out ModelData? model))
        {
            if (!string.IsNullOrWhiteSpace(model.IconName))
            {
                properties.IconValue = model.IconName;
                Logger.Debug($"Model resolved for block '{id}' -> icon '{model.IconName}'");
            }
            if (model.Boxes.Count > 0)
            {
                properties.ModelBoxes = model.Boxes;
                properties.ModelIsFullCube = IsFullCube(model.Boxes);
            }
        }
        else if (!string.IsNullOrWhiteSpace(properties.ModelValue))
        {
            Logger.Warning($"Model not found for block '{id}' (model '{properties.ModelValue}')");
        }
    }

    internal static void ApplyItemModel(Identifier id, Item.ItemProperties properties)
    {
        if (!ShouldResolveModel(properties.ModelValue, properties.IconValue, ModelKind.Item))
            return;

        if (TryLoadModel(id, properties.ModelValue, ModelKind.Item, out ModelData? model))
        {
            if (!string.IsNullOrWhiteSpace(model.IconName))
            {
                properties.IconValue = model.IconName;
                Logger.Debug($"Model resolved for item '{id}' -> icon '{model.IconName}'");
            }
        }
        else if (!string.IsNullOrWhiteSpace(properties.ModelValue))
        {
            Logger.Warning($"Model not found for item '{id}' (model '{properties.ModelValue}')");
        }
    }

    private static bool ShouldResolveModel(string? modelValue, string iconValue, ModelKind kind)
    {
        if (!string.IsNullOrWhiteSpace(modelValue))
            return true;

        // Only auto-resolve if icon was not provided.
        if (kind == ModelKind.Item && string.IsNullOrWhiteSpace(iconValue))
            return true;

        return false;
    }

    private static bool TryLoadModel(Identifier id, string? modelValue, ModelKind kind, out ModelData? model)
    {
        model = null;
        if (string.IsNullOrWhiteSpace(ModContext.ModFolder))
            return false;

        string modelPath;
        string modelNamespace;
        ModelKind effectiveKind = kind;

        if (kind == ModelKind.Item)
        {
            if (TryGetModelFilePath(id, modelValue, ModelKind.Block, out string blockModelPath, out string blockNamespace) &&
                File.Exists(blockModelPath))
            {
                modelPath = blockModelPath;
                modelNamespace = blockNamespace;
                effectiveKind = ModelKind.Block;
            }
            else if (TryGetModelFilePath(id, modelValue, ModelKind.Item, out string itemModelPath, out string itemNamespace) &&
                     File.Exists(itemModelPath))
            {
                modelPath = itemModelPath;
                modelNamespace = itemNamespace;
            }
            else
            {
                return false;
            }
        }
        else
        {
            if (!TryGetModelFilePath(id, modelValue, kind, out modelPath, out modelNamespace))
                return false;
            if (!File.Exists(modelPath))
                return false;
        }

        if (!TryParseTextures(modelPath, out Dictionary<string, string> textures))
            return false;

        var data = new ModelData();

        string? texture = SelectTexture(textures, effectiveKind);
        if (!string.IsNullOrWhiteSpace(texture))
        {
            texture = ResolveTextureReference(texture, textures);
            if (!string.IsNullOrWhiteSpace(texture))
                data.IconName = NormalizeTextureName(texture, modelNamespace);
        }

        TryParseElements(modelPath, data.Boxes);
        model = data;
        return !string.IsNullOrWhiteSpace(data.IconName) || data.Boxes.Count > 0;
    }

    private static bool TryGetModelFilePath(Identifier id, string? modelValue, ModelKind kind, out string modelPath, out string modelNamespace)
    {
        modelPath = "";
        modelNamespace = "";

        string ns = id.Namespace;
        string rel = id.Path;

        if (!string.IsNullOrWhiteSpace(modelValue))
        {
            string raw = modelValue!;
            int colon = raw.IndexOf(':');
            if (colon >= 0)
            {
                ns = raw[..colon];
                rel = raw[(colon + 1)..];
            }
            else
            {
                rel = raw;
            }
        }

        rel = rel.Replace('\\', '/');
        if (rel.StartsWith("models/", StringComparison.OrdinalIgnoreCase))
            rel = rel["models/".Length..];
        if (!rel.StartsWith("block/", StringComparison.OrdinalIgnoreCase) &&
            !rel.StartsWith("item/", StringComparison.OrdinalIgnoreCase))
        {
            rel = (kind == ModelKind.Block ? "block/" : "item/") + rel;
        }

        string modRoot = ModContext.ModFolder ?? "";
        if (string.IsNullOrWhiteSpace(modRoot))
            return false;

        string file = Path.Combine(modRoot, "assets", ns, "models", rel.Replace('/', Path.DirectorySeparatorChar) + ".json");
        modelPath = file;
        modelNamespace = ns;
        return true;
    }

    private static bool TryParseTextures(string modelPath, out Dictionary<string, string> textures)
    {
        textures = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        try
        {
            using var doc = JsonDocument.Parse(File.ReadAllText(modelPath));
            if (!doc.RootElement.TryGetProperty("textures", out JsonElement texElem))
                return false;
            if (texElem.ValueKind != JsonValueKind.Object)
                return false;

            foreach (var prop in texElem.EnumerateObject())
            {
                if (prop.Value.ValueKind != JsonValueKind.String)
                    continue;
                string? value = prop.Value.GetString();
                if (string.IsNullOrWhiteSpace(value))
                    continue;
                textures[prop.Name] = value!;
            }
            return textures.Count > 0;
        }
        catch (Exception ex)
        {
            Logger.Warning($"Failed to parse model JSON '{modelPath}': {ex.Message}");
            return false;
        }
    }

    private static void TryParseElements(string modelPath, List<ModelBox> boxes)
    {
        try
        {
            using var doc = JsonDocument.Parse(File.ReadAllText(modelPath));
            if (!doc.RootElement.TryGetProperty("elements", out JsonElement elements) ||
                elements.ValueKind != JsonValueKind.Array)
                return;

            foreach (var elem in elements.EnumerateArray())
            {
                if (!elem.TryGetProperty("from", out JsonElement fromEl) ||
                    !elem.TryGetProperty("to", out JsonElement toEl))
                    continue;

                if (fromEl.ValueKind != JsonValueKind.Array || toEl.ValueKind != JsonValueKind.Array)
                    continue;

                if (fromEl.GetArrayLength() < 3 || toEl.GetArrayLength() < 3)
                    continue;

                float fx0 = (float)fromEl[0].GetDouble();
                float fy0 = (float)fromEl[1].GetDouble();
                float fz0 = (float)fromEl[2].GetDouble();
                float fx1 = (float)toEl[0].GetDouble();
                float fy1 = (float)toEl[1].GetDouble();
                float fz1 = (float)toEl[2].GetDouble();

                float x0 = MathF.Min(fx0, fx1) / 16.0f;
                float y0 = MathF.Min(fy0, fy1) / 16.0f;
                float z0 = MathF.Min(fz0, fz1) / 16.0f;
                float x1 = MathF.Max(fx0, fx1) / 16.0f;
                float y1 = MathF.Max(fy0, fy1) / 16.0f;
                float z1 = MathF.Max(fz0, fz1) / 16.0f;

                if (x1 <= x0 || y1 <= y0 || z1 <= z0)
                    continue;

                boxes.Add(new ModelBox
                {
                    X0 = x0,
                    Y0 = y0,
                    Z0 = z0,
                    X1 = x1,
                    Y1 = y1,
                    Z1 = z1
                });
            }
        }
        catch (Exception ex)
        {
            Logger.Warning($"Failed to parse model elements in '{modelPath}': {ex.Message}");
        }
    }

    private static bool IsFullCube(List<ModelBox> boxes)
    {
        if (boxes.Count != 1)
            return false;
        ModelBox box = boxes[0];
        const float eps = 0.0001f;
        return box.X0 <= 0.0f + eps && box.Y0 <= 0.0f + eps && box.Z0 <= 0.0f + eps &&
               box.X1 >= 1.0f - eps && box.Y1 >= 1.0f - eps && box.Z1 >= 1.0f - eps;
    }

    private static string? SelectTexture(Dictionary<string, string> textures, ModelKind kind)
    {
        string[] keys = kind == ModelKind.Item
            ? new[] { "layer0", "layer1" }
            : new[] { "all", "side", "top", "bottom", "particle" };

        foreach (string key in keys)
        {
            if (textures.TryGetValue(key, out string value))
                return value;
        }

        foreach (var kvp in textures)
            return kvp.Value;

        return null;
    }

    private static string ResolveTextureReference(string texture, Dictionary<string, string> textures)
    {
        string current = texture;
        for (int i = 0; i < 8; i++)
        {
            if (!current.StartsWith("#", StringComparison.Ordinal))
                return current;
            string key = current[1..];
            if (!textures.TryGetValue(key, out string next) || string.IsNullOrWhiteSpace(next))
                return "";
            current = next;
        }
        return "";
    }

    private static string NormalizeTextureName(string texture, string modelNamespace)
    {
        string value = texture.Replace('\\', '/');
        if (value.Contains(':'))
            return value;
        if (string.IsNullOrWhiteSpace(modelNamespace))
            return value;
        return $"{modelNamespace}:{value}";
    }
}
