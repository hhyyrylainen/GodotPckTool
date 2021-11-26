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

    File = std::fstream(Path, std::ios::in | std::ios::binary);

    if(!File->good()) {
        std::cout << "ERROR: file is unreadable: " << Path << "\n";
        return false;
    }

    File->exceptions(std::ifstream::failbit | std::ifstream::badbit);

    // Separate reader to make writing work
    DataReader = std::ifstream(Path, std::ios::in | std::ios::binary);

    if(!DataReader || !DataReader->good())
        throw std::runtime_error("second data reader opening failed");

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

    size_t excluded = 0;

    for(uint32_t i = 0; i < files; i++) {

        const auto pathLength = Read32();

        ContainedFile entry;

        entry.Path.resize(pathLength);

        File->read(entry.Path.data(), pathLength);

        // Remove trailing null bytes
        while(!entry.Path.empty() && entry.Path.back() == '\0')
            entry.Path.pop_back();

        entry.Offset = Read64();
        entry.Size = Read64();

        File->read(reinterpret_cast<char*>(entry.MD5.data()), sizeof(entry.MD5));

        entry.GetData = [offset = entry.Offset, size = entry.Size, this]() {
            return ReadContainedFileContents(offset, size);
        };

        if(IncludeFilter && !IncludeFilter(entry)) {
            ++excluded;
            continue;
        }

        Contents[entry.Path] = std::move(entry);
    }

    File->close();
    File.reset();

    if(excluded)
        std::cout << Path << " files excluded by filters: " << excluded << "\n";

    return true;
}
// ------------------------------------ //
bool PckFile::Save()
{
    const auto tmpWrite = Path + ".write";

    File = std::fstream(tmpWrite, std::ios::trunc | std::ios::out | std::ios::binary);

    if(!File->good()) {
        std::cout << "ERROR: file is unwritable: " << tmpWrite << "\n";
        return false;
    }

    File->exceptions(std::ifstream::failbit | std::ifstream::badbit);

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

    // Things are filtered before adding to Contents, so we don't do any filtering here
    // File count
    Write32(Contents.size());

    std::map<const ContainedFile*, std::fstream::pos_type> continueWriteIndex;

    // First write blank file entries as placeholders
    for(const auto& [_, entry] : Contents) {

        const size_t pathToWriteSize =
            entry.Path.size() + (entry.Path.size() % PadPathsToMultipleWithNULLS);
        const size_t padding = pathToWriteSize - entry.Path.size();

        Write32(pathToWriteSize);
        File->write(entry.Path.data(), entry.Path.size());

        // Padding null bytes
        for(size_t i = 0; i < padding; ++i) {
            char null = '\0';
            File->write(&null, 1);
        }

        continueWriteIndex[&entry] = File->tellg();
        // No offset yet
        Write64(0);
        Write64(entry.Size);

        // MD5 is not calculated yet, so this writes garbage but this will be fixed later
        // Apparently this is not needed at all as the godot packer never calculates the md5
        File->write(reinterpret_cast<const char*>(entry.MD5.data()), sizeof(entry.MD5));
    }

    // Align
    int alignment = 0;

    // TODO: add command line flag to enable alignment

    if(alignment >= 4) {
        while(File->tellg() % alignment != 0) {
            uint8_t null = 0;
            File->write(reinterpret_cast<const char*>(&null), sizeof(null));
        }
    }

    // Then write the data
    for(auto& [_, entry] : Contents) {
        uint64_t offset = File->tellg();

        // Write the data here
        // NOTE: the godot packer only writes like 50k bytes at once, but we load the whole
        // thing to memory
        const auto data = entry.GetData();

        if(data.size() != entry.Size) {
            std::cout << "ERROR: file entry data source returned different amount of data "
                      << "than the entry said its size is";
            return false;
        }

        File->write(data.data(), entry.Size);

        // Update the entry offset for writing
        entry.Offset = offset;
    }


    // And finally fix up the files
    for(const auto& [_, entry] : Contents) {
        File->seekg(continueWriteIndex[&entry]);
        Write64(entry.Offset);

        // MD5 is not calculated so no need to write that here
    }

    File->close();
    File.reset();
    DataReader.reset();

    try {
        std::filesystem::remove(Path);
    } catch(const std::filesystem::filesystem_error&) {
    }

    std::filesystem::rename(tmpWrite, Path);
    return true;
}
// ------------------------------------ //
void PckFile::AddFile(ContainedFile&& file)
{
    if(IncludeFilter && !IncludeFilter(file))
        return;

    Contents[file.Path] = std::move(file);
}
// ------------------------------------ //
bool PckFile::AddFilesFromFilesystem(const std::string& path, const std::string& stripPrefix)
{
    if(!std::filesystem::exists(path)) {
        return false;
    }

    if(!std::filesystem::is_directory(path)) {
        // Just a single file
        AddSingleFile(path, PreparePckPath(path, stripPrefix));
        return true;
    }

    for(const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if(entry.is_directory())
            continue;

        const std::string entryPath = entry.path().string();

        AddSingleFile(entryPath, PreparePckPath(entryPath, stripPrefix));
    }

    return true;
}

void PckFile::AddSingleFile(const std::string& filesystemPath, std::string pckPath)
{
    if(pckPath.empty())
        throw std::runtime_error("path inside pck is empty to add file to");

    ContainedFile file;

    const auto size = std::filesystem::file_size(filesystemPath);

    file.Path = pckPath;
    file.Offset = -1;
    file.Size = size;

    // Seems like Godot pcks don't currently use hashes, so we don't need to do this
    // file.MD5 = {0};

    file.GetData = [filesystemPath, size]() {
        std::ifstream reader(filesystemPath, std::ios::in | std::ios::binary);

        if(!reader.good()) {
            std::cout << "ERROR: opening for reading: " << filesystemPath << "\n";
            throw std::runtime_error("can't read source file");
        }

        std::string data;
        data.resize(size);

        reader.read(data.data(), size);

        return data;
    };

    // TODO: might be nice to add an option to print out rejected files
    if(IncludeFilter && !IncludeFilter(file))
        return;

    std::cout << "Adding " << filesystemPath << " as " << pckPath << "\n";

    Contents[pckPath] = file;
}

std::string PckFile::PreparePckPath(std::string path, const std::string& stripPrefix)
{
    if(stripPrefix.size() > 0 && path.find(stripPrefix) == 0) {
        path = path.substr(stripPrefix.size());
    }

    // Fix Windows paths to make this work on Windows
    for(size_t i = 0; i < path.size(); ++i) {
        if(path[i] == '\\')
            path[i] = '/';
    }

    while(path.size() > 0 && path.front() == '/') {
        path.erase(path.begin());
    }

    return GODOT_RES_PATH + path;
}
// ------------------------------------ //
void PckFile::ChangePath(const std::string& path)
{
    File.reset();
    // Actually needed for reading previous data
    // DataReader.reset();

    Path = path;
}
// ------------------------------------ //
bool PckFile::Extract(const std::string& outputPrefix)
{
    const auto outputBase = std::filesystem::path(outputPrefix);

    for(const auto& [path, entry] : Contents) {
        std::string processedPath = path.find(GODOT_RES_PATH) == 0 ?
                                        path.substr(std::string_view(GODOT_RES_PATH).size()) :
                                        path;

        // Remove any starting slashes
        while(processedPath.size() > 0 && processedPath.front() == '/')
            processedPath.erase(processedPath.begin());

        const auto fsPath = std::filesystem::path(processedPath);

        const auto filename = fsPath.filename();
        const auto baseFolder = fsPath.parent_path();

        const auto targetFolder = outputBase / baseFolder;
        const auto targetFile = targetFolder / filename;

        std::cout << "Extracting " << path << " to " << targetFile << "\n";
        try {
            std::filesystem::create_directories(targetFolder);
        } catch(const std::filesystem::filesystem_error& e) {
            std::cout << "ERROR: creating target directory (" << targetFolder
                      << "): " << e.what() << "\n";
            return false;
        }

        std::ofstream writer(
            targetFile.string(), std::ios::trunc | std::ios::out | std::ios::binary);

        if(!writer.good()) {
            std::cout << "ERROR: opening file for writing: " << targetFile << "\n";
            return false;
        }

        const auto data = entry.GetData();

        writer.write(data.data(), data.size());

        if(!writer.good()) {
            std::cout << "ERROR: writing failure to file\n";
            return false;
        }
    }

    return true;
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
    if(!DataReader) {
        throw std::runtime_error("Data reader is no longer open to read file contents");
    }

    std::string result;
    result.resize(size);

    DataReader->seekg(offset);
    DataReader->read(result.data(), size);

    if(!DataReader->good())
        throw std::runtime_error("files for entry contents failed");

    return result;
}
// ------------------------------------ //
uint32_t PckFile::Read32()
{
    uint32_t value;

    File->read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

uint64_t PckFile::Read64()
{
    uint64_t value;

    File->read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

void PckFile::Write32(uint32_t value)
{
    File->write(reinterpret_cast<char*>(&value), sizeof(value));
}

void PckFile::Write64(uint64_t value)
{
    File->write(reinterpret_cast<char*>(&value), sizeof(value));
}
