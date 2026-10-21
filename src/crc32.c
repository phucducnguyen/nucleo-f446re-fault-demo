#include "app.h"

/* CRC-32/ISO-HDLC, reflected, poly 0xEDB88320. Check value of "123456789" is 0xCBF43926. */
uint32_t crc32(const void *data, size_t len)
{
    const uint8_t *p = data;
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i + 1 < len; i++) {
        c ^= p[i];
        for (int b = 0; b < 8; b++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
    }
    return ~c;
}
