// ------------------------------------ //
#include "PackTool.h"

#include <filesystem>
#include <iostream>

using namespace packtool;
// ------------------------------------ //
PackTool::PackTool(const Options& options) : Opts(options) {}
// ------------------------------------ //
int PackTool::Run()
{
    if(Opts.Action == "list") {
        if(!RequireTargetFileExists())
            return 2;

        std::cout << "Contents of '" << Opts.Pack << "':\n";

        std::cout << "end of contents.\n";
        return 0;
    }

    std::cout << "ERROR: unknown action: " << Opts.Action << "\n";
    return 1;
}
// ------------------------------------ //
bool PackTool::RequireTargetFileExists()
{
    if(!std::filesystem::exists(Opts.Pack)) {
        std::cout << "ERROR: specified pck file doesn't exist: " << Opts.Pack << "\n";
        return false;
    }

    return true;
}
