using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace WeaveLoader.API.Item;

internal static class ManagedItemRendererDispatcher
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate int NativeItemRenderDelegate(IntPtr args, int sizeBytes);

    private static readonly Dictionary<int, IItemRenderer> s_renderers = new();
    private static readonly NativeItemRenderDelegate s_callback = HandleRender;
    private static readonly IntPtr s_callbackPtr = Marshal.GetFunctionPointerForDelegate(s_callback);
    private static readonly int s_argsSize = Marshal.SizeOf<ItemRenderNativeArgs>();

    internal static void Register(int itemId, IItemRenderer renderer)
    {
        s_renderers[itemId] = renderer;
        NativeInterop.native_register_item_renderer(itemId, s_callbackPtr);
    }

    private static int HandleRender(IntPtr args, int sizeBytes)
    {
        if (args == IntPtr.Zero || sizeBytes < s_argsSize)
            return 0;

        ItemRenderNativeArgs nativeArgs = Marshal.PtrToStructure<ItemRenderNativeArgs>(args);
        if (!s_renderers.TryGetValue(nativeArgs.ItemId, out IItemRenderer? renderer) || renderer == null)
            return 0;

        try
        {
            return renderer.Render(new ItemRenderContext(nativeArgs)) ? 1 : 0;
        }
        catch (Exception ex)
        {
            Logger.Error($"Item renderer for {nativeArgs.ItemId} threw: {ex.Message}");
            return 0;
        }
    }
}
