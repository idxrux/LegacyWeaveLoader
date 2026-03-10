using System.Globalization;
using System.Runtime.InteropServices;

namespace WeaveLoader.API;

/// <summary>
/// Localized text value used for names, tooltips, and UI strings.
/// </summary>
public sealed class Text
{
    public enum TextKind
    {
        Literal = 0,
        Translatable = 1
    }

    public TextKind Kind { get; }
    public string Value { get; }

    private Text(TextKind kind, string value)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(value);
        Kind = kind;
        Value = value;
    }

    public static Text Literal(string value) => new(TextKind.Literal, value);
    public static Text Translatable(string key) => new(TextKind.Translatable, key);

    internal string Resolve()
    {
        return Kind == TextKind.Literal ? Value : Localization.Resolve(Value);
    }
}

/// <summary>
/// Simple language table loader for mod translations (Java-style .lang files).
/// </summary>
public static class Localization
{
    private static readonly object s_lock = new();
    private static string s_locale = GetDefaultLocale();
    private static string? s_loadedLocale;
    private static Dictionary<string, string> s_entries = new(StringComparer.Ordinal);
    private static int s_lastGameLanguage = int.MinValue;

    /// <summary>When true, follow the game's current language selection.</summary>
    public static bool UseGameLanguage { get; set; } = true;

    /// <summary>Active locale used when resolving translatable keys (e.g. "en-GB").</summary>
    public static string Locale
    {
        get => s_locale;
        set
        {
            ArgumentException.ThrowIfNullOrWhiteSpace(value);
            lock (s_lock)
            {
                s_locale = value;
                s_loadedLocale = null;
            }
        }
    }

    /// <summary>Reload language files for the current locale.</summary>
    public static void Reload()
    {
        lock (s_lock)
        {
            s_loadedLocale = null;
        }
    }

    internal static string Resolve(string key)
    {
        EnsureLoaded();
        return s_entries.TryGetValue(key, out var value) ? value : key;
    }

    private static void EnsureLoaded()
    {
        lock (s_lock)
        {
            if (UseGameLanguage)
            {
                string? gameLocale = TryGetGameLocale();
                if (!string.IsNullOrWhiteSpace(gameLocale) && !string.Equals(gameLocale, s_locale, StringComparison.OrdinalIgnoreCase))
                {
                    s_locale = gameLocale;
                    s_loadedLocale = null;
                }
            }
            if (s_loadedLocale == s_locale)
                return;
            s_entries = LoadLocaleTable(s_locale);
            s_loadedLocale = s_locale;
        }
    }

    private static string GetDefaultLocale()
    {
        string? gameLocale = TryGetGameLocale();
        if (!string.IsNullOrWhiteSpace(gameLocale))
            return gameLocale!;
        var env = Environment.GetEnvironmentVariable("WEAVELOADER_LOCALE");
        if (!string.IsNullOrWhiteSpace(env))
            return env;
        try
        {
            var culture = CultureInfo.CurrentUICulture;
            if (!string.IsNullOrWhiteSpace(culture.Name))
                return culture.Name.Replace('_', '-');
        }
        catch
        {
            // ignore
        }
        return "en-GB";
    }

    private static string? TryGetGameLocale()
    {
        try
        {
            int lang = NativeInterop.native_get_minecraft_language();
            if (lang == s_lastGameLanguage)
                return null;
            s_lastGameLanguage = lang;
            return MapLanguageToLocale(lang);
        }
        catch
        {
            return null;
        }
    }

    private static string? MapLanguageToLocale(int lang)
    {
        // 0 = default (system); return null so we fall back to system culture / env.
        return lang switch
        {
            1 => "en-GB",
            2 => "ja-JP",
            3 => "de-DE",
            4 => "fr-FR",
            5 => "es-ES",
            6 => "it-IT",
            7 => "ko-KR",
            8 => "zh-TW",
            9 => "pt-PT",
            10 => "pt-BR",
            11 => "ru-RU",
            12 => "nl-NL",
            13 => "fi-FI",
            14 => "sv-SE",
            15 => "da-DK",
            16 => "no-NO",
            17 => "pl-PL",
            18 => "tr-TR",
            19 => "es-MX",
            20 => "el-GR",
            _ => null
        };
    }

    private static Dictionary<string, string> LoadLocaleTable(string locale)
    {
        var entries = new Dictionary<string, string>(StringComparer.Ordinal);
        var locales = BuildLocaleFallbacks(locale);
        foreach (var loc in locales)
            LoadLocaleFiles(entries, loc);
        return entries;
    }

    private static List<string> BuildLocaleFallbacks(string locale)
    {
        var list = new List<string>();
        void Add(string value)
        {
            if (string.IsNullOrWhiteSpace(value))
                return;
            if (!list.Contains(value, StringComparer.OrdinalIgnoreCase))
                list.Add(value);
        }

        Add(locale);
        Add(locale.Replace('_', '-'));
        Add(locale.Replace('-', '_'));
        Add("en-GB");
        Add("en-US");
        return list;
    }

    private static void LoadLocaleFiles(Dictionary<string, string> entries, string locale)
    {
        foreach (var modsPath in GetModsRoots())
        {
            foreach (var modDir in Directory.EnumerateDirectories(modsPath))
            {
                var assetsDir = Path.Combine(modDir, "assets");
                if (!Directory.Exists(assetsDir))
                    continue;

                foreach (var nsDir in Directory.EnumerateDirectories(assetsDir))
                {
                    var langFile = Path.Combine(nsDir, "lang", $"{locale}.lang");
                    if (!File.Exists(langFile))
                        continue;
                    ParseLangFile(langFile, entries);
                }
            }
        }
    }

    private static void ParseLangFile(string path, Dictionary<string, string> entries)
    {
        foreach (var rawLine in File.ReadLines(path))
        {
            var line = rawLine.Trim();
            if (line.Length == 0 || line.StartsWith('#'))
                continue;

            int idx = line.IndexOf('=');
            if (idx <= 0 || idx >= line.Length - 1)
                continue;

            string key = line[..idx].Trim();
            string value = line[(idx + 1)..].Trim();
            if (key.Length == 0)
                continue;

            entries[key] = value;
        }
    }

    private static List<string> GetModsRoots()
    {
        var roots = new List<string>();
        void AddCandidate(string? path)
        {
            if (string.IsNullOrWhiteSpace(path))
                return;
            string full;
            try
            {
                full = Path.GetFullPath(path);
            }
            catch
            {
                return;
            }
            if (!Directory.Exists(full))
                return;
            if (!roots.Contains(full, StringComparer.OrdinalIgnoreCase))
                roots.Add(full);
        }

        AddCandidate(GetNativeModsPath());

        var baseDir = AppContext.BaseDirectory;
        AddCandidate(Path.Combine(baseDir, "mods"));
        AddCandidate(Path.Combine(baseDir, "..", "mods"));
        AddCandidate(Path.Combine(baseDir, "..", "..", "mods"));

        var cwd = Directory.GetCurrentDirectory();
        AddCandidate(Path.Combine(cwd, "mods"));
        AddCandidate(Path.Combine(cwd, "..", "mods"));

        return roots;
    }

    private static string? GetNativeModsPath()
    {
        try
        {
            var ptr = NativeInterop.native_get_mods_path();
            if (ptr == nint.Zero)
                return null;
            return Marshal.PtrToStringAnsi(ptr);
        }
        catch
        {
            return null;
        }
    }
}
