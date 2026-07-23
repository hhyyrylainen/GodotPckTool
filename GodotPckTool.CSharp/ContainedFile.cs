namespace GodotPckTool;

/// <summary>
///   A file that is contained within a PCK file.
/// </summary>
public class ContainedFile
{
    public string Path { get; set; } = string.Empty;
    public ulong Offset { get; set; }
    public ulong Size { get; set; }
    public byte[] Md5 { get; set; } = new byte[16];
    public uint Flags { get; set; }
    public string Salt { get; set; } = string.Empty;

    public Func<byte[]>? GetData { get; set; }
}
