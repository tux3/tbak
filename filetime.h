#ifndef FILETIME_H
#define FILETIME_H

#include "pathhash.h"
#include <cstdint>

struct FileTime
{
public:
    FileTime();

    // Comparison operators only consider the path hash, not the time
    bool operator<(const FileTime& other) const noexcept;
    bool operator==(const FileTime& other) const noexcept;

public:
    PathHash hash;
    uint64_t mtime;
};

#endif // FILETIME_H
