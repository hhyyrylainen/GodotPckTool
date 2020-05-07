#include "Define.h"
#include "PackTool.h"

#include <cxxopts.hpp>

#include <iostream>
#include <string>


int main(int argc, char* argv[])
{
    cxxopts::Options options("godotpcktool", "Godot .pck file extractor and packer");

    // clang-format off
    options.add_options()
        ("p,pack", "Pck file to use", cxxopts::value<std::string>())
        ("a,action", "Action to perform", cxxopts::value<std::string>()->default_value("list"))
        ("f,file", "Files to perform the action on",
            cxxopts::value<std::vector<std::string>>())
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
        return 0;
    }

    std::string pack;
    std::string action;
    std::vector<std::string> files;

    if(result.count("file")) {
        files = result["file"].as<decltype(files)>();
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

    auto tool = packtool::PackTool({pack, action, files});

    return tool.Run();
}
