#include "source.h"
#include "util/pathtools.h"
#include <dirent.h>
#include <cstring>
#include <cassert>

using namespace std;

Source::Source(std::string path)
    : path{path}, size{0}
{

}

std::string Source::getPath() const
{
    return path;
}

void Source::populateCache() const
{
    sourceFiles.clear();
    size = 0;
    vector<string> filelist = listFiles(path.c_str(), 0);
    for (auto& file : filelist)
        sourceFiles.push_back(SourceFile(this, normalizeFileName(path,file)));

    for (auto& file : sourceFiles)
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

std::vector<std::string> Source::listFiles(const char *name, int level) const
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
            char path[PATH_MAX];
            int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
            path[len] = 0;
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            vector<string> newfiles = listFiles(path, level + 1);
            files.insert(end(files), begin(newfiles), end(newfiles));
        }
        else if (entry->d_type == DT_REG)
        {
            char path[PATH_MAX];
            int len = snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            std::string s(path, len);
            files.push_back(s);
        }
    } while ((entry = readdir(dir)));
    closedir(dir);
    return files;
}
