#pragma once

#include "Define.h"

#include <array>
#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>

namespace pcktool {

// Pck magic
constexpr uint32_t PCK_HEADER_MAGIC = 0x43504447;

// Highest pck version supported by this tool
constexpr int MAX_PCK_FORMAT_VERSION = 1;

//! \brief A single pck file object. Handles reading and writing
//!
//! Probably only works on little endian systems
class PckFile {
public:
    struct ContainedFile {
        std::string Path;
        uint64_t Offset;
        uint64_t Size;
        std::array<uint8_t, 16> MD5 = {0};

        std::function<std::string()> GetData;
    };

public:
    explicit PckFile(const std::string& path);
    PckFile(PckFile&& other) = delete;
    PckFile(const PckFile& other) = delete;

    PckFile& operator=(PckFile&& other) = delete;
    PckFile& operator=(const PckFile& other) = delete;

    bool Load();

    //! \brief Saves the entire pack over the Path file
    bool Save();

    //! \brief Extracts the read contents to the outputPrefix
    bool Extract(const std::string& outputPrefix, bool printExtracted);

    void PrintFileList(bool includeSize = true);

    void AddFile(ContainedFile&& file);

    //! \brief Adds recursively files from path to this pck
    bool AddFilesFromFilesystem(
        const std::string& path, const std::string& stripPrefix, bool printAddedFiles = false);

    void AddSingleFile(const std::string& filesystemPath, const std::string& pckPath,
        bool printAddedFile = false);

    //! \note Automatically converts \'s in the path to /'s
    std::string PreparePckPath(std::string path, const std::string& stripPrefix);

    void ChangePath(const std::string& path);

    std::string ReadContainedFileContents(uint64_t offset, uint64_t size);

    void SetGodotVersion(uint32_t major, uint32_t minor, uint32_t patch)
    {
        MajorGodotVersion = major;
        MinorGodotVersion = minor;
        PatchGodotVersion = patch;
    }

    //! \brief Sets a filter for entries to be added to this object
    //!
    //! This must be set before loading the data. The Save method doesn't apply the filter.
    void SetIncludeFilter(std::function<bool(const ContainedFile&)> callback)
    {
        IncludeFilter = std::move(callback);
    }

    inline const auto& GetPath()
    {
        return Path;
    }

private:
    // These need swaps on non little endian machine
    uint32_t Read32();
    uint64_t Read64();
    void Write32(uint32_t value);
    void Write64(uint64_t value);

private:
    std::string Path;
    std::optional<std::fstream> File;
    std::optional<std::ifstream> DataReader;

    // PCK Format version number
    // 0 = Godot 1.x, 2.x
    // 1 = Godot 3.x
    // 2 = Godot 4.x
    uint32_t FormatVersion = 0;

    // Loaded version info from a pack
    uint32_t MajorGodotVersion = 0;
    uint32_t MinorGodotVersion = 0;
    uint32_t PatchGodotVersion = 0;

    //! Add trailing null bytes to the length of a path until it is a multiple of this size
    size_t PadPathsToMultipleWithNULLS = 4;

    std::map<std::string, ContainedFile> Contents;

    //! Used in a bunch of operations to check if a file entry should be included or ignored
    std::function<bool(const ContainedFile&)> IncludeFilter;
};

} // namespace pcktool
