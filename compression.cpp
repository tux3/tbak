#include "compression.h"
#include <zlib.h>

Compression::Compression()
{

}

std::vector<char> Compression::deflate(const std::vector<char> in)
{
    std::vector<char> out;
    const size_t chunk = chunksize;
    unsigned char buf[chunk];
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (unsigned char *) in.data();
    strm.avail_in = in.size();
    deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED,
                window, memLevel, Z_DEFAULT_STRATEGY);
    do {
        int have;
        strm.avail_out = chunk;
        strm.next_out = buf;
        ::deflate(&strm, Z_FINISH);
        have = chunk - strm.avail_out;
        out.insert(end(out), buf, buf+have);
    } while (strm.avail_out == 0);
    deflateEnd (& strm);
    return out;
}

std::vector<char> Compression::inflate(const std::vector<char> in)
{
    std::vector<char> out;
    const size_t chunk = chunksize;
    unsigned char buf[chunk];
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (unsigned char *) in.data();
    strm.avail_in = in.size();
    inflateInit2(&strm, window);
    do {
            int have;
            strm.avail_out = chunk;
            strm.next_out = buf;
            ::inflate(&strm, Z_FINISH);
            have = chunk - strm.avail_out;
            out.insert(end(out), buf, buf+have);
        } while (strm.avail_out == 0);
        inflateEnd (& strm);
        return out;
}
