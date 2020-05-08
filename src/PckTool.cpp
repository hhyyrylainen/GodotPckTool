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
    if(Opts.Action == "list" || Opts.Action == "l") {
        auto pck = LoadPck();

        if(!pck)
            return 2;

        std::cout << "Contents of '" << Opts.Pack << "':\n";

        pck->PrintFileList();

        std::cout << "end of contents.\n";
        return 0;
    } else if(Opts.Action == "repack" || Opts.Action == "r") {
        // This action just reads the pck and writes it out again over the file (if no other
        // files are specified)
        auto pck = LoadPck();

        if(!pck)
            return 2;

        if(!Opts.Files.empty()) {
            if(Opts.Files.size() != 1) {
                std::cout << "ERROR: only one target file to repack as is allowed\n";
                return 1;
            }

            pck->ChangePath(Opts.Files.front());
        }

        std::cout << "Repacking to: " << pck->GetPath() << "\n";

        if(!pck->Save()) {
            std::cout << "Failed to repack\n";
            return 2;
        }

        std::cout << "Repack complete\n";
        return 0;
    } else if(Opts.Action == "extract" || Opts.Action == "e") {
        auto pck = LoadPck();

        if(!pck)
            return 2;

        std::cout << "Extracting to: " << Opts.Output << "\n";

        if(!pck->Extract(Opts.Output)) {
            std::cout << "ERROR: extraction failed\n";
            return 2;
        }

        return 0;
    } else if(Opts.Action == "add" || Opts.Action == "a") {
        std::cout << "TODO: add\n";
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

std::unique_ptr<PckFile> PckTool::LoadPck()
{
    if(!RequireTargetFileExists())
        return nullptr;

    auto pck = std::make_unique<PckFile>(Opts.Pack);

    if(!pck->Load()) {
        std::cout << "ERROR: couldn't load pck file: " << pck->GetPath() << "\n";
        return nullptr;
    }

    return pck;
}
