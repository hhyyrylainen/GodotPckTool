namespace GodotPckTool;

/// <summary>
///   Constants used for PCK handling.
/// </summary>
public static class Constants
{
    public const uint PckHeaderMagic = 0x43504447;
    public const uint PackDirEncrypted = 1 << 0;
    public const uint PckFileEncrypted = 1 << 0;
    public const uint PckFileDeleted = 1 << 1;

    public const uint PckFileRelativeBase = 1 << 1;
    public const uint PckFileSparseBundle = 1 << 2;

    public const int MaxSupportedPckVersionLoad = 4;
    public const int MaxSupportedPckVersionSave = 4;

    public const int Godot3PckVersion = 1;
    public const int Godot4PckVersion = 2;
    public const int Godot45PckVersion = 3;
    public const int Godot47PckVersion = 4;

    public const string GodotPckExtension = ".pck";
    public const string GodotResPath = "res://";
}