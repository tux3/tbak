#ifndef COMMANDS_H
#define COMMANDS_H

#include <string>

// Client command handlers
namespace cmd
{

void help();
void folderShow();
void folderRemoveSource(const std::string& path);
void folderRemoveArchive(const std::string& path);
void folderAddSource(const std::string& path);
bool folderAddArchive(const std::string& path);
bool folderPush(const std::string& path);
void folderUpdate(const std::string& path);
void folderStatus(const std::string& path);
bool folderSync(const std::string& path);
bool folderRestore(const std::string& path);
void nodeShow();
void nodeShowkey();
void nodeAdd(const std::string& uri);
void nodeAdd(const std::string& uri, const std::string& pk);
void nodeRemove(const std::string& uri);
bool nodeStart();

}

#endif // COMMANDS_H
