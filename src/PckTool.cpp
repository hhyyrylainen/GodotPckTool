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
        std::unique_ptr<PckFile> pck;

        if(Opts.Files.empty()) {
            std::cout << "ERROR: no files specified\n";
            return 1;
        }

        if(TargetExists()) {
            std::cout << "Target pck exists, loading it before adding new files\n";

            pck = LoadPck();

            if(!pck) {
                std::cout << "ERROR: couldn't load existing target pck. Please change the "
                             "target or delete the existing file.\n";
                return 2;
            }
        } else {
            pck = std::make_unique<PckFile>(Opts.Pack);
        }

        for(const auto& entry : Opts.Files) {
            if(!pck->AddFilesFromFilesystem(entry, Opts.RemovePrefix)) {
                std::cout << "ERROR: failed to process file to add: " << entry << "\n";
                return 3;
            }
        }

        if(!pck->Save()) {
            std::cout << "Failed to save pck\n";
            return 2;
        }

        std::cout << "Writing / updating pck finished\n";
        return 0;
    }

    std::cout << "ERROR: unknown action: " << Opts.Action << "\n";
    return 1;
}
// ------------------------------------ //
bool PckTool::TargetExists()
{
    return std::filesystem::exists(Opts.Pack);
}

bool PckTool::RequireTargetFileExists()
{
    if(!TargetExists()) {
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
