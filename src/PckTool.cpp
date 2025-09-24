// ------------------------------------ //
#include "PckTool.h"

#include "pck/PckFile.h"

#include <filesystem>
#include <iostream>
#include <utility>

using namespace pcktool;
// ------------------------------------ //
PckTool::PckTool(Options options) : Opts(std::move(options)) {}
// ------------------------------------ //
int PckTool::Run()
{
    if(!BuildFileList())
        return 1;

    if(Opts.Action == "list" || Opts.Action == "l") {
        auto pck = LoadPck();

        if(!pck)
            return 2;

        std::cout << "Pck version: " << pck->GetFormatVersion()
                  << ", Godot: " << pck->GetGodotVersion() << "\n";
        std::cout << "Contents of '" << Opts.Pack << "':\n";

        pck->PrintFileList(Opts.PrintHashes);

        std::cout << "end of contents.\n";
        return 0;
    } else if(Opts.Action == "repack" || Opts.Action == "r") {
        // This action just reads the pck and writes it out again over the file (if no other
        // files are specified)
        auto pck = LoadPck();

        if(!pck)
            return 2;

        if(!Files.empty()) {
            if(Files.size() != 1) {
                std::cout << "ERROR: only one target file to repack as is allowed\n";
                return 1;
            }

            pck->ChangePath(Files.front().InputFile);
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

        if(!pck->Extract(Opts.Output, !Opts.ReducedVerbosity)) {
            std::cout << "ERROR: extraction failed\n";
            return 2;
        }

        std::cout << "Extraction completed\n";

        return 0;
    } else if(Opts.Action == "add" || Opts.Action == "a") {
        if(Files.empty()) {
            std::cout << "ERROR: no files specified\n";
            return 1;
        }

        std::unique_ptr<PckFile> pck;

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

            SetIncludeFilter(*pck);

            pck->SetGodotVersion(Opts.GodotMajor, Opts.GodotMinor, Opts.GodotPatch);
        }

        for(const auto& entry : Files) {
            if(!entry.Target.empty()) {
                // Already known target for a single file
                pck->AddSingleFile(entry.InputFile, pck->PreparePckPath(entry.Target, ""),
                    !Opts.ReducedVerbosity);
            } else {
                // Potentially a folder tree with no pre-defined targets
                if(!pck->AddFilesFromFilesystem(
                       entry.InputFile, Opts.RemovePrefix, !Opts.ReducedVerbosity)) {
                    std::cout << "ERROR: failed to process file to add: " << entry.InputFile
                              << "\n";
                    return 3;
                }
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
bool PckTool::TargetExists() const
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

    SetIncludeFilter(*pck);

    if(!pck->Load()) {
        std::cout << "ERROR: couldn't load pck file: " << pck->GetPath() << "\n";
        return nullptr;
    }

    return pck;
}
// ------------------------------------ //
void PckTool::SetIncludeFilter(PckFile& pck)
{
    pck.SetIncludeFilter(std::bind(&FileFilter::Include, Opts.Filter, std::placeholders::_1));
}
// ------------------------------------ //
bool PckTool::BuildFileList()
{
    // Copy standard files
    for(const auto& entry : Opts.Files)
        Files.push_back({entry});

    // Handle json commands
    if(Opts.FileCommands.is_array()) {
        for(const auto& entry : Opts.FileCommands) {
            try {
                Files.push_back(
                    {entry["file"].get<std::string>(), entry["target"].get<std::string>()});
            } catch(const std::exception& e) {
                std::cout << "ERROR: unexpected JSON object format in array: " << e.what()
                          << "\n";
                std::cout << "Incorrect object:\n";
                std::cout << entry << "\n";
                return false;
            }
        }
    }

    // Use first file as the pck if pck is missing
    if(Opts.Pack.empty()) {
        if(Files.empty()) {
            std::cout << "ERROR: No pck file or list of files given\n";
            return false;
        }

        // User first file as the pck file
        Opts.Pack = Files.front().InputFile;
        Files.erase(Files.begin());
    }

    if(Opts.Pack.find(pcktool::GODOT_PCK_EXTENSION) == std::string::npos) {
        std::cout << "WARNING: Given pck file doesn't contain the pck file extension\n";
    }

    return true;
}
