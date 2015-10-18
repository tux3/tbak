#ifndef ARCHIVEFILE_H
#define ARCHIVEFILE_H

#include "pathhash.h"
#include <vector>

class Archive;

class ArchiveFile
{
public:
    ArchiveFile(const Archive* parent, PathHash pathHash, uint64_t mtime, const std::vector<char>& data);
    ArchiveFile(const Archive* parent, std::vector<char>::const_iterator& serializedData); ///< Reads from serialized data
    PathHash getPathHash() const;
    uint64_t getMtime() const;
    uint64_t getActualSize() const;

    std::vector<char> read(uint64_t startPos, uint64_t size) const;
    std::vector<char> readMetadata() const;
    std::vector<char> readAll() const;
    void overwrite(uint64_t mtime, const std::vector<char>& data);

    /// Serializes only the metadata, not the content of the file
    std::vector<char> serialize() const;
    /// Deserializes the file path from the metadata
    static std::string deserializePath(std::vector<char>::const_iterator& meta);
    /// Size of the serialized data
    static constexpr size_t serializedSize()
    {
        return PathHash::hashlen + sizeof(mtime) + sizeof(actualSize);
    }

private:
    PathHash pathHash;
    uint64_t mtime;
    uint64_t actualSize;
    const Archive* parent;
};

#endif // ARCHIVEFILE_H
