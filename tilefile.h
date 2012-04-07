/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_TILEFILE_H
#define _PUNANI_TILEFILE_H

/* Tile file format:
 * [ hdr ]
 * [ h_num_assets * assset names ]
 * [ h_num_items * struct tile_item ]
*/

#define TILEFILE_MAGIC	0x55d4b400

#define TILEFILE_NAMELEN 32

struct tile_hdr {
	uint32_t h_num_assets;
	uint32_t h_num_items;
	uint32_t h_magic;
}__attribute__((packed));

struct tile_item {
	uint8_t i_asset;
	uint8_t i_flags;
	int16_t i_x;
	int16_t i_y;
}__attribute__((packed));

#endif /* _PUNANI_TILEFILE_H */
