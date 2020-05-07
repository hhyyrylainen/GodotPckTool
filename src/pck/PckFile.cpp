// ------------------------------------ //
#include "PckFile.h"

#include <filesystem>
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

    // Separate reader to make writing work
    DataReader.open(Path, std::ios::in);

    uint32_t magic = Read32();

    if(magic != PCK_HEADER_MAGIC) {
        std::cout << "ERROR: invalid magic number\n";
        return false;
    }

    uint32_t version = Read32();

    // Godot engine version, we don't care about these
    MajorGodotVersion = Read32();
    MinorGodotVersion = Read32();
    PatchGodotVersion = Read32();

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

        entry.GetData = [offset = entry.Offset, size = entry.Size, this]() {
            return ReadContainedFileContents(offset, size);
        };

        Contents[entry.Path] = std::move(entry);
    };

    File.close();
    return true;
}
// ------------------------------------ //
bool PckFile::Save()
{
    const auto tmpWrite = Path + ".write";

    File.open(tmpWrite, std::ios::trunc | std::ios::out);

    if(!File.good()) {
        std::cout << "ERROR: file is unwriteable: " << tmpWrite << "\n";
        return false;
    }

    File.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    // Header

    Write32(PCK_HEADER_MAGIC);
    Write32(PCK_FORMAT_VERSION);

    // Godot version

    Write32(MajorGodotVersion);
    Write32(MinorGodotVersion);
    Write32(PatchGodotVersion);

    // Reserved part
    for(int i = 0; i < 16; i++) {
        Write32(0);
    }

    // File count
    Write32(Contents.size());

    std::map<const ContainedFile*, std::fstream::pos_type> continueWriteIndex;

    // First write blank file entries as placeholders
    for(const auto& [_, entry] : Contents) {

        Write32(entry.Path.size());
        File.write(entry.Path.data(), entry.Path.size());

        continueWriteIndex[&entry] = File.tellg();
        // No offset yet
        Write64(0);
        Write64(entry.Size);

        // MD5 is not calculated yet, so this writes garbage but this will be fixed later
        // Apparently this is not needed at all as the godot packer never calculates the md5
        File.write(reinterpret_cast<const char*>(entry.MD5.data()), sizeof(entry.MD5));
    }

    // Align
    // Has to be at least 4 when used
    int alignment = 0;

    if(alignment >= 4) {
        while(File.tellg() % alignment != 0)
            Write32(0);
    }

    // Then write the data
    for(auto& [_, entry] : Contents) {
        uint64_t offset = File.tellg();

        // Write the data here
        // NOTE: the godot packer only writes like 50k bytes at once, but we load the whole
        // thing to memory
        const auto data = entry.GetData();

        if(data.size() != entry.Size) {
            std::cout << "ERROR: file entry data source returned different amount of data "
                      << "than the entry said its size is";
            return false;
        }

        File.write(data.data(), entry.Size);

        // Update the entry offset for writing
        entry.Offset = offset;
    }


    // And finally fix up the files
    for(const auto& [_, entry] : Contents) {
        File.seekg(continueWriteIndex[&entry]);
        Write64(entry.Offset);

        // MD5 is not calculated so no need to write that here
    }

    File.close();
    if(DataReader.is_open())
        DataReader.close();

    std::filesystem::remove(Path);
    std::filesystem::rename(tmpWrite, Path);
    return true;
}
// ------------------------------------ //
void PckFile::AddFile(ContainedFile&& file)
{
    Contents[file.Path] = std::move(file);
}

void PckFile::ChangePath(const std::string& path)
{
    if(File.is_open())
        File.close();
    Path = path;
}
// ------------------------------------ //
void PckFile::PrintFileList(bool includeSize /*= true*/)
{
    for(const auto& [path, entry] : Contents) {
        std::cout << path;
        if(includeSize)
            std::cout << " size: " << entry.Size;

        std::cout << "\n";
    }
}
// ------------------------------------ //
std::string PckFile::ReadContainedFileContents(uint64_t offset, uint64_t size)
{
    if(!DataReader.is_open())
        DataReader.open(Path, std::ios::in);

    std::string result;
    result.resize(size);

    DataReader.seekg(offset);
    DataReader.read(result.data(), size);

    if(!DataReader.good())
        throw std::runtime_error("files for entry contents failed");

    return result;
}
// ------------------------------------ //
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

void PckFile::Write32(uint32_t value)
{
    File.write(reinterpret_cast<char*>(&value), sizeof(value));
}

void PckFile::Write64(uint64_t value)
{
    File.write(reinterpret_cast<char*>(&value), sizeof(value));
}
