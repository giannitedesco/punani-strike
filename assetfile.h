/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_ASSETFILE_H
#define _PUNANI_ASSETFILE_H

#define ASSETFILE_MAGIC	0x55daba03

typedef uint16_t idx_t;

/* asset file layout
 * [ header ]
 * [ struct asset_desc * h_num_assets] - sorted by name
 * [ asset_vbo * h_verts] - vertices
 * [ idx_t * h_num_indices ] - indices in to verts/norms arrays
*/

struct assetfile_hdr {
	uint32_t h_num_assets;
	uint32_t h_num_idx;
	uint32_t h_verts;
	uint32_t h_magic;
}__attribute__((packed));

struct asset_vbo {
	float v_vert[3];
	float v_norm[3];
	uint8_t v_rgba[4];
	int16_t v_st[2];
}__attribute__((packed));

#define ASSET_NAMELEN 32
struct asset_desc {
	uint8_t a_name[ASSET_NAMELEN];
	uint32_t a_off;
	uint32_t a_num_idx;
	uint32_t a_flags;
	uint8_t a_rgba[4];
	float a_mins[3];
	float a_maxs[3];
}__attribute__((packed));

/* Internal data structures */
struct _asset_file {
	struct list_head f_list;
	const struct assetfile_hdr *f_hdr;
	const struct asset_desc *f_desc;
	struct _asset **f_db;
	const uint8_t *f_buf;
	const struct asset_vbo *f_verts;
	float *f_verts_ex;
	idx_t *f_idx_shadow;
	idx_t *f_idx_begin;
	char *f_name;
	vec3_t f_lightpos;
	size_t f_sz;
	unsigned int f_shadows_dirty;
	unsigned int f_ref;
	unsigned int f_num_indices;
	unsigned int f_shadow_indices;

	unsigned int f_vbo_geom;
	unsigned int f_ibo_geom;

	unsigned int f_vbo_shadow;
	unsigned int f_ibo_shadow;
};

struct _asset {
	struct _asset_file *a_owner;
	const idx_t *a_indices;
	idx_t *a_shadow_idx;
	unsigned int a_idx;
	unsigned int a_ref;
	unsigned int a_num_shadow_idx;
};

#endif /* _PUNANI_ASSETFILE_H */
