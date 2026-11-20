/*
 * Byte order (D37): <machine/endian.h> says big-endian, and so does the
 * layout of a long in memory; htonl and ntohl return their argument,
 * htons and ntohs its low 16 bits.  A value converted with htonl or htons
 * has its most significant byte first in memory, as the network expects.
 */
#include "libtest.h"
#include <arpa/inet.h>
#include <machine/endian.h>

int
main(void)
{
    union {
        unsigned long l;
        unsigned short s;
        unsigned char c[4];
    } u;

    console();
    check(BYTE_ORDER == BIG_ENDIAN);
    check(LITTLE_ENDIAN == 1234 && BIG_ENDIAN == 4321 && PDP_ENDIAN == 3412);

    u.l = 0x11223344;
    check(u.c[0] == 0x11 && u.c[1] == 0x22 && u.c[2] == 0x33 && u.c[3] == 0x44);

    check(htonl(0x11223344) == 0x11223344);
    check(ntohl(0x11223344) == 0x11223344);
    check(htonl(0x80000001) == 0x80000001);
    check(htons(0x1234) == 0x1234);
    check(ntohs(0x1234) == 0x1234);
    check(htons(0xabcd1234) == 0x1234);
    check(ntohs(0xabcd1234) == 0x1234);

    u.l = htonl(0xc0a80001);                    /* 192.168.0.1 */
    check(u.c[0] == 192 && u.c[1] == 168 && u.c[2] == 0 && u.c[3] == 1);
    u.s = htons(80);
    check(u.c[0] == 0 && u.c[1] == 80);

    finish("libendian");
    return 0;
}
