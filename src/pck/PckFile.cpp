// ------------------------------------ //
#include "PckFile.h"

#include <cstring>
#include <filesystem>
#include <iostream>
#include <utility>

#include "md5.h"

static_assert(MD5_SIZE == 16, "MD5 size changed");
// ------------------------------------ //
using namespace pcktool;

PckFile::PckFile(std::string path) : Path(std::move(path)) {}
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

    const auto pckStart = DataReader->tellg();

    uint32_t magic = Read32();

    if(magic != PCK_HEADER_MAGIC) {
        std::cout << "ERROR: invalid magic number\n";
        return false;
    }

    FormatVersion = Read32();

    // Godot engine version, we don't care about these
    MajorGodotVersion = Read32();
    MinorGodotVersion = Read32();
    PatchGodotVersion = Read32();

    if(FormatVersion > MAX_SUPPORTED_PCK_VERSION_LOAD) {
        std::cout << "ERROR: pck is unsupported version: " << FormatVersion << "\n";
        return false;
    }

    if(FormatVersion >= 2) {
        Flags = Read32();
        FileOffsetBase = Read64();
    }

    if(Flags & PACK_DIR_ENCRYPTED) {
        std::cout << "ERROR: pck is encrypted\n";
        return false;
    }

    if(Flags & PCK_FILE_SPARSE_BUNDLE) {
        std::cout << "Warning: Sparse pck detected, this is unlikely to work!\n";
    }

    if(FormatVersion >= 3 || (FormatVersion == 2 && Flags & PCK_FILE_RELATIVE_BASE)) {
        FileOffsetBase += pckStart;
    }

    if(FormatVersion >= 3) {
        // New feature: offset to the directory
        DirectoryOffset = Read64();

        // Seek to the directory to keep the following logic the same (this skips the reserved
        // part of the header)
        File->seekg(pckStart + static_cast<std::streampos>(
                                   DirectoryOffset)); // NOLINT(*-narrowing-conversions)
    } else {
        // V2 has the directory immediately after the header
        // Reserved
        for(int i = 0; i < 16; i++) {
            Read32();
        }
    }


    // Now we are at the file directory section
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

        entry.Offset = FileOffsetBase + Read64();
        entry.Size = Read64();

        File->read(reinterpret_cast<char*>(entry.MD5.data()), sizeof(entry.MD5));

        if(FormatVersion >= 2) {
            entry.Flags = Read32();

            if(entry.Flags & PCK_FILE_ENCRYPTED) {
                std::cout << "WARNING: pck file (" << entry.Path
                          << ") is marked as encrypted, decoding the encryption is not "
                             "implemented\n";
            }

            if(entry.Flags & PCK_FILE_DELETED) {
                std::cout << "Pck file is marked as removed (but still processing it): "
                          << entry.Path << "\n";
            }
        }

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
    if(FormatVersion > MAX_SUPPORTED_PCK_VERSION_SAVE) {
        std::cout << "ERROR: cannot save pck version: " << FormatVersion << "\n";
        return false;
    }

    const auto tmpWrite = Path + ".write";

    File = std::fstream(tmpWrite, std::ios::trunc | std::ios::out | std::ios::binary);

    if(!File->good()) {
        std::cout << "ERROR: file is unwritable: " << tmpWrite << "\n";
        return false;
    }

    // Alignment is used with Godot 4 .pck files
    if(FormatVersion >= 2 && Alignment < 1) {
        Alignment = 32;
    }

    File->exceptions(std::ifstream::failbit | std::ifstream::badbit);

    // Header

    Write32(PCK_HEADER_MAGIC);
    Write32(FormatVersion);

    // Godot version

    Write32(MajorGodotVersion);
    Write32(MinorGodotVersion);
    Write32(PatchGodotVersion);

    std::fstream::off_type baseOffsetLocation = 0;
    std::fstream::off_type directoryOffsetLocation = 0;

    const bool useRelativeOffset = Flags & PCK_FILE_RELATIVE_BASE || FormatVersion >= 3;

    if(FormatVersion >= 2) {

        // Pck flags
        uint32_t flags = 0;

        if(useRelativeOffset) {
            flags |= PCK_FILE_RELATIVE_BASE;
        }

        Write32(flags);

        // File entry base offset (dummy value for now)
        baseOffsetLocation = File->tellg();
        Write64(0);

        if(FormatVersion >= 3) {
            // Offset to the directory
            directoryOffsetLocation = File->tellg();
            Write64(0);
        }
    }

    // Reserved part
    for(int i = 0; i < 16; ++i) {
        Write32(0);
    }

    // In Godot 4.5 the directory is written at the end of the file, but that is not an
    // absolutely mandatory requirement, so we can keep writing the directory here before the
    // file data

    const auto remember = File->tellg();

    if(FormatVersion >= 3) {
        // Write start of the directory variable
        File->seekg(directoryOffsetLocation);
        Write64(remember);
    }

    File->seekg(remember);

    // Things are filtered before adding to Contents, so we don't do any filtering here
    // File count
    Write32(Contents.size());

    std::map<const ContainedFile*, std::fstream::pos_type> continueWriteIndex;

    // First, write blank file entries as placeholders
    for(const auto& [_, entry] : Contents) {

        // When the path is exactly the right size, this results in 4 extra NULLs
        // but that seems to be what Godot itself also does
        const size_t pathToWriteSize =
            entry.Path.size() +
            (PadPathsToMultipleWithNULLS - (entry.Path.size() % PadPathsToMultipleWithNULLS));
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
        File->write(reinterpret_cast<const char*>(entry.MD5.data()), sizeof(entry.MD5));

        if(FormatVersion >= 2) {
            Write32(entry.Flags);
        }
    }

    // Align
    PadToAlignment();

    const auto filesStart = File->tellg();


    if(FormatVersion >= 2) {

        // Set a valid offset now for the files start.
        // Even with useRelativeOffset we don't need to adjust here as we always write the
        // header at offset 0 of the file.
        File->seekg(baseOffsetLocation);
        Write64(filesStart);
    }

    File->seekg(filesStart);

    // Then write the data
    for(auto& [_, entry] : Contents) {
        // Pad file data to the alignment (doing it here ensures it is correct for the first
        // file as well)
        PadToAlignment();

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

        File->write(data.data(), entry.Size); // NOLINT(*-narrowing-conversions)

        // Update MD5
        // The md5 library assumes the write target size here
        static_assert(sizeof(entry.MD5) == MD5_SIZE);
        md5::md5_t(data.data(), data.size(), entry.MD5.data());

        // Update the entry offset for writing
        if(FormatVersion < 2) {

            entry.Offset = offset;
        } else {
            // Offsets are now relative to the files block
            entry.Offset = offset - filesStart;
        }
    }

    // Non-embedded pck doesn't have to be aligned up to end at Alignment size
    // PadToAlignment();

    // And finally fix up the file headers
    for(const auto& [_, entry] : Contents) {
        File->seekg(continueWriteIndex[&entry]);
        Write64(entry.Offset);

        // Don't need to write this again, but for simplicity this can be written again to
        // align the hash to the right spot
        Write64(entry.Size);

        // Update MD5 now that it is calculated
        File->write(reinterpret_cast<const char*>(entry.MD5.data()), sizeof(entry.MD5));
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
bool PckFile::AddFilesFromFilesystem(
    const std::string& path, const std::string& stripPrefix, bool printAddedFiles /*= false*/)
{
    if(!std::filesystem::exists(path)) {
        return false;
    }

    if(!std::filesystem::is_directory(path)) {
        // Just a single file
        AddSingleFile(path, PreparePckPath(path, stripPrefix), printAddedFiles);
        return true;
    }

    for(const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if(entry.is_directory())
            continue;

        const std::string entryPath = entry.path().string();

        AddSingleFile(entryPath, PreparePckPath(entryPath, stripPrefix), printAddedFiles);
    }

    return true;
}

void PckFile::AddSingleFile(const std::string& filesystemPath, const std::string& pckPath,
    bool printAddedFile /*= false*/)
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

    if(printAddedFile)
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
bool PckFile::Extract(const std::string& outputPrefix, bool printExtracted)
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

        if(printExtracted)
            std::cout << "Extracting " << path << " to " << targetFile << "\n";

        if(!targetFolder.empty()) {
            try {
                std::filesystem::create_directories(targetFolder);
            } catch(const std::filesystem::filesystem_error& e) {
                std::cout << "ERROR: creating target directory (" << targetFolder
                          << "): " << e.what() << "\n";
                return false;
            }
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
void PckFile::PrintFileList(bool printHashes, bool includeSize /*= true*/)
{
    char signatureTempBuffer[MD5_STRING_SIZE];

    for(const auto& [path, entry] : Contents) {
        std::cout << path;
        if(includeSize)
            std::cout << " size: " << entry.Size;

        // TODO: flag to also verify the hashes?
        if(printHashes) {
            static_assert(sizeof(entry.MD5) == MD5_SIZE);
            md5::sig_to_string(entry.MD5.data(), signatureTempBuffer, MD5_STRING_SIZE);

            std::cout << " md5: "
                      << std::string_view(
                             signatureTempBuffer, std::strlen(signatureTempBuffer));
        }

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

    if(!DataReader->good()) {
        throw std::runtime_error("reading file entry content failed (specified offset or data "
                                 "length is too large, pck may be corrupt or malformed)");
    }

    return result;
}
// ------------------------------------ //
void PckFile::SetGodotVersion(uint32_t major, uint32_t minor, uint32_t patch)
{
    MajorGodotVersion = major;
    MinorGodotVersion = minor;
    PatchGodotVersion = patch;

    if(MajorGodotVersion <= 3) {

        FormatVersion = GODOT_3_PCK_VERSION;
    } else if(MajorGodotVersion >= 4) {

        FormatVersion = GODOT_4_PCK_VERSION;

        if(MinorGodotVersion >= 5)
            FormatVersion = GODOT_4_5_PCK_VERSION;
    }
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

void PckFile::PadToAlignment()
{
    if(Alignment <= 0)
        return;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"
    while((File->tellg() % Alignment) != 0) {
        uint8_t null = 0;
        File->write(reinterpret_cast<const char*>(&null), sizeof(null));
    }
#pragma clang diagnostic pop
}
