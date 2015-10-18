#ifndef NORMALIZEPATHS_H
#define NORMALIZEPATHS_H

#include <string>

std::string normalizePath(const std::string& folder);
std::string normalizeFileName(const std::string& folder, const std::string& file);
void createDirectory(const char* path);
void createDirectory(const std::string& path);
/// Create the necessary directory structure in folder base up to the file
void createPathTo(const std::string& base, const std::string& relfile);

#endif // NORMALIZEPATHS_H

