/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_ASSETFILE_H
#define _PUNANI_ASSETFILE_H

#define ASSETFILE_MAGIC	0x55daba00

#define ASSET_USE_FLOAT 1
#if ASSET_USE_FLOAT
typedef float fp_t;
#else
#define FP_MIN (-32768)
#define FP_MAX (32767)
typedef int16_t fp_t;
#endif

#define RCMD_NORMAL_FLAG (1 << 15)
typedef uint16_t idx_t;

/* asset file layout
 * [ header ]
 * [ struct asset_desc * h_num_assets] - sorted by name
 * [ fp_t * D * h_verts] - vertices
 * [ fp_t * D * h_verts] - normals
 * [ idx_t * h_num_assets * a_len ] - indices in to verts/norms arrays
*/

struct assetfile_hdr {
	uint32_t h_num_assets;
	uint32_t h_verts;
	uint32_t h_magic;
}__attribute__((packed));

#define ASSET_NAMELEN 32
struct asset_desc {
	uint8_t a_name[ASSET_NAMELEN];
	uint32_t a_off;
	uint32_t a_num_idx;
}__attribute__((packed));

static inline fp_t float_to_fp(float f)
{
#if ASSET_USE_FLOAT
	return f;
#else
	float a, v;

	assert(f >= -1.0 && f <= 1.0);

	a = fabs(f);
	if ( a != f ) {
		v = -v;
		v = (float)(-FP_MIN) / a;
	}else{
		v = (float)FP_MAX / a;
	}

	return (fp_t)v;
#endif
}

static inline float fp_to_float(fp_t fp)
{
#if ASSET_USE_FLOAT
	return fp;
#else
	float v = fp;
	if ( fp < 0 ) {
		v = v / (float)(-FP_MIN);
	}else{
		v = v / (float)(FP_MAX);
	}
	return v;
#endif
}

/* Internal data structures */
struct _asset_file {
	struct list_head f_list;
	const struct assetfile_hdr *f_hdr;
	const struct asset_desc *f_desc;
	struct _asset **f_db;
	const uint8_t *f_buf;
	const fp_t *f_verts;
	const float *f_norms;
	idx_t *f_idx_begin;
	char *f_name;
	size_t f_sz;
	unsigned int f_ref;
};

struct _asset {
	struct _asset_file *a_owner;
	const uint16_t *a_indices;
	unsigned int a_idx;
	unsigned int a_ref;
};

#endif /* _PUNANI_ASSETFILE_H */
