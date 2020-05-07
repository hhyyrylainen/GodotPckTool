// ------------------------------------ //
#include "PckFile.h"

#include <iostream>

using namespace pcktool;
// ------------------------------------ //
PckFile::PckFile(const std::string& path) : Path(path) {}
// ------------------------------------ //
bool PckFile::Load()
{
    Contents.clear();

    File.open(Path, std::ios::in);

    if(!File.good()) {
        std::cout << "ERROR: file is unreadable: " << Path << "\n";
        return false;
    }

    File.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    uint32_t magic = Read32();

    if(magic != PCK_HEADER_MAGIC) {
        std::cout << "ERROR: invalid magic number\n";
        return false;
    }

    uint32_t version = Read32();

    // Godot engine version, we don't care about these
    uint32_t ver_major = Read32();
    uint32_t ver_minor = Read32();
    Read32();

    if(version != PCK_FORMAT_VERSION) {
        std::cout << "ERROR: pck is unsupported version: " << version << "\n";
        return false;
    }

    // Reserved
    for(int i = 0; i < 16; i++) {
        Read32();
    }

    // Now we are at the file section
    const auto files = Read32();

    for(uint32_t i = 0; i < files; i++) {

        const auto pathLength = Read32();

        ContainedFile entry;

        entry.Path.resize(pathLength);

        File.read(entry.Path.data(), pathLength);

        entry.Offset = Read64();
        entry.Size = Read64();

        File.read(reinterpret_cast<char*>(entry.MD5.data()), sizeof(entry.MD5));

        Contents[entry.Path] = std::move(entry);
    };

    return true;
}
// ------------------------------------ //
bool PckFile::Save()
{
    return false;
}
// ------------------------------------ //
void PckFile::PrintFileList(bool includeSize /*= true*/)
{
    for(const auto [path, entry] : Contents) {
        std::cout << path;
        if(includeSize)
            std::cout << " size: " << entry.Size;

        std::cout << "\n";
    }
}
// ------------------------------------ //
std::string PckFile::ReadContainedFileContents(uint64_t offset, uint64_t size)
{
    if(!File.is_open())
        File.open(Path, std::ios::in);

    std::string result;
    result.resize(size);

    File.seekg(offset);
    File.read(result.data(), size);

    if(!File.good())
        throw std::runtime_error("files for entry contents failed");

    return result;
}

uint32_t PckFile::Read32()
{
    uint32_t value;

    File.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

uint64_t PckFile::Read64()
{
    uint64_t value;

    File.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}
