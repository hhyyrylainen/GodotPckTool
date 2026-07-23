namespace GodotPckTool.CSharp.Tests;

using System.Text;
using Xunit;
using Xunit.Abstractions;

public sealed class PckFileTests : IDisposable
{
    /// <summary>
    ///   Path to a real PCK file in the user's home folder for read test. If missing, it is just ignored.
    /// </summary>
    private const string RealTestPckFile = "Projects/Thrive/builds/Thrive_1.2.0.0-alpha_linux_x11/Thrive.pck";

    private readonly ITestOutputHelper _testOutputHelper;
    private readonly string _testDir;
    private readonly string _pckPath;

    public PckFileTests(ITestOutputHelper testOutputHelper)
    {
        _testOutputHelper = testOutputHelper;
        _testDir = Path.Combine(Path.GetTempPath(), "GodotPckToolTests_" + Guid.NewGuid());
        Directory.CreateDirectory(_testDir);
        _pckPath = Path.Combine(_testDir, "test.pck");
    }

    public void Dispose()
    {
        if (Directory.Exists(_testDir))
        {
            Directory.Delete(_testDir, true);
        }
    }

    [Fact]
    public void TestCreateAndReadPck()
    {
        // 1. Create
        using (var pck = new PckFile(_pckPath))
        {
            pck.SetGodotVersion(4, 3, 0);

            var fileData = Encoding.UTF8.GetBytes("Hello Godot!");
            var entry = new ContainedFile
            {
                Path = "res://hello.txt",
                Size = (ulong)fileData.Length,
                GetData = () => fileData,
            };
            pck.AddFile(entry);

            Assert.True(pck.Save());
        }

        Assert.True(File.Exists(_pckPath));

        // 2. Read
        using (var pck = new PckFile(_pckPath))
        {
            Assert.True(pck.Load());
            Assert.Equal(4u, pck.MajorGodotVersion);
            Assert.Single(pck.Contents);
            Assert.True(pck.Contents.ContainsKey("res://hello.txt"));

            var entry = pck.Contents["res://hello.txt"];
            Assert.Equal((ulong)Encoding.UTF8.GetByteCount("Hello Godot!"), entry.Size);

            var data = entry.GetData?.Invoke();
            Assert.NotNull(data);
            Assert.Equal("Hello Godot!", Encoding.UTF8.GetString(data));
        }
    }

    [Fact]
    public void TestPackAndExtractFolder()
    {
        string sourceDir = Path.Combine(_testDir, "source");
        Directory.CreateDirectory(sourceDir);
        File.WriteAllText(Path.Combine(sourceDir, "file1.txt"), "Content 1");
        Directory.CreateDirectory(Path.Combine(sourceDir, "sub"));
        File.WriteAllText(Path.Combine(sourceDir, "sub", "file2.txt"), "Content 2");

        // 3. Pack up a folder
        using (var pck = new PckFile(_pckPath))
        {
            pck.SetGodotVersion(4, 3, 0);
            Assert.True(pck.AddFilesFromFilesystem(sourceDir, sourceDir));
            Assert.True(pck.Save());
        }

        // 4. Extract the folder
        string extractDir = Path.Combine(_testDir, "extracted");
        using (var pck = new PckFile(_pckPath))
        {
            Assert.True(pck.Load());
            Assert.True(pck.Extract(extractDir, false));
        }

        Assert.True(File.Exists(Path.Combine(extractDir, "file1.txt")));
        Assert.True(File.Exists(Path.Combine(extractDir, "sub", "file2.txt")));
        Assert.Equal("Content 1", File.ReadAllText(Path.Combine(extractDir, "file1.txt")));
        Assert.Equal("Content 2", File.ReadAllText(Path.Combine(extractDir, "sub", "file2.txt")));
    }

    [Fact]
    public void TestReadRealPckFile()
    {
        // This test reads the contents of a real PCK file that is assumed to exist at path RealTestPckFile.
        // It can be skipped when the file doesn't exist as it is not going to be included in this repo.

        string home = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        string realPckPath = Path.Combine(home, RealTestPckFile);

        if (!File.Exists(realPckPath))
        {
            // Skip test if file doesn't exist
            _testOutputHelper.WriteLine($"Skipping test as PCK file doesn't exist: {realPckPath}");
            return;
        }

        using var pck = new PckFile(realPckPath);
        Assert.True(pck.Load());
        Assert.NotEmpty(pck.Contents);

        // Print some info to confirm it works
        _testOutputHelper.WriteLine($"Loaded real PCK: {realPckPath}");
        _testOutputHelper.WriteLine(
            $"Godot Version: {pck.MajorGodotVersion}.{pck.MinorGodotVersion}.{pck.PatchGodotVersion}");
        _testOutputHelper.WriteLine($"File count: {pck.Contents.Count}");
    }
}
