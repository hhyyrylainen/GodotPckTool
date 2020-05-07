#pragma once

#include "Define.h"

#include <string>
#include <vector>

namespace packtool {

//! \brief Main class for the Godot Pack Tool
class PackTool {
public:
    struct Options {
        std::string Pack;
        std::string Action;
        std::vector<std::string> Files;
    };

public:
    PackTool(const Options& options);

    //! \brief Runs the tool
    //! \returns The exit code to return
    int Run();

private:
    bool RequireTargetFileExists();

private:
    Options Opts;
};

} // namespace packtool
