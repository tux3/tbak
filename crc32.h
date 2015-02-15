#ifndef CRC32_H
#define CRC32_H

/// compute CRC32 (Slicing-by-16 algorithm)
uint32_t crc32(const void* data, size_t length, uint32_t previousCrc32 = 0);

#endif // CRC32_H
