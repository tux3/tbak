#include "filetime.h"

FileTime::FileTime()
{
}

bool FileTime::operator<(const FileTime &other) const noexcept
{
    return hash < other.hash;
}

bool FileTime::operator==(const FileTime &other) const noexcept
{
    return hash == other.hash;
}
