#ifndef FOLDER_H
#define FOLDER_H

#include <string>
#include <vector>
#include <mutex>
#include "archivefile.h"
#include "crypto.h"

class Server;

/// Metadata about an archived folder, a set of compressed encrypted files.
class Archive
{
public:
    Archive(const Archive& other);
    Archive(PathHash pathHash); ///< Construct an empty archive
    Archive(const std::vector<char>& data); ///< Construct from serialized data
    ~Archive();
    Archive& operator=(const Archive& other);

    std::vector<char> serialize() const;
    void deserialize(const std::vector<char>& data);

    PathHash getPathHash() const;
    uint64_t getActualSize() const;
    size_t getFileCount() const;
    ArchiveFile* getFile(PathHash filePathHash);
    const std::vector<ArchiveFile>& getFiles() const;

    std::string getFilesDbPath() const; ///< Returns the path of the Files database for this Folder
    std::string getFolderDataPath() const; ///< Returns the path of the data folder, containing the files db

    void removeData() const; ///< Delete this Folder's Files database and data path
    /// Write a downloaded archive file to disk, adding it to our list if it's new
    void writeArchiveFile(const PathHash& filePath, uint64_t mtime, const std::vector<char>& data);
    /// Deletes an archive file, if it exists
    bool removeArchiveFile(const PathHash &pathHash);

private:
    std::vector<std::string> listfiles(const char *name, int level) const; ///< Lists files recursively
    void deleteFolderRecursively(const char* path) const; ///< Deletes the folder and all of its contents

private:
    PathHash pathHash; ///< Hash of the absolute path of the folder
    uint64_t actualSize; ////< Actual disk space used, taking metadata, compression, etc into account
    std::vector<ArchiveFile> files; ///< Files stored in this archive. NOT in the serialized data!
    mutable std::recursive_mutex mutex;
};

#endif // FOLDER_H
