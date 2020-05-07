#pragma once

#include "Define.h"

#include <string>
#include <vector>

namespace pcktool {

//! \brief Main class for the Godot Pck Tool
class PckTool {
public:
    struct Options {
        std::string Pack;
        std::string Action;
        std::vector<std::string> Files;
    };

public:
    PckTool(const Options& options);

    //! \brief Runs the tool
    //! \returns The exit code to return
    int Run();

private:
    bool RequireTargetFileExists();

private:
    Options Opts;
};

} // namespace pcktool
