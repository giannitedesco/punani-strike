/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_ASSETFILE_H
#define _PUNANI_ASSETFILE_H

#define ASSETFILE_MAGIC	0x55daba01

#define RCMD_NORMAL_FLAG (1 << 15)
typedef uint16_t idx_t;
typedef uint16_t idx3_t[3];

/* asset file layout
 * [ header ]
 * [ struct asset_desc * h_num_assets] - sorted by name
 * [ float * D * h_verts] - vertices
 * [ float * D * h_verts] - normals
 * [ idx_t * h_num_indices ] - indices in to verts/norms arrays
*/

struct assetfile_hdr {
	uint32_t h_num_assets;
	uint32_t h_num_idx;
	uint32_t h_verts;
	uint32_t h_magic;
}__attribute__((packed));

#define ASSET_NAMELEN 32
struct asset_desc {
	uint8_t a_name[ASSET_NAMELEN];
	uint32_t a_off;
	uint32_t a_num_idx;
}__attribute__((packed));

/* Internal data structures */
struct _asset_file {
	struct list_head f_list;
	const struct assetfile_hdr *f_hdr;
	const struct asset_desc *f_desc;
	struct _asset **f_db;
	const uint8_t *f_buf;
	const float *f_verts;
	const float *f_norms;
	float *f_verts_ex;
	idx_t *f_idx_shadow;
	idx_t *f_idx_begin;
	char *f_name;
	vec3_t f_lightpos;
	size_t f_sz;
	unsigned int f_shadows_dirty;
	unsigned int f_ref;
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
