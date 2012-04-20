/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_MAPFILE_H
#define _PUNANI_MAPFILE_H

/* Tile file format:
 * [ hdr ]
 * [ h_num_tiles * tile names ]
 * [ h_x * h_y indices ]
*/

#define MAPFILE_MAGIC	0x55d45400

#define MAPFILE_NAMELEN 32

struct map_hdr {
	uint32_t h_num_tiles;
	uint32_t h_x;
	uint32_t h_y;
	uint32_t h_magic;
}__attribute__((packed));

#define MAP_IDX_MAX 0xffff
typedef uint16_t midx_t;

/* in-core data structures */
#if MAP_INTERNAL
struct _map {
	unsigned int m_x;
	unsigned int m_y;
	unsigned int m_num_tiles;
	struct _tile *m_tile;
	midx_t *m_indices;
};
#endif

#endif /* _PUNANI_MAPFILE_H */
