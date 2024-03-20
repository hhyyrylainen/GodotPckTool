#pragma once

#include "Define.h"

#include "FileFilter.h"

#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <vector>

namespace pcktool {

using json = nlohmann::json;

class PckFile;

//! \brief Main class for the Godot Pck Tool
class PckTool {
    struct FileEntry {
        std::string InputFile;
        std::string Target;
    };

public:
    struct Options {
        std::string Pack;
        std::string Action;
        std::vector<std::string> Files;
        std::string Output;
        std::string RemovePrefix;

        // Godot version to set in pck
        int GodotMajor;
        int GodotMinor;
        int GodotPatch;

        json FileCommands;

        FileFilter Filter;

        bool ReducedVerbosity;
        bool PrintHashes;
    };

public:
    explicit PckTool(Options options);

    //! \brief Runs the tool
    //! \returns The exit code to return
    int Run();

private:
    bool BuildFileList();

    bool TargetExists() const;

    bool RequireTargetFileExists();

    std::unique_ptr<PckFile> LoadPck();

    void SetIncludeFilter(PckFile& pck);

private:
    Options Opts;

    std::vector<FileEntry> Files;
};

} // namespace pcktool
