// ------------------------------------ //
#include "PckTool.h"

#include "pck/PckFile.h"

#include <filesystem>
#include <iostream>

using namespace pcktool;
// ------------------------------------ //
PckTool::PckTool(const Options& options) : Opts(options) {}
// ------------------------------------ //
int PckTool::Run()
{
    if(Opts.Action == "list") {
        if(!RequireTargetFileExists())
            return 2;

        auto pck = PckFile(Opts.Pack);

        if(!pck.Load()) {
            std::cout << "ERROR: couldn't load pck file\n";
            return 2;
        }

        std::cout << "Contents of '" << Opts.Pack << "':\n";

        pck.PrintFileList();

        std::cout << "end of contents.\n";
        return 0;
    }

    std::cout << "ERROR: unknown action: " << Opts.Action << "\n";
    return 1;
}
// ------------------------------------ //
bool PckTool::RequireTargetFileExists()
{
    if(!std::filesystem::exists(Opts.Pack)) {
        std::cout << "ERROR: specified pck file doesn't exist: " << Opts.Pack << "\n";
        return false;
    }

    return true;
}
