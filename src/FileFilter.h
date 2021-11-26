#pragma once

#include "Define.h"

#include "pck/PckFile.h"

#include <limits>

namespace pcktool {

//! \brief An include / exclude filter for files
class FileFilter {
public:
    //! \returns true if filter doesn't exclude a file
    bool Include(const PckFile::ContainedFile& file);

    void SetSizeMinLimit(uint64_t size)
    {
        MinSizeLimit = size;
    }

    void SetSizeMaxLimit(uint64_t size)
    {
        MaxSizeLimit = size;
    }

private:
    //! File is excluded if it is under this size
    uint64_t MinSizeLimit = 0;

    //! File is excluded if it is over this size
    uint64_t MaxSizeLimit = std::numeric_limits<uint64_t>::max();
};

} // namespace pcktool
