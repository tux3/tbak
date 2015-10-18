#include "folderdb.h"
#include "serialize.h"
#include "pathtools.h"
#include "settings.h"
#include <fstream>
#include <algorithm>
#include <iostream>

using namespace std;

FolderDB::FolderDB(const string &path)
    : file{path}
{
    load();
}

FolderDB::~FolderDB()
{
    save();
}

void FolderDB::save() const
{
    file.overwrite(serialize());
}

vector<char> FolderDB::serialize() const
{
    vector<char> data;

    vector<vector<char>> archivesData;
    for (const Archive& f : archives)
        archivesData.push_back(f.serialize());
    vector<vector<char>> sourcesData;
    for (const Source& f : sources)
        sourcesData.push_back(::serialize(f.getPath()));

    serializeAppend(data, archivesData);
    serializeAppend(data, sourcesData);

    return data;
}

void FolderDB::load()
{
    deserialize(file.readAll());
}

void FolderDB::deserialize(const std::vector<char> &data)
{
    createPathTo("/", dataPath()+"archive/");

    if (data.empty())
        return;
    auto it = begin(data);

    vector<vector<char>> archivesData = deserializeConsume<decltype(archivesData)>(it);
    for (const vector<char>& vec : archivesData)
        archives.emplace_back(vec);
    vector<vector<char>> sourcesData = deserializeConsume<decltype(sourcesData)>(it);
    for (const vector<char>& vec : sourcesData)
    {
        auto it = vec.begin();
        sources.emplace_back(::dataToString(it));
    }
}

const std::vector<Source> &FolderDB::getSources() const
{
    return sources;
}

const std::vector<Archive>& FolderDB::getArchives() const
{
    return archives;
}

Source *FolderDB::getSource(const string &path)
{
    auto it = find_if(begin(sources), end(sources), [&path](const Source& s)
    {
        return s.getPath() == path;
    });
    if (it == end(sources))
        return nullptr;

    return &*it;
}

Archive *FolderDB::getArchive(const PathHash &pathHash)
{
    auto it = find_if(begin(archives), end(archives), [&pathHash](const Archive& a)
    {
        return a.getPathHash() == pathHash;
    });
    if (it == end(archives))
        return nullptr;

    return &*it;
}

void FolderDB::addArchive(PathHash pathHash)
{
    if (any_of(begin(archives), end(archives), [&pathHash](const Archive& a){return a.getPathHash() == pathHash;}))
        return;

    archives.emplace_back(pathHash);
}

void FolderDB::addSource(const std::string& path)
{
    if (any_of(begin(sources), end(sources), [&path](const Source& s){return s.getPath() == path;}))
        return;

    sources.emplace_back(path);
}

bool FolderDB::removeArchive(const PathHash& pathHash)
{
    auto it = find_if(begin(archives), end(archives), [&pathHash](const Archive& a)
    {
        return a.getPathHash() == pathHash;
    });
    if (it == end(archives))
    {
        cout << "FolderDB::removeArchive: Archive "<<pathHash.toBase64()<<" not found"<<endl;
        return false;
    }

    it->removeData();
    archives.erase(it);
    return true;
}

bool FolderDB::removeArchive(const string &pathHashStr)
{
    auto it = find_if(begin(archives), end(archives), [&pathHashStr](const Archive& a)
    {
        return a.getPathHash().toBase64() == pathHashStr;
    });
    if (it == end(archives))
    {
        cout << "FolderDB::removeArchive: Archive "<<pathHashStr<<" not found"<<endl;
        return false;
    }

    it->removeData();
    archives.erase(it);
    return true;
}

bool FolderDB::removeSource(const std::string& path)
{
    size_t size = sources.size();
    sources.erase(remove_if(begin(sources), end(sources), [&path](const Source& s)
    {
        return s.getPath() == path;
    }), end(sources));
    return size != sources.size();
}

