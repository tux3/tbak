#ifndef FOLDERDB_H
#define FOLDERDB_H

#include <vector>
#include <string>
#include "folder.h"
#include "filelocker.h"

/// TODO: Store archives in ~/.tbak/<hash of the folder path>
/// So this way we can always find it in the remote
/// Also, keep a compiled binary in ~/.tbak/ so clients can find it

/// Maintains a database of Folders
class FolderDB
{
public:
    FolderDB(const std::string& path); ///< Reads serialized data from file

    void load();
    void deserialize(const std::vector<char>& data);
    void save() const;
    std::vector<char> serialize() const;

    std::vector<Folder>& getFolders();
    const std::vector<Folder>& getFolders() const;
    void addFolder(const Folder& folder);
    void addFolder(const std::string& path);
    bool removeFolder(const std::string& path, bool archive=true);

private:
    std::vector<Folder> folders;
    FileLocker file;
};

#endif // FOLDERDB_H
