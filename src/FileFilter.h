#pragma once

#include "Define.h"

#include "pck/PckFile.h"

#include <limits>
#include <regex>
#include <utility>

namespace pcktool {

//! \brief An include / exclude filter for files
class FileFilter {
public:
    //! \returns true if filter doesn't exclude a file
    [[nodiscard]] bool Include(const PckFile::ContainedFile& file) const;

    void SetSizeMinLimit(uint64_t size)
    {
        MinSizeLimit = size;
    }

    void SetSizeMaxLimit(uint64_t size)
    {
        MaxSizeLimit = size;
    }

    void SetIncludeRegexes(const std::vector<std::regex>& filters)
    {
        IncludePatterns = filters;
    }

    void SetExcludeRegexes(const std::vector<std::regex>& filters)
    {
        ExcludePatterns = filters;
    }

    void SetIncludeOverrideRegexes(const std::vector<std::regex>& filters)
    {
        OverridePatterns = filters;
    }

private:
    //! File is excluded if it is under this size
    uint64_t MinSizeLimit = 0;

    //! File is excluded if it is over this size
    uint64_t MaxSizeLimit = std::numeric_limits<uint64_t>::max();

    //! If non-empty any passed in files must pass regex_search in at least one regex
    //! specified in here
    std::vector<std::regex> IncludePatterns;

    //! If non-empty then any files that pass the IncludePatterns filter (or if it is empty
    //! any file being checked) must not regex_search find a match in any regexes in this
    //! vector, if they do, they are excluded
    std::vector<std::regex> ExcludePatterns;

    //! If non-empty then any files that pass this filter are included anyway, even if they
    //! would fail another inclusion check
    std::vector<std::regex> OverridePatterns;
};

} // namespace pcktool
