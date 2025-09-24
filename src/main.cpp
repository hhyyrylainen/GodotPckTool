#include "Define.h"
#include "FileFilter.h"
#include "PckTool.h"

#include <cxxopts.hpp>

#include <iostream>
#include <regex>
#include <string>
#include <tuple>

constexpr auto MAX_ACTION_NAME_LENGTH = 27;

void PrintActionLine(const std::string_view firstPart, const std::string_view descriptionPart)
{
    std::cout << "  " << firstPart;

    int padding = MAX_ACTION_NAME_LENGTH - static_cast<int>(firstPart.size()) - 2;

    for(int i = 0; i < padding; ++i)
        std::cout << " ";

    std::cout << descriptionPart << "\n";
}

std::tuple<int, int, int> ParseGodotVersion(const std::string& version)
{
    const auto versionFormat = std::regex(R"((\d+)\.(\d+)\.(\d+))");

    std::smatch matches;

    if(!std::regex_match(version, matches, versionFormat))
        throw std::runtime_error("invalid version format, expected format: x.y.z");

    return std::make_tuple(
        std::stoi(matches[1]), std::stoi(matches[2]), std::stoi(matches[3]));
}

std::vector<std::regex> ParseRegexList(const std::vector<std::string>& regexTexts)
{
    std::vector<std::regex> result;
    result.reserve(regexTexts.size());

    for(const auto& text : regexTexts) {
        // TODO: should we allow enabling case-insensitive mode?
        result.emplace_back(text, std::regex_constants::ECMAScript);
    }

    return result;
}

int main(int argc, char* argv[])
{
    cxxopts::Options options("godotpcktool", "Godot .pck file extractor and packer");

    // clang-format off
    options.add_options()
        ("p,pack", "Pck file to use", cxxopts::value<std::string>())
        ("a,action", "Action to perform", cxxopts::value<std::string>()->default_value("list"))
        ("positional-file", "Files to perform the action on",
            cxxopts::value<std::vector<std::string>>())
        ("f,file", "Files to perform the action on",
            cxxopts::value<std::vector<std::string>>())
        ("o,output", "Target folder for extracting", cxxopts::value<std::string>())
        ("remove-prefix", "Remove a prefix from files added to a pck",
            cxxopts::value<std::string>())
        ("command-file", "Read JSON commands from the specified file", cxxopts::value<std::string>())
        ("set-godot-version", "Set the godot version to use when creating a new pck",
            cxxopts::value<std::string>()->default_value("4.0.0"))
        ("min-size-filter", "Set minimum size for files to include in operation",
            cxxopts::value<uint64_t>())
        ("max-size-filter", "Set maximum size for files to include in operation",
            cxxopts::value<uint64_t>())
        ("i,include-regex-filter", "Set inclusion regexes for files to include in operation, "
            "if not set all files are included",
            cxxopts::value<std::vector<std::string>>())
        ("e,exclude-regex-filter", "Set exclusion regexes for files to include in operation, "
            "if both include and exclude are specified includes are handled first and "
            "excludes exclude files that include filters passed through",
            cxxopts::value<std::vector<std::string>>())
        ("include-override-filter", "Set regexes for files to include in operation even if "
            "another filter might exclude it, doesn't affect inclusion of files not "
            "matching this",
            cxxopts::value<std::vector<std::string>>())
        ("q,quieter", "Don't output all processed files to keep output more compact")
        ("print-hashes", "Print hashes of contained files in the .pck")
        ("v,version", "Print version and quit")
        ("h,help", "Print help and quit")
        ;
    // clang-format on

    options.parse_positional({"positional-file"});
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
    std::string commandFile;
    int godotMajor, godotMinor, godotPatch;
    nlohmann::json fileCommands;
    pcktool::FileFilter filter;
    bool reducedVerbosity = false;
    bool printHashes = false;

    if(result.count("file")) {
        files = result["file"].as<decltype(files)>();
    }

    if(result.count("positional-file")) {
        for(const auto& file : result["positional-file"].as<decltype(files)>()) {
            files.push_back(file);
        }
    }

    if(result.count("output")) {
        output = result["output"].as<std::string>();
    }

    if(result.count("remove-prefix")) {
        removePrefix = result["remove-prefix"].as<std::string>();
    }

    if(result.count("pack")) {
        pack = result["pack"].as<std::string>();
    }

    if(result.count("command-file")) {
        commandFile = result["command-file"].as<std::string>();
    }

    if(result.count("min-size-filter")) {
        filter.SetSizeMinLimit(result["min-size-filter"].as<uint64_t>());
    }

    if(result.count("max-size-filter")) {
        filter.SetSizeMaxLimit(result["max-size-filter"].as<uint64_t>());
    }

    if(result.count("include-regex-filter")) {
        // TODO: should we add a wildcard / plain text search mode?
        filter.SetIncludeRegexes(
            ParseRegexList(result["include-regex-filter"].as<std::vector<std::string>>()));
    }

    if(result.count("exclude-regex-filter")) {
        filter.SetExcludeRegexes(
            ParseRegexList(result["exclude-regex-filter"].as<std::vector<std::string>>()));
    }

    if(result.count("include-override-filter")) {
        filter.SetIncludeOverrideRegexes(
            ParseRegexList(result["include-override-filter"].as<std::vector<std::string>>()));
    }

    if(result.count("quieter")) {
        reducedVerbosity = true;
    }

    if(result.count("print-hashes")) {
        printHashes = true;
    }

    action = result["action"].as<std::string>();

    try {
        std::tie(godotMajor, godotMinor, godotPatch) =
            ParseGodotVersion(result["set-godot-version"].as<std::string>());
    } catch(const std::exception& e) {
        std::cout << "ERROR: specified version number is invalid: " << e.what() << "\n";
        return 1;
    }

    bool alreadyRead = false;

    // Stdin json commands
    for(auto iter = files.begin(); iter != files.end();) {
        if(*iter != "-" || alreadyRead) {
            ++iter;
            continue;
        }

        alreadyRead = true;
        iter = files.erase(iter);

        std::cout << "Reading JSON file commands from STDIN until EOF...\n";

        std::string data;

        std::string line;
        while(std::getline(std::cin, line)) {
            data += line;
        }

        std::cout << "Finished reading STDIN (total characters: " << data.size() << ").\n";

        try {
            fileCommands = nlohmann::json::parse(data);
        } catch(const nlohmann::json::parse_error& e) {
            std::cout << "ERROR: invalid json: " << e.what() << "\n";
            return 1;
        }
    }

    if(!commandFile.empty()) {
        std::cout << "Reading JSON commands from file: " << commandFile << "\n";

        try {
            auto file = std::ifstream(commandFile);

            if(!file.good())
                throw std::runtime_error("failed to open the command file");

            std::stringstream buffer;
            buffer << file.rdbuf();

            const auto parsed = nlohmann::json::parse(buffer);

            if(!parsed.is_array()) {
                throw std::runtime_error(
                    "expected JSON file to contain a single JSON array with objects in it");
            }

            if(!fileCommands.is_array()) {
                // If didn't read stdin commands, make the result array now
                fileCommands = nlohmann::json::array();
            }

            for(const auto& entry : parsed) {
                fileCommands.push_back(entry);
            }

        } catch(const std::exception& e) {
            std::cout << "ERROR: invalid json file: " << e.what() << "\n";
            return 1;
        }
    }

    auto tool = pcktool::PckTool({pack, action, files, output, removePrefix, godotMajor,
        godotMinor, godotPatch, fileCommands, filter, reducedVerbosity, printHashes});

    return tool.Run();
}
