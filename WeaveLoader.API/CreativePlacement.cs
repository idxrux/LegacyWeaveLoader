namespace WeaveLoader.API;

public enum CreativeInsert
{
    Append = 0,
    Prepend = 1
}

public readonly struct CreativePlacement
{
    public CreativeInsert Insert { get; }

    private CreativePlacement(CreativeInsert insert)
    {
        Insert = insert;
    }

    public static CreativePlacement Append() => new(CreativeInsert.Append);
    public static CreativePlacement Prepend() => new(CreativeInsert.Prepend);
}
