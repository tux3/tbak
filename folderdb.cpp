#include "folderdb.h"
#include "serialize.h"
#include <fstream>
#include <algorithm>
#include <iostream>

using namespace std;

FolderDB::FolderDB(const string &path)
    : file{path}
{
    load();
}

void FolderDB::save() const
{
    file.overwrite(serialize());
}

vector<char> FolderDB::serialize() const
{
    vector<char> data;

    vector<vector<char>> foldersData;
    for (const Folder& f : folders)
        foldersData.push_back(f.serialize());

    serializeAppend(data, foldersData);

    return data;
}

void FolderDB::load()
{
    deserialize(file.readAll());
}

void FolderDB::deserialize(const std::vector<char> &data)
{
    if (data.empty())
        return;
    auto it = begin(data);

    vector<vector<char>> foldersData = deserializeConsume<decltype(foldersData)>(it);
    for (const vector<char>& vec : foldersData)
        folders.push_back(Folder(vec));
}

const std::vector<Folder>& FolderDB::getFolders() const
{
    return folders;
}

void FolderDB::addFolder(const Folder& folder)
{
    auto pred = [&folder](const Folder& f){return f.getPath() == folder.getPath();};
    if (find_if(begin(folders), end(folders), pred) != end(folders))
        return;

    folders.push_back(folder);
}

void FolderDB::addFolder(const std::string& path)
{
    auto pred = [&path](const Folder& f){return f.getPath() == path;};
    if (find_if(begin(folders), end(folders), pred) != end(folders))
        return;

    folders.push_back(Folder(path));
}

bool FolderDB::removeFolder(const std::string& path, bool archive)
{
    for (unsigned i=0; i<folders.size(); ++i)
    {
        if (folders[i].getPath() == path)
        {
            if (archive != (folders[i].getType() == FolderType::Archive))
                continue;
            folders[i].close();
            folders[i].removeData();
            folders.erase(begin(folders)+i);
            return true;
        }
    }
    return false;
}

Folder* FolderDB::getFolder(const string &path)
{
    Folder* folder=nullptr;
    for (size_t i=0; i<folders.size(); i++)
    {
        if (folders[i].getPath() == path)
        {
            folder = &folders[i];
            break;
        }
    }
    return folder;
}

Folder& FolderDB::getFolder(const std::vector<Folder>::const_iterator it)
{
    return *folders.erase(it, it); // Doesn't erase anything (empty range), but removes constness
}
