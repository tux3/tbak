#ifndef FOLDER_H
#define FOLDER_H

#include <string>
#include <vector>
#include <mutex>
#include "file.h"
#include "crypto.h"

class Server;

enum class FolderType : uint8_t
{
    Source,
    Archive
};

/// Metadata about an archived folder, a set of files.
/// A Folder can be a source folder (raw files) or an archive (compressed, encrypted files)
class Folder
{
public:
    Folder(const Folder& other);
    Folder(const std::string& path); ///< Construct from a real source folder
    Folder(const std::vector<char>& data); ///< Construct from serialized data
    ~Folder();
    Folder& operator=(const Folder& other);

    std::vector<char> serialize() const;
    void deserialize(const std::vector<char>& data);

    std::string getPath() const;
    FolderType getType() const;
    void setType(FolderType type);
    std::string getTypeString() const;
    uint64_t getRawSize() const;
    uint64_t getActualSize() const;
    size_t getFileCount() const;
    const std::vector<File>& getFiles() const;

    static std::string normalizePath(const std::string& folder);
    static std::string normalizeFileName(const std::string& folder, const std::string& file);
    std::string getFilesDbPath() const; ///< Returns the path of the Files database for this Folder
    std::string getFolderDataPath() const; ///< Returns the path of the data folder, containing the files db

    void open(bool forceupdate=false); ///< Opens and reads the Files database from disk
    void close(); ///< Closes the Files database
    void removeData() const; ///< Delete this Folder's Files database and data path
    /// Unserialize and write a downloaded archive file to disk
    void writeArchiveFile(const std::vector<char>& data, const Server& s, const PublicKey& rpk);
    /// Deletes an archive file, if it exists
    bool removeArchiveFile(const PathHash &pathHash);

private:
    std::vector<std::string> listfiles(const char *name, int level) const; ///< Lists files recursively
    void deleteFolderRecursively(const char* path) const; ///< Deletes the folder and all of its contents
    void updateSizes(); ///< Re-computes rawSize and actualSize
    void createDirectory(const char* path) const; ///< Create a physical directory on disk
    void createDirectory(const std::string& path) const; ///< Create a physical directory on disk
    void createPathTo(const std::string& relfile) const; ///< Create the necessary directories to a file in this folder

private:
    FolderType type; ///< Type (source/archive) of the folder
    std::string path; ///< Absolute path of the folder
    std::string hash; ///< Hash of the absolute path
    uint64_t rawSize; ///< Cumulated size of the folder's raw files
    uint64_t actualSize; ////< Size of the actual disk space used, taking metadata, compression, etc into account
    std::vector<File> files; ///< Files stored in this directory
    bool isOpen;
    mutable std::recursive_mutex mutex;
};

#endif // FOLDER_H
