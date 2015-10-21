#ifndef SOURCE_H
#define SOURCE_H

#include <string>
#include <vector>
#include "sourcefile.h"

class Source
{
public:
    explicit Source(std::string path);
    const std::string& getPath() const;

    void populateCache() const; ///< Super slow, will recurse through the filesystem!
    uint64_t getSize() const; ///< Uses cached data
    const std::vector<SourceFile>& getSourceFiles() const; ///< Uses cached data
    /// Writes a source file from downloaded metadata and file data
    void restoreFile(const std::vector<char>& metadata, uint64_t mtime, const std::vector<char>& data);

private:
    void listFilesInto(const char *name, std::vector<std::string>& dest) const; ///< Lists files recursively
    void listFilesIntoInternal(char *namebuf, size_t basenameSize, std::vector<std::string>& dest) const;

private:
    std::string path;
    /// Cached data, populated on first use
    mutable std::vector<SourceFile> sourceFiles;
    mutable uint64_t size;
};

#endif // SOURCE_H
