#ifndef FOLDER_H
#define FOLDER_H

#include <string>
#include <vector>
#include "file.h"

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
    Folder(const std::string& path); ///< Construct from a real source folder
    Folder(const std::vector<char>& data); ///< Construct from serialized data
    ~Folder();

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

    void open(bool forceupdate=false); ///< Opens and reads the Files database from disk
    void close(); ///< Closes the Files database
    void removeData(); ///< Delete this Folder's Files database and data path
    void writeArchiveFile(const std::vector<char>& data); ///< Unserialize and write a downloaded archive file to disk

private:
    std::vector<std::string> listfiles(const char *name, int level); ///< Lists files recursively
    void deleteFolderRecursively(const char* path); ///< Deletes the folder and all of its contents
    void updateSizes(); ///< Re-computes rawSize and actualSize
    void createDirectory(const char* path); ///< Create a physical directory on disk
    void createDirectory(const std::string& path); ///< Create a physical directory on disk
    std::string getFilesDbPath(); ///< Returns the path of the Files database for this Folder

private:
    FolderType type; ///< Type (source/archive) of the folder
    std::string path; ///< Absolute path of the file
    std::string hash; ///< Hash of the absolute path
    uint64_t rawSize; ///< Cumulated size of the folder's raw files
    uint64_t actualSize; ////< Size of the actual disk space used, taking metadata, compression, etc into account
    std::vector<File> files; ///< Files stored in this directory
    bool isOpen;
};

#endif // FOLDER_H
