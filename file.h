#ifndef FILE_H
#define FILE_H

#include <string>
#include <vector>
#include <cstdint>
#include <mutex>

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
class Folder;

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
    File(const File& other);
    File(const Folder* parent, const std::string& path); ///< Construct from a real source file
    File(const Folder* parent, const std::vector<char>& data); ///< Construct from serialized data
    File(const Folder* parent, std::vector<char>::const_iterator& data); ///< Construct from serialized data
    File& operator=(const File& other);

    std::vector<char> serialize() const;
    void deserialize(const std::vector<char>& data);
    void deserialize(std::vector<char>::const_iterator& data);
    std::vector<char> readAll() const;
    size_t metadataSize(); ///< Get the size of the serialized metadata. Slow.

private:
    void readAttributes(); ///< Assumes path is valid

public:
    const Folder* parent;
    std::string path; ///< Path of the file relative to it's Folder (the backup folder)
    uint64_t rawSize; ///< Size of the uncompressed, raw file itself
    uint64_t actualSize; ///< Size of the file taking compression, metadata, etc into account
    FileAttr attrs; ///< File attributes
    mutable std::mutex mutex;
};

#endif // FILE_H
