#include "compression.h"
#include <zlib.h>
#include <brotli/dec/decode.h>
#include <brotli/enc/encode.h>
#include <stdexcept>

using namespace std;
using namespace brotli;

class BrotliVecOut : public brotli::BrotliOut
{
public:
    BrotliVecOut(std::vector<char>& out) : out{out} {}
    bool Write(const void* buf, size_t n)
    {
        out.insert(out.end(), (char*)buf, (char*)buf+n);
        return true;
    }

private:
    std::vector<char>& out;
};

std::vector<char> Compression::deflate(const std::vector<char>& in)
{
    std::vector<char> out;
    BrotliParams params;
    params.quality = 7;
    BrotliMemIn brotliIn(in.data(), in.size());
    BrotliVecOut brotliOut(out);
    if (!BrotliCompress(params, &brotliIn, &brotliOut))
        throw std::runtime_error("Failed to compress");
    return out;
}

std::vector<char> Compression::inflate(const std::vector<char>& in)
{
    std::vector<char> out;
    out.reserve(in.size());
    BrotliMemInput brotliMemIn;
    BrotliInput brotliIn = BrotliInitMemInput((uint8_t*)in.data(), in.size(), &brotliMemIn);
    BrotliOutput brotliOut;
    brotliOut.data_ = &out;
    brotliOut.cb_ = [](void* _data, const uint8_t* buf, size_t count) -> int
    {
        vector<char>* data = static_cast<vector<char>*>(_data);
        data->insert(data->end(), buf, buf+count);
        return count;
    };

    if (!BrotliDecompress(brotliIn, brotliOut))
        throw std::runtime_error("Failed to decompress");
    return out;
}
