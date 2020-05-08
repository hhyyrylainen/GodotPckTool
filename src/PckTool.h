#pragma once

#include "Define.h"

#include <memory>
#include <string>
#include <vector>

namespace pcktool {

class PckFile;

//! \brief Main class for the Godot Pck Tool
class PckTool {
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
    };

public:
    PckTool(const Options& options);

    //! \brief Runs the tool
    //! \returns The exit code to return
    int Run();

private:
    bool RequireTargetFileExists();

    std::unique_ptr<PckFile> LoadPck();

private:
    Options Opts;
};

} // namespace pcktool
