#include "humanreadable.h"
#include <cstdlib>

std::string humanReadableSize(double size)
{
    char buf[16];
    int i = 0;
    const char* units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    while (size > 1024) {
        size /= 1024;
        i++;
    }
    sprintf(buf, "%.*f %s", i, size, units[i]);
    return std::string(buf);
}
