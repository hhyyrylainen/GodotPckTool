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
    } else if(Opts.Action == "repack") {
        // This action just reads the pck and writes it out again over the file (if no other
        // files are specified)
        if(!RequireTargetFileExists())
            return 2;

        auto pck = PckFile(Opts.Pack);

        if(!pck.Load()) {
            std::cout << "ERROR: couldn't load pck file\n";
            return 2;
        }

        if(!Opts.Files.empty()) {
            if(Opts.Files.size() != 1) {
                std::cout << "ERROR: only one target file to repack as is allowed\n";
                return 1;
            }

            pck.ChangePath(Opts.Files.front());
        }

        std::cout << "Repacking to: " << pck.GetPath() << "\n";

        if(!pck.Save()) {
            std::cout << "Failed to repack\n";
            return 2;
        }

        std::cout << "Repack complete\n";
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
