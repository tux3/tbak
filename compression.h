#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <vector>
#include <cstddef>

class Compression
{
public:
    Compression() = delete;

    static std::vector<char> deflate(const std::vector<char>& in);
    static std::vector<char> inflate(const std::vector<char>& in);
};

#endif // COMPRESSION_H
