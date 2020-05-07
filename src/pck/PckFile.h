#pragma once

#include "Define.h"

#include <array>
#include <fstream>
#include <functional>
#include <map>
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
        std::array<uint8_t, 16> MD5;

        std::function<std::string()> GetData;
    };

public:
    PckFile(const std::string& path);

    bool Load();

    bool Save();

    void PrintFileList(bool includeSize = true);

    std::string ReadContainedFileContents(uint64_t offset, uint64_t size);

private:
    // These need swaps on non little endian machine
    uint32_t Read32();
    uint64_t Read64();

private:
    std::string Path;
    std::fstream File;

    std::map<std::string, ContainedFile> Contents;
};

} // namespace pcktool
