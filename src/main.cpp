#include "Define.h"
#include "PckTool.h"

#include <cxxopts.hpp>

#include <iostream>
#include <string>

constexpr auto MAX_ACTION_NAME_LENGTH = 27;

void PrintActionLine(const std::string_view firstPart, const std::string_view descriptionPart)
{
    std::cout << "  " << firstPart;

    int padding = MAX_ACTION_NAME_LENGTH - firstPart.size() - 2;

    for(int i = 0; i < padding; ++i)
        std::cout << " ";

    std::cout << descriptionPart << "\n";
}

int main(int argc, char* argv[])
{
    cxxopts::Options options("godotpcktool", "Godot .pck file extractor and packer");

    // clang-format off
    options.add_options()
        ("p,pack", "Pck file to use", cxxopts::value<std::string>())
        ("a,action", "Action to perform", cxxopts::value<std::string>()->default_value("list"))
        ("f,file", "Files to perform the action on",
            cxxopts::value<std::vector<std::string>>())
        ("o,output", "Target folder for extracting", cxxopts::value<std::string>())
        ("remove-prefix", "Remove a prefix from files added to a pck",
            cxxopts::value<std::string>())
        ("v,version", "Print version and quit")
        ("h,help", "Print help and quit")
        ;
    // clang-format on

    options.parse_positional({"file"});
    options.positional_help("files");

    auto result = options.parse(argc, argv);

    // Handle options
    if(result.count("version")) {
        // Use first file as the pack file
        std::cout << "GodotPckTool version " << GODOT_PCK_TOOL_VERSIONS << "\n";
        return 0;
    }

    if(result.count("help")) {
        std::cout << options.help();
        std::cout << "Available actions (brackets denote shorthand):\n";

        PrintActionLine("[l]ist", "List contents of a pck file");
        PrintActionLine("[e]xtract", "Extract the contents of a pck");
        PrintActionLine("[a]dd", "Add files to a new or existing pck");
        PrintActionLine("[r]epack", "Repack an existing pack, optionally to a different file");
        return 0;
    }

    std::string pack;
    std::string action;
    std::vector<std::string> files;
    std::string output;
    std::string removePrefix;

    if(result.count("file")) {
        files = result["file"].as<decltype(files)>();
    }

    if(result.count("output")) {
        output = result["output"].as<std::string>();
    }

    if(result.count("removePrefix")) {
        removePrefix = result["remove-prefix"].as<std::string>();
    }

    if(result.count("pack") < 1) {
        // Use first file as the pack file
        if(files.empty()) {
            std::cout << "ERROR: No pck file or list of files given\n";
            return 1;
        }

        // User first file as the pck file
        pack = files.front();
        files.erase(files.begin());

    } else {
        pack = result["pack"].as<std::string>();
    }

    action = result["action"].as<std::string>();

    if(pack.find(pcktool::GODOT_PCK_EXTENSION) == std::string::npos) {
        std::cout << "ERROR: Given pck file doesn't contain the pck file extension\n";
        return 1;
    }

    auto tool = pcktool::PckTool({pack, action, files, output, removePrefix});

    return tool.Run();
}
