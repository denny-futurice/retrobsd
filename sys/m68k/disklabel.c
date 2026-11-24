/*
 * Partition tables (D36).  Only the MBR for now; an Amiga RDB parser
 * belongs here too (M10).
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/disk.h>
#include <machine/disklabel.h>

#define MBR_TABLE       446             /* offset of the 4 entries */
#define MBR_ENTRY       16
#define MBR_MAGIC       510             /* 0x55, 0xaa */

/* A little-endian 32-bit field. */
static u_int
get_le32(const u_char *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((u_int) p[3] << 24);
}

int
mbr_parse(const u_char *sector, struct diskpart *part)
{
    const u_char *e;
    int i;

    bzero(&part[1], NPARTITIONS * sizeof(*part));
    if (sector[MBR_MAGIC] != 0x55 || sector[MBR_MAGIC + 1] != 0xaa)
        return 0;
    for (i = 1; i <= NPARTITIONS; i++) {
        e = sector + MBR_TABLE + (i - 1) * MBR_ENTRY;
        part[i].dp_status = e[0];
        bcopy(e + 1, part[i].dp_start_chs, 3);
        part[i].dp_type = e[4];
        bcopy(e + 5, part[i].dp_end_chs, 3);
        part[i].dp_offset = get_le32(e + 8);
        part[i].dp_nsectors = get_le32(e + 12);
    }
    return 1;
}
