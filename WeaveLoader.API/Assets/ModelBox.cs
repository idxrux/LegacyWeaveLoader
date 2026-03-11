using System.Runtime.InteropServices;

namespace WeaveLoader.API.Assets;

[StructLayout(LayoutKind.Sequential)]
internal struct ModelBox
{
    public float X0;
    public float Y0;
    public float Z0;
    public float X1;
    public float Y1;
    public float Z1;
}
