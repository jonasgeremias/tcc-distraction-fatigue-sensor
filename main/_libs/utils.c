#define ARRAY_SIZE_OF(a) (sizeof(a) / sizeof(a[0]))
#define MAKE8(a, i) ((uint8_t)(a >> (8 * (3 - i)) & 0xff))
#define MAKE32(a, b, c, d) (((uint32_t)((a)&0xff) << 24) | ((uint32_t)((b)&0xff) << 16) | ((uint32_t)((c)&0xff) << 8) | (uint32_t)((d)&0xff))
#define BIT_TEST(a, p) ((a & ((int)1 << p)) >> p)
#define BIT_SET(a, b) (a |= 1 << b)
#define BIT_CLEAR(a, b) (a &= ~(1 << b))

#include <stdio.h>
#include <string.h>