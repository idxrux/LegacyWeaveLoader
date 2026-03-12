using System;
using System.Runtime.InteropServices;

namespace WeaveLoader.API.Item;

public enum ItemDisplayContext
{
    Gui = 0,
    Ground = 1,
    Fixed = 2,
    Head = 3,
    FirstPersonRightHand = 4,
    FirstPersonLeftHand = 5,
    ThirdPersonRightHand = 6,
    ThirdPersonLeftHand = 7,
}

[StructLayout(LayoutKind.Sequential)]
public struct ItemDisplayTransform
{
    public float RotationX;
    public float RotationY;
    public float RotationZ;
    public float TranslationX;
    public float TranslationY;
    public float TranslationZ;
    public float ScaleX;
    public float ScaleY;
    public float ScaleZ;

    public ItemDisplayTransform(
        float rotationX,
        float rotationY,
        float rotationZ,
        float translationX,
        float translationY,
        float translationZ,
        float scaleX,
        float scaleY,
        float scaleZ)
    {
        RotationX = rotationX;
        RotationY = rotationY;
        RotationZ = rotationZ;
        TranslationX = translationX;
        TranslationY = translationY;
        TranslationZ = translationZ;
        ScaleX = scaleX;
        ScaleY = scaleY;
        ScaleZ = scaleZ;
    }

    public static ItemDisplayTransform Identity => new(
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f);
}

public readonly struct ItemRenderContext
{
    public int ItemId { get; }
    public ItemDisplayContext DisplayContext { get; }
    public IntPtr RendererPtr { get; }
    public IntPtr ItemInstancePtr { get; }
    public float X { get; }
    public float Y { get; }
    public float ScaleX { get; }
    public float ScaleY { get; }
    public float Alpha { get; }

    internal ItemRenderContext(ItemRenderNativeArgs args)
    {
        ItemId = args.ItemId;
        DisplayContext = (ItemDisplayContext)args.Context;
        RendererPtr = args.RendererPtr;
        ItemInstancePtr = args.ItemInstancePtr;
        X = args.X;
        Y = args.Y;
        ScaleX = args.ScaleX;
        ScaleY = args.ScaleY;
        Alpha = args.Alpha;
    }
}

public interface IItemRenderer
{
    bool Render(ItemRenderContext context);
}

[StructLayout(LayoutKind.Sequential)]
internal struct ItemRenderNativeArgs
{
    public int ItemId;
    public int Context;
    public IntPtr RendererPtr;
    public IntPtr ItemInstancePtr;
    public float X;
    public float Y;
    public float ScaleX;
    public float ScaleY;
    public float Alpha;
}
