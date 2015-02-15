#include "folder.h"
#include "serialize.h"
#include "sha512.h"
#include "settings.h"
#include "filelocker.h"
#include <dirent.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>
#include <fstream>
#include <cassert>
#include <limits.h>

using namespace std;

Folder::Folder(const std::string& Path)
{
    type = FolderType::Source;
    path = normalizePath(Path);
    rawSize = actualSize = 0;

    sha512str(path.c_str(), path.size(), hash);

    open();
}

Folder::Folder(const std::vector<char>& data)
    : isOpen{false}
{
    deserialize(data);
}

Folder::~Folder()
{
    if (isOpen)
        close();
}

std::vector<std::string> Folder::listfiles(const char *name, int level)
{
    assert(name[strlen(name)-1] != '/');
    std::vector<std::string> files;
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name)))
        return files;
    if (!(entry = readdir(dir)))
        return files;

    do
    {
        if (entry->d_type == DT_DIR)
        {
            char path[1024];
            int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
            path[len] = 0;
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            vector<string> newfiles = listfiles(path, level + 1);
            files.insert(end(files), begin(newfiles), end(newfiles));
        }
        else
        {
            char path[1024];
            int len = snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            std::string s(path, len);
            files.push_back(s);
        }
    } while ((entry = readdir(dir)));
    closedir(dir);
    return files;
}

void Folder::deleteFolderRecursively(const char* name)
{
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name)))
        return;
    if (!(entry = readdir(dir)))
        return;

    do
    {
        if (entry->d_type == DT_DIR)
        {
            char path[1024];
            int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
            path[len] = 0;
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            deleteFolderRecursively(path);
        }
        else
        {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            unlink(path);
        }
    } while ((entry = readdir(dir)));
    closedir(dir);
    rmdir(name);
}

vector<char> Folder::serialize() const
{
    vector<char> data;

    serializeAppend(data, (uint8_t)type);
    serializeAppend(data, path);
    serializeAppend(data, rawSize);
    serializeAppend(data, actualSize);

    return data;
}

void Folder::deserialize(const std::vector<char>& data)
{
    auto it = begin(data);
    type = (FolderType)deserializeConsume<uint8_t>(it);
    path = deserializeConsume<decltype(path)>(it);
    rawSize = deserializeConsume<decltype(rawSize)>(it);
    actualSize = deserializeConsume<decltype(actualSize)>(it);

    sha512str(path.c_str(), path.size(), hash);
}

std::string Folder::getPath() const
{
    return path;
}

FolderType Folder::getType() const
{
    return type;
}

void Folder::setType(FolderType newtype)
{
    if (newtype != FolderType::Archive
        && newtype != FolderType::Source)
        return;

    type = newtype;
}

std::string Folder::getTypeString() const
{
    if (type == FolderType::Source)
        return "source";
    else if (type == FolderType::Archive)
        return "archive";
    else
        return "?????";
}

uint64_t Folder::getRawSize() const
{
    return rawSize;
}

uint64_t Folder::getActualSize() const
{
    return actualSize;
}

size_t Folder::getFileCount() const
{
    return files.size();
}

const std::vector<File>& Folder::getFiles() const
{
    return files;
}

void Folder::updateSizes()
{
    if (!isOpen)
        throw std::runtime_error("Folder::updateSizes: Folder not open");

     rawSize = actualSize = 0;
     for (const File& f : files)
     {
         rawSize += f.rawSize;
         actualSize += f.actualSize;
     }
}

void Folder::createDirectory(const char* path)
{
    mkdir(path, S_IRWXU | S_IRGRP | S_IWGRP);
}

void Folder::createDirectory(const std::string& path)
{
    createDirectory(path.c_str());
}

void Folder::open(bool forceupdate)
{
    ifstream f(getFilesDbPath(), ios_base::binary);
    vector<char> data((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    if (f.is_open())
        f.close();

    if (forceupdate)
        data.clear();

    if (data.empty())
    {
        if (type == FolderType::Archive)
        {
            createDirectory(dataPath()+"/archive/"+hash);
        }
        else if (type == FolderType::Source)
        {
            createDirectory(dataPath()+"/source/"+hash);
            files.clear();
            vector<string> filelist = listfiles(path.c_str(), 0);
            for (auto& file : filelist)
                files.push_back(File(this,normalizeFileName(path,file)));
        }
    }
    else
    {
        if (type == FolderType::Archive)
            createDirectory(dataPath()+"/archive/"+hash);
        else if (type == FolderType::Source)
            createDirectory(dataPath()+"/source/"+hash);

        auto it = data.cbegin();
        vector<vector<char>> filesData = deserializeConsume<decltype(filesData)>(it);
        for (const vector<char>& vec : filesData)
            files.push_back(File(this,vec));
    }

    isOpen = true;

    updateSizes();
}

void Folder::close()
{
    vector<char> data;
    vector<vector<char>> filesData;
    for (const File& f : files)
        filesData.push_back(f.serialize());
    serializeAppend(data, filesData);

    ofstream f(getFilesDbPath(), ios_base::binary | ios_base::trunc);
    if (f.is_open())
    {
        copy(begin(data), end(data), ostreambuf_iterator<char>(f));
        f.close();
    }
}

std::string Folder::getFilesDbPath() const
{
    if (type == FolderType::Source)
        return dataPath()+"/source/"+hash+"/files.dat";
    else if (type == FolderType::Archive)
        return dataPath()+"/archive/"+hash+"/files.dat";
    else
        throw std::runtime_error("Folder::getFilesDbPath: Unknown folder type");
}

std::string Folder::getFolderDataPath() const
{
    if (type == FolderType::Source)
        return dataPath()+"/source/"+hash;
    else if (type == FolderType::Archive)
        return dataPath()+"/archive/"+hash;
    else
        throw std::runtime_error("Folder::getFilesDbPath: Unknown folder type");
}

void Folder::removeData()
{
    if (type == FolderType::Source)
        deleteFolderRecursively((dataPath()+"/source/"+hash).c_str());
    else if (type == FolderType::Archive)
        deleteFolderRecursively((dataPath()+"/archive/"+hash).c_str());
    else
        throw std::runtime_error("Folder::removeData: Unknown folder type");
}

void Folder::writeArchiveFile(const std::vector<char>& data)
{
    open();

    cout << "Folder: got "<<data.size()<<" bytes"<<endl;

    auto it = data.cbegin();
    File fmeta(this, it);
    size_t metasize = fmeta.metadataSize();
    cout << "fmeta has "<<fmeta.rawSize<<" raw bytes, "<<fmeta.actualSize<<" actual bytes, "<<metasize<< " meta bytes and "
         <<data.size()-metasize<<" data bytes"<<endl;

    assert(data.size() == fmeta.rawSize + metasize);

    string newhash;
    sha512str(fmeta.path.c_str(), fmeta.path.size(), newhash);
    cout << "Hash is "<<newhash<<endl;
    string hashdirPath = getFolderDataPath()+"/"+newhash.substr(0,2);
    createDirectory(hashdirPath);

    string hashfilePath = hashdirPath+"/"+newhash.substr(2);
    FileLocker filel(hashfilePath);
    filel.overwrite(data.data()+metasize, fmeta.rawSize);

    files.push_back(fmeta);
    close();
}

std::string Folder::normalizePath(const std::string& folder)
{
    std::string cleanstr;
    char* clean = realpath(folder.c_str(), nullptr);
    if (clean)
    {
        cleanstr = string(clean);
    }
    else
    {
        cleanstr = folder;
        while (*cleanstr.rbegin() == '/')
            cleanstr.resize(cleanstr.size()-1);
    }

    free(clean);
    return cleanstr;
}

std::string Folder::normalizeFileName(const std::string& folder, const std::string& file)
{
    assert(!folder.empty() && !file.empty());
    string folderclean = normalizePath(folder);
    string fileclean = file;
    if (file.size() > folder.size()
            && file.find(folder) == 0)
    {
        fileclean = fileclean.substr(folder.size()+1);
    }
    return fileclean;
}
