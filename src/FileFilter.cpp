// ------------------------------------ //
#include "FileFilter.h"

using namespace pcktool;
// ------------------------------------ //
bool FileFilter::Include(const PckFile::ContainedFile& file)
{
    if(file.Size < MinSizeLimit)
        return false;

    if(file.Size > MaxSizeLimit)
        return false;

    // Wasn't excluded
    return true;
}
// ------------------------------------ //
