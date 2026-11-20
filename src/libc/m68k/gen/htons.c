/*
 * htons, ntohs: the host byte order is the network byte order (D37); only
 * the low 16 bits of the argument are a short.
 */
#include <arpa/inet.h>

unsigned
htons(unsigned x)
{
    return x & 0xffff;
}

unsigned
ntohs(unsigned x)
{
    return x & 0xffff;
}
