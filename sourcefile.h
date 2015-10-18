#ifndef SOURCEFILE_H
#define SOURCEFILE_H

#include <string>
#include <vector>
#include <cstdint>
#include "pathhash.h"

class Source;

/// Filesystem metadata (permissions, ownership, ...)
struct FileAttr
{
    uint64_t mtime; ///< Time of last modification
    uint32_t userId, groupId; ///< Owner user/group
    uint16_t mode; ///< Permissions (as returned by stat(2))
};

/// Provides access to a source file
class SourceFile
{
public:
    SourceFile(const Source* parent, const std::string& path); ///< Construct from a real source file
    SourceFile(const Source* parent, const std::vector<char>& metadata,
               uint64_t mtime, const std::vector<char>& data); ///< Construct from downloaded data

    uint64_t getRawSize() const;
    FileAttr getAttrs() const;
    std::string getPath() const;
    PathHash getPathHash() const;

    std::vector<char> readAll() const;
    std::vector<char> serializeMetadata() const;

    /// Sorts according to the path hash
    bool operator <(const SourceFile& other) const;

private:
    void applyAttrs();

private:
    PathHash pathHash; ///< Hash of the path, for source and archive folders
    const Source* parent;
    std::string path; ///< Path of the file relative to it's Folder, only for source folders
    uint64_t rawSize; ///< Size of the source file
    FileAttr attrs; ///< File attributes
};

#endif // SOURCEFILE_H
