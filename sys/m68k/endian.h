/*
 * Byte order of the 68030 (D37): big-endian, the network byte order, so
 * htonl, htons, ntohl and ntohs (libc, <arpa/inet.h>) do not swap bytes.
 * The values are those of 4.4BSD.
 */
#ifndef _MACHINE_ENDIAN_H_
#define _MACHINE_ENDIAN_H_

#define LITTLE_ENDIAN   1234    /* least significant byte first */
#define BIG_ENDIAN      4321    /* most significant byte first */
#define PDP_ENDIAN      3412    /* LSB first in word, MSW first in long */

#define BYTE_ORDER      BIG_ENDIAN

#endif /* _MACHINE_ENDIAN_H_ */
