#ifndef SOURCE_H
#define SOURCE_H

#include <string>
#include <vector>
#include "sourcefile.h"

class Source
{
public:
    Source(std::string path);
    std::string getPath() const;

    void populateCache() const; ///< Super slow, will recurse through the filesystem!
    uint64_t getSize() const; ///< Uses cached data
    const std::vector<SourceFile>& getSourceFiles() const; ///< Uses cached data
    /// Writes a source file from downloaded metadata and file data
    void restoreFile(const std::vector<char>& metadata, uint64_t mtime, const std::vector<char>& data);

private:
    std::vector<std::string> listFiles(const char *name, int level) const; ///< Lists files recursively

private:
    std::string path;
    /// Cached data, populated on first use
    mutable std::vector<SourceFile> sourceFiles;
    mutable uint64_t size;
};

#endif // SOURCE_H
