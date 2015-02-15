#ifndef FILE_H
#define FILE_H

#include <string>
#include <vector>
#include <cstdint>

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

/// Filesystem metadata (permissions, ownership, ...)
struct FileAttr
{
    uint64_t mtime; ///< Time of last modification
    uint32_t userId, groupId; ///< Owner user/group
    uint16_t mode; ///< Permissions (as returned by stat(2))
};

/// Metadata needed to represent a stored file in a Folder
/// Files in an Archive folder are always compressed then encrypted
class File
{
public:
    File(const std::string& path); ///< Construct from a real source file
    File(const std::vector<char>& data); ///< Construct from serialized data

    std::vector<char> serialize() const;
    void deserialize(const std::vector<char>& data);

private:
    void readAttributes(); ///< Assumes path is valid
    void computeCRC(); ///< Assumes path is valid

public:
    std::string path; ///< Path of the file relative to it's Folder (the backup folder)
    uint64_t rawSize; ///< Size of the uncompressed, raw file itself
    uint64_t actualSize; ///< Size of the file taking compression, metadata, etc into account
    uint32_t crc32; ///< CRC32 of the raw file data
    FileAttr attrs; ///< File attributes
};

#endif // FILE_H
