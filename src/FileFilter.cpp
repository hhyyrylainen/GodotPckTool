// ------------------------------------ //
#include "FileFilter.h"

using namespace pcktool;
// ------------------------------------ //
bool FileFilter::Include(const PckFile::ContainedFile& file) const
{
    if(file.Size < MinSizeLimit)
        return false;

    if(file.Size > MaxSizeLimit)
        return false;

    if(!IncludePatterns.empty()){
        bool matched = false;

        for(const auto& pattern : IncludePatterns){
            if(std::regex_search(file.Path, pattern)){
                matched = true;
                break;
            }
        }

        if(!matched)
            return false;
    }

    if(!ExcludePatterns.empty()){
        for(const auto& pattern : ExcludePatterns){
            if(std::regex_search(file.Path, pattern))
                return false;
        }
    }

    // Wasn't excluded
    return true;
}
// ------------------------------------ //
