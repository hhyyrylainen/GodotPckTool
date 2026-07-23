using System.Security.Cryptography;
using System.Text;

namespace GodotPckTool;

/// <summary>
///   A C# implementation of the Godot PCK file format.
/// </summary>
public class PckFile : IDisposable
{
    private Stream? _readStream;
    private BinaryReader? _reader;

    public PckFile(string path)
    {
        Path = path;
    }

    public string Path { get; private set; }

    public uint FormatVersion { get; set; } = Constants.Godot4PckVersion;
    public uint MajorGodotVersion { get; private set; }
    public uint MinorGodotVersion { get; private set; }
    public uint PatchGodotVersion { get; private set; }

    public string Salt { get; set; } = string.Empty;
    public uint Flags { get; private set; }
    public ulong FileOffsetBase { get; private set; }
    public ulong DirectoryOffset { get; private set; }
    public int Alignment { get; set; }
    public int PadPathsToMultipleWithNulls { get; set; } = 4;
    public bool NoResPrefix { get; set; } = false;

    public Dictionary<string, ContainedFile> Contents { get; } = [];
    public Func<ContainedFile, bool>? IncludeFilter { get; set; }


    public bool Load(bool throwOnError = true)
    {
        Contents.Clear();

        try
        {
            _readStream = File.Open(Path, FileMode.Open, FileAccess.Read, FileShare.Read);
            _reader = new BinaryReader(_readStream, Encoding.UTF8, leaveOpen: true);

            long pckStart = _readStream.Position;

            uint magic = _reader.ReadUInt32();
            if (magic != Constants.PckHeaderMagic)
            {
                Console.WriteLine("ERROR: invalid magic number");
                if (throwOnError)
                    throw new Exception("Invalid magic number");
                return false;
            }

            FormatVersion = _reader.ReadUInt32();
            MajorGodotVersion = _reader.ReadUInt32();
            MinorGodotVersion = _reader.ReadUInt32();
            PatchGodotVersion = _reader.ReadUInt32();

            if (FormatVersion > Constants.MaxSupportedPckVersionLoad)
            {
                Console.WriteLine($"ERROR: pck is unsupported version: {FormatVersion}");
                if (throwOnError)
                    throw new Exception("Unsupported pck version");
                return false;
            }

            if (FormatVersion >= 2)
            {
                Flags = _reader.ReadUInt32();
                FileOffsetBase = _reader.ReadUInt64();
            }

            if ((Flags & Constants.PackDirEncrypted) != 0)
            {
                Console.WriteLine("ERROR: pck is encrypted");
                if (throwOnError)
                    throw new Exception("Pck is encrypted");
                return false;
            }

            if ((Flags & Constants.PckFileSparseBundle) != 0)
            {
                Console.WriteLine("Warning: Sparse pck detected, this is unlikely to work!");
            }

            if (FormatVersion >= 3 || (FormatVersion == 2 && (Flags & Constants.PckFileRelativeBase) != 0))
            {
                FileOffsetBase += (ulong)pckStart;
            }

            if (FormatVersion >= 3)
            {
                DirectoryOffset = _reader.ReadUInt64();

                if (FormatVersion >= 4 && (Flags & Constants.PckFileSparseBundle) != 0 &&
                    (Flags & Constants.PackDirEncrypted) != 0)
                {
                    byte[] saltBytes = _reader.ReadBytes(32);
                    Salt = Encoding.UTF8.GetString(saltBytes);
                }

                _readStream.Seek((long)(pckStart + (long)DirectoryOffset), SeekOrigin.Begin);
            }
            else
            {
                // Reserved
                for (int i = 0; i < 16; i++)
                {
                    _reader.ReadUInt32();
                }
            }

            uint fileCount = _reader.ReadUInt32();
            uint excluded = 0;

            for (uint i = 0; i < fileCount; i++)
            {
                uint pathLength = _reader.ReadUInt32();
                byte[] pathBytes = _reader.ReadBytes((int)pathLength);
                string path = Encoding.UTF8.GetString(pathBytes).TrimEnd('\0');

                ContainedFile entry = new()
                {
                    Path = path,
                    Offset = FileOffsetBase + _reader.ReadUInt64(),
                    Size = _reader.ReadUInt64(),
                    Md5 = _reader.ReadBytes(16)
                };

                if (FormatVersion >= 2)
                {
                    entry.Flags = _reader.ReadUInt32();
                    entry.Salt = Salt;

                    if ((entry.Flags & Constants.PckFileEncrypted) != 0)
                    {
                        Console.WriteLine(
                            $"WARNING: pck file ({entry.Path}) is marked as encrypted, decoding the encryption is not implemented");
                    }

                    if ((entry.Flags & Constants.PckFileDeleted) != 0)
                    {
                        Console.WriteLine($"Pck file is marked as removed (but still processing it): {entry.Path}");
                    }
                }

                entry.GetData = () => ReadContainedFileContents(entry.Offset, entry.Size);

                if (IncludeFilter != null && !IncludeFilter(entry))
                {
                    excluded++;
                    continue;
                }

                Contents[entry.Path] = entry;
            }

            if (excluded > 0)
            {
                Console.WriteLine($"{Path} files excluded by filters: {excluded}");
            }

            return true;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"ERROR: {ex.Message}");

            if (throwOnError)
                throw;

            return false;
        }
        finally
        {
            // We keep _readStream open for ReadContainedFileContents
        }
    }

    public void UseFileFilter(FileFilter filter)
    {
        IncludeFilter = filter.Include;
    }

    public byte[] ReadContainedFileContents(ulong offset, ulong size)
    {
        if (_readStream == null)
        {
            throw new Exception("Data reader is no longer open to read file contents");
        }

        lock (_readStream)
        {
            _readStream.Seek((long)offset, SeekOrigin.Begin);
            byte[] data = new byte[size];
            int read = _readStream.Read(data, 0, (int)size);
            if ((ulong)read != size)
            {
                throw new Exception(
                    "reading file entry content failed (specified offset or data length is too large, pck may be corrupt or malformed)");
            }

            return data;
        }
    }

    public bool Save()
    {
        if (FormatVersion > Constants.MaxSupportedPckVersionSave)
        {
            Console.WriteLine($"ERROR: cannot save pck version: {FormatVersion}");
            return false;
        }

        string tmpWrite = Path + ".write";

        try
        {
            using (var writeStream = File.Open(tmpWrite, FileMode.Create, FileAccess.Write, FileShare.None))
            using (var writer = new BinaryWriter(writeStream, Encoding.UTF8, leaveOpen: false))
            {
                if (FormatVersion >= 2 && Alignment < 1)
                {
                    Alignment = 32;
                }

                // Header
                writer.Write(Constants.PckHeaderMagic);
                writer.Write(FormatVersion);

                // Godot version
                writer.Write(MajorGodotVersion);
                writer.Write(MinorGodotVersion);
                writer.Write(PatchGodotVersion);

                long baseOffsetLocation = 0;
                long directoryOffsetLocation = 0;

                bool useRelativeOffset = (Flags & Constants.PckFileRelativeBase) != 0 || FormatVersion >= 3;

                if (FormatVersion >= 2)
                {
                    uint flags = Flags;
                    if (useRelativeOffset)
                    {
                        flags |= Constants.PckFileRelativeBase;
                    }

                    writer.Write(flags);

                    baseOffsetLocation = writeStream.Position;
                    writer.Write((ulong)0);

                    if (FormatVersion >= 3)
                    {
                        directoryOffsetLocation = writeStream.Position;
                        writer.Write((ulong)0);
                    }
                }

                // Reserved
                if (FormatVersion >= 4 && (Flags & Constants.PckFileSparseBundle) != 0 &&
                    (Flags & Constants.PackDirEncrypted) != 0 && Salt.Length == 32)
                {
                    writer.Write(Encoding.UTF8.GetBytes(Salt));
                    for (int i = 0; i < 8; i++) writer.Write((uint)0);
                }
                else
                {
                    for (int i = 0; i < 16; i++) writer.Write((uint)0);
                }

                long remember = writeStream.Position;

                if (FormatVersion >= 3)
                {
                    writeStream.Seek(directoryOffsetLocation, SeekOrigin.Begin);
                    writer.Write((ulong)remember);
                    writeStream.Seek(remember, SeekOrigin.Begin);
                }

                writer.Write((uint)Contents.Count);

                Dictionary<ContainedFile, long> continueWriteIndex = [];

                foreach (var entry in Contents.Values)
                {
                    byte[] pathBytes = Encoding.UTF8.GetBytes(entry.Path);
                    int pathToWriteSize = pathBytes.Length +
                                          (PadPathsToMultipleWithNulls -
                                           (pathBytes.Length % PadPathsToMultipleWithNulls));
                    int padding = pathToWriteSize - pathBytes.Length;

                    writer.Write((uint)pathToWriteSize);
                    writer.Write(pathBytes);
                    for (int i = 0; i < padding; i++) writer.Write((byte)0);

                    continueWriteIndex[entry] = writeStream.Position;
                    writer.Write((ulong)0); // Offset placeholder
                    writer.Write(entry.Size);
                    writer.Write(entry.Md5); // Placeholder

                    if (FormatVersion >= 2)
                    {
                        writer.Write(entry.Flags);
                    }
                }

                PadToAlignment(writeStream, writer);

                long filesStart = writeStream.Position;

                if (FormatVersion >= 2)
                {
                    writeStream.Seek(baseOffsetLocation, SeekOrigin.Begin);
                    writer.Write((ulong)filesStart);
                    writeStream.Seek(filesStart, SeekOrigin.Begin);
                }

                foreach (var entry in Contents.Values)
                {
                    PadToAlignment(writeStream, writer);

                    ulong offset = (ulong)writeStream.Position;
                    byte[] data = entry.GetData?.Invoke() ?? [];

                    if ((ulong)data.Length != entry.Size)
                    {
                        Console.WriteLine(
                            "ERROR: file entry data source returned different amount of data than the entry said its size is");
                        return false;
                    }

                    writer.Write(data);

                    entry.Md5 = MD5.HashData(data);

                    if (FormatVersion < 2)
                    {
                        entry.Offset = offset;
                    }
                    else
                    {
                        entry.Offset = offset - (ulong)filesStart;
                    }
                }

                foreach (var entry in Contents.Values)
                {
                    writeStream.Seek(continueWriteIndex[entry], SeekOrigin.Begin);
                    writer.Write(entry.Offset);
                    writer.Write(entry.Size);
                    writer.Write(entry.Md5);
                }
            }

            _reader?.Dispose();
            _reader = null;
            _readStream?.Dispose();
            _readStream = null;

            if (File.Exists(Path))
            {
                File.Delete(Path);
            }

            File.Move(tmpWrite, Path);

            return true;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"ERROR: {ex.Message}");
            return false;
        }
    }


    public void AddFile(ContainedFile file)
    {
        if (IncludeFilter != null && !IncludeFilter(file))
            return;

        Contents[file.Path] = file;
    }

    public bool AddFilesFromFilesystem(string path, string stripPrefix, bool printAddedFiles = false)
    {
        if (!Directory.Exists(path) && !File.Exists(path))
            return false;

        if (!Directory.Exists(path))
        {
            AddSingleFile(path, PreparePckPath(path, stripPrefix), printAddedFiles);
            return true;
        }

        foreach (string file in Directory.EnumerateFiles(path, "*", SearchOption.AllDirectories))
        {
            AddSingleFile(file, PreparePckPath(file, stripPrefix), printAddedFiles);
        }

        return true;
    }

    public void AddSingleFile(string filesystemPath, string pckPath, bool printAddedFile = false)
    {
        if (string.IsNullOrEmpty(pckPath))
            throw new Exception("path inside pck is empty to add file to");

        FileInfo info = new(filesystemPath);
        ContainedFile file = new()
        {
            Path = pckPath,
            Offset = ulong.MaxValue,
            Size = (ulong)info.Length,
            GetData = () => File.ReadAllBytes(filesystemPath)
        };

        if (IncludeFilter != null && !IncludeFilter(file))
            return;

        if (printAddedFile)
            Console.WriteLine($"Adding {filesystemPath} as {pckPath}");

        Contents[pckPath] = file;
    }

    public string PreparePckPath(string path, string stripPrefix)
    {
        if (!string.IsNullOrEmpty(stripPrefix) && path.StartsWith(stripPrefix))
        {
            path = path[stripPrefix.Length..];
        }

        path = path.Replace('\\', '/');

        while (path.StartsWith('/'))
        {
            path = path[1..];
        }

        if (NoResPrefix)
            return path;

        return Constants.GodotResPath + path;
    }

    public void ChangePath(string path)
    {
        Path = path;
    }

    public bool Extract(string outputPrefix, bool printExtracted)
    {
        try
        {
            foreach (var entry in Contents.Values)
            {
                string processedPath = entry.Path.StartsWith(Constants.GodotResPath)
                    ? entry.Path[Constants.GodotResPath.Length..]
                    : entry.Path;

                while (processedPath.StartsWith('/'))
                {
                    processedPath = processedPath[1..];
                }

                string targetFile = System.IO.Path.Combine(outputPrefix, processedPath);
                string? targetFolder = System.IO.Path.GetDirectoryName(targetFile);

                if (printExtracted)
                    Console.WriteLine($"Extracting {entry.Path} to {targetFile}");

                if (!string.IsNullOrEmpty(targetFolder))
                {
                    Directory.CreateDirectory(targetFolder);
                }

                byte[] data = entry.GetData?.Invoke() ?? [];
                File.WriteAllBytes(targetFile, data);
            }

            return true;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"ERROR: {ex.Message}");
            return false;
        }
    }

    public void PrintFileList(bool printHashes, bool includeSize = true)
    {
        foreach (var entry in Contents.Values)
        {
            Console.Write(entry.Path);
            if (includeSize)
                Console.Write($" size: {entry.Size}");

            if (printHashes)
            {
                Console.Write($" md5: {Convert.ToHexString(entry.Md5).ToLower()}");
            }

            Console.WriteLine();
        }
    }

    public void SetGodotVersion(uint major, uint minor, uint patch)
    {
        MajorGodotVersion = major;
        MinorGodotVersion = minor;
        PatchGodotVersion = patch;

        if (MajorGodotVersion <= 3)
        {
            FormatVersion = Constants.Godot3PckVersion;
        }
        else if (MajorGodotVersion >= 4)
        {
            FormatVersion = Constants.Godot4PckVersion;

            if (MinorGodotVersion >= 7)
            {
                FormatVersion = Constants.Godot47PckVersion;
            }
            else if (MinorGodotVersion >= 5)
            {
                FormatVersion = Constants.Godot45PckVersion;
            }
        }
    }

    public void Dispose()
    {
        _reader?.Dispose();
        _readStream?.Dispose();
        GC.SuppressFinalize(this);
    }

    private void PadToAlignment(Stream stream, BinaryWriter writer)
    {
        if (Alignment <= 0) return;
        while ((stream.Position % Alignment) != 0)
        {
            writer.Write((byte)0);
        }
    }
}