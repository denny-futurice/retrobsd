/*
 * Partition table parsers shared by the disk drivers (D36).
 */
#ifndef _MACHINE_DISKLABEL_H_
#define _MACHINE_DISKLABEL_H_

#define NPARTITIONS     4               /* MBR primary partitions */
#define RAWPART         0               /* the whole disk */

/*
 * Fill part[1..NPARTITIONS] from the MBR in sector, a 512-byte buffer.
 * Return 0 and clear them if there is no MBR.
 */
int mbr_parse(const u_char *sector, struct diskpart *part);

#endif /* _MACHINE_DISKLABEL_H_ */
