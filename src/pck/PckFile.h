#pragma once

#include "Define.h"

#include <array>
#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <string>

namespace pcktool {

// Pck magic
constexpr uint32_t PCK_HEADER_MAGIC = 0x43504447;

// Pck version
constexpr int PCK_FORMAT_VERSION = 1;

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
    PckFile(const std::string& path);
    PckFile(PckFile&& other) = delete;
    PckFile(const PckFile& other) = delete;

    PckFile& operator=(PckFile&& other) = delete;
    PckFile& operator=(const PckFile& other) = delete;

    bool Load();

    //! \brief Saves the entire pack over the Path file
    bool Save();

    //! \brief Extracts the read contents to the outputPrefix
    bool Extract(const std::string& outputPrefix);

    void PrintFileList(bool includeSize = true);

    void AddFile(ContainedFile&& file);

    void ChangePath(const std::string& path);

    std::string ReadContainedFileContents(uint64_t offset, uint64_t size);

    void SetGodotVersion(uint32_t major, uint32_t minor, uint32_t patch)
    {
        MajorGodotVersion = major;
        MinorGodotVersion = minor;
        PatchGodotVersion = patch;
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

    // Loaded version info from a pack
    uint32_t MajorGodotVersion = 0;
    uint32_t MinorGodotVersion = 0;
    uint32_t PatchGodotVersion = 0;

    std::map<std::string, ContainedFile> Contents;
};

} // namespace pcktool
