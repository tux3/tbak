#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <vector>
#include <cstddef>

class Compression
{
public:
    Compression();

    static std::vector<char> deflate(const std::vector<char> in);
    static std::vector<char> inflate(const std::vector<char> in);

private:
    static const int window = -15;
    static const size_t chunksize = 4096;
    static const unsigned memLevel = 9;
};

#endif // COMPRESSION_H
