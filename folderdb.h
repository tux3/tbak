#ifndef FOLDERDB_H
#define FOLDERDB_H

#include <vector>
#include <string>
#include "archive.h"
#include "source.h"
#include "filelocker.h"

/// Maintains a database of Folders
class FolderDB
{
public:
    FolderDB(const std::string& path); ///< Reads serialized data from file
    ~FolderDB(); ///< Saves automatically

    void load();
    void deserialize(const std::vector<char>& data);
    void save() const;
    std::vector<char> serialize() const;

    const std::vector<Source>& getSources() const;
    const std::vector<Archive>& getArchives() const;
    Source* getSource(const std::string& path);
    Archive* getArchive(const PathHash &pathHash);
    void addSource(const std::string& path);
    void addArchive(PathHash pathHash);
    bool removeSource(const std::string& path);
    bool removeArchive(const PathHash &pathHash);
    bool removeArchive(const std::string &pathHashStr); ///< Takes a base64 path hash string

private:
    std::vector<Archive> archives;
    std::vector<Source> sources;
    FileLocker file;
};

#endif // FOLDERDB_H
