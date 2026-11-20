/*
 * htonl, ntohl: the host byte order is the network byte order (D37).
 */
#include <arpa/inet.h>

unsigned
htonl(unsigned x)
{
    return x;
}

unsigned
ntohl(unsigned x)
{
    return x;
}
