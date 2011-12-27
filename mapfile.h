#ifndef _MAPFILE_H
#define _MAPFILE_H

#define MAPFILE_MAGIC ('P' | (0x55 << 8) | ('M' << 16) | (1 << 24))

/* all values little endian */
struct mapfile_hdr {
	uint32_t	magic;

	/* describes the map */
	uint16_t	xtiles;
	uint16_t	ytiles;

	uint16_t 	reserved;

	/* describes the tilemap PNG */
	uint8_t		tile_width;
	uint8_t		tile_height;
	uint8_t		tilemap[16];
}__attribute__((packed));

/* followed by xtiles * ytiles uint16_t tile ID's */

#endif /* _MAPFILE_H */
