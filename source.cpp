#include "source.h"
#include "util/pathtools.h"
#include <dirent.h>
#include <cstring>
#include <cassert>
#include <iostream>

using namespace std;

Source::Source(std::string path)
    : path{path}, size{0}
{

}

const string &Source::getPath() const
{
    return path;
}

void Source::populateCache() const
{
    sourceFiles.clear();
    size = 0;
    vector<string> filelist;
    listFilesInto(path.c_str(), filelist);
    sourceFiles.reserve(filelist.size());

    // Precook a buffer to store full paths
    char fullpath[PATH_MAX];
    strcpy(fullpath, path.c_str());
    fullpath[path.size()] = '/';
    size_t fpathSize = path.size()+1;

    for (auto& file : filelist)
    {
        strcpy(fullpath+fpathSize, file.c_str());
        sourceFiles.emplace_back(this, move(file), fullpath);
    }

    for (const auto& file : sourceFiles)
        size += file.getRawSize();
}

uint64_t Source::getSize() const
{
    if (!sourceFiles.size())
        populateCache();

    return size;
}

const std::vector<SourceFile> &Source::getSourceFiles() const
{
    if (!sourceFiles.size())
        populateCache();

    return sourceFiles;
}

void Source::restoreFile(const std::vector<char> &metadata, uint64_t mtime, const std::vector<char> &data)
{
    sourceFiles.emplace_back(this, metadata, mtime, data);
}

void Source::listFilesInto(const char *name, std::vector<string> &dest) const
{
    size_t namelen = strlen(name);
    assert(name[namelen-1] != '/');
    DIR *dir;
    struct dirent *entry;

    // Precook a buffer to write dirent full paths in
    char path[PATH_MAX];
    strcpy(path, name);
    path[namelen] = '/';
    ++namelen;
    size_t sizeLeft = PATH_MAX-namelen;

    if (!(dir = opendir(name)))
        return;
    if (!(entry = readdir(dir)))
        return;

    do
    {
        if (entry->d_type == DT_REG)
        {
            dest.emplace_back(entry->d_name);
        }
        else if (entry->d_type == DT_DIR)
        {
            if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0'
                || (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
                continue;
            if (strlen(entry->d_name) > sizeLeft)
                continue;
            strcpy(path+namelen, entry->d_name);
            listFilesIntoInternal(path, namelen, dest);
        }
    } while ((entry = readdir(dir)));
    closedir(dir);
    return;
}

void Source::listFilesIntoInternal(char *namebuf, size_t basenameSize, std::vector<string> &dest) const
{
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(namebuf)))
        return;
    if (!(entry = readdir(dir)))
        return;

    // Precook a buffer to write dirent full paths in
    size_t namelen = strlen(namebuf);
    namebuf[namelen] = '/';
    ++namelen;
    size_t sizeLeft = PATH_MAX-namelen;

    do
    {
        if (entry->d_type == DT_REG)
        {
            if (strlen(entry->d_name) > sizeLeft)
                continue;
            strcpy(namebuf+namelen, entry->d_name);
            dest.emplace_back(namebuf+basenameSize);
        }
        else if (entry->d_type == DT_DIR)
        {
            if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0'
                || (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
                continue;
            if (strlen(entry->d_name) > sizeLeft)
                continue;
            strcpy(namebuf+namelen, entry->d_name);
            listFilesIntoInternal(namebuf, basenameSize, dest);
        }
    } while ((entry = readdir(dir)));
    closedir(dir);
    return;
}
