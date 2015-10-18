#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

const std::string& dataPath();
const std::string& folderDBPath();
const std::string& nodeDBPath();
const std::string& serverConfigPath();

extern const int PORT_NUMBER;
extern const char* PORT_NUMBER_STR;

#endif // SETTINGS_H
