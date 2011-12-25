/*
* This file is part of blackbloc
* Copyright (c) 2010 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Load TGA image files. Only uncompressed or RLE encoded 24bpp or
* 32bpp pixel formats are supported.
*/
#include <punani/punani.h>
#include <punani/blob.h>
#include <punani/tex.h>
#include "tex-internal.h"

/* On disk format */
struct tga {
	uint8_t		tga_identsize;
	uint8_t		tga_colormap_type;
	uint8_t		tga_image_type;

	uint16_t	tga_color_map_start;
	uint16_t	tga_color_map_len;
	uint8_t		tga_color_map_bits;

	uint16_t	tga_xstart;
	uint16_t	tga_ystart;
	uint16_t	tga_width;
	uint16_t	tga_height;
	uint8_t		tga_bpp;
	uint8_t		tga_descriptor;
} _packed;

/* Internal API */
struct _tga_img {
	struct _texture tex;
	const uint8_t *pixels;
};

static LIST_HEAD(tga_list);

#if 0
static int prep_rle(struct _texture *tex, unsigned int mip);
static int prep_uncompressed(struct _texture *tex, unsigned int mip);

static unsigned int tga_width(struct _texture *tex)
{
	struct _tga_img *tga = (struct _tga_img *)tex;
	return tga->tex.t_x;
}

static unsigned int tga_height(struct _texture *tex)
{
	struct _tga_img *tga = (struct _tga_img *)tex;
	return tga->tex.t_y;
}

static void tga_dtor(struct _texture *tex)
{
	struct _tga_img *tga = (struct _tga_img *)tex;
	list_del(&tga->tex.t_list);
	free(tga);
}

static const struct _texops ops_24 = {
	.prep = prep_uncompressed,
	.upload = teximg_upload_bgr,
	.dtor = tga_dtor,
	.width = tga_width,
	.height = tga_height,
};

static const struct _texops ops_32 = {
	.prep = prep_uncompressed,
	.upload = teximg_upload_bgra,
	.dtor = tga_dtor,
	.width = tga_width,
	.height = tga_height,
};

static const struct _texops ops_rle24 = {
	.prep = prep_rle,
	.unprep = teximg_unprep_generic,
	.upload = teximg_upload_rgb,
	.dtor = tga_dtor,
	.width = tga_width,
	.height = tga_height,
};

static const struct _texops ops_rle32 = {
	.prep = prep_rle,
	.unprep = teximg_unprep_generic,
	.upload = teximg_upload_rgba,
	.dtor = tga_dtor,
	.width = tga_width,
	.height = tga_height,
};

static int prep_uncompressed(struct _texture *tex, unsigned int mip)
{
	struct _tga_img *tga = (struct _tga_img *)tex;
	if ( mip )
		return 0;
	tga->tex.t_mipmap[mip].m_pixels = (uint8_t *)tga->pixels;
	tga->tex.t_mipmap[mip].m_height = tga->tex.t_y;
	tga->tex.t_mipmap[mip].m_width = tga->tex.t_x;
	return 1;
}

static int prep_rle(struct _texture *tex, unsigned int mip)
{
	struct _tga_img *tga = (struct _tga_img *)tex;
	const uint8_t *s_ptr;
	uint8_t r, g, b, a;
	uint8_t *buf, *ptr;
	size_t row, col;
	int count, rcount;
	unsigned char stride;

	if ( mip )
		return 0;

	stride = (tex->t_ops == &ops_rle24) ? 3 : 4;
	s_ptr = tga->pixels;

	ptr = buf = malloc(tga->tex.t_y * tga->tex.t_x * stride);
	if ( NULL == buf )
		return 0;

	for(r = b = g = a = row = col = 0, rcount = 1; row < tga->tex.t_y; ) {
		count = s_ptr[0];
		s_ptr++;
		if ( count & 0x80 )
			rcount = 1;
		count = 1 + (count & 0x7f);

		while ( count-- && row < tga->tex.t_y ) {
			if ( rcount-- > 0 ) {
				b = s_ptr[0];
				g = s_ptr[1];
				r = s_ptr[2];
				if ( stride > 3 )
					a = s_ptr[3];
				else
					a = 0xff;
				s_ptr += stride;
			}
			ptr[0] = r;
			ptr[1] = g;
			ptr[2] = b;
			if ( stride > 3 )
				ptr[3] = a;
			ptr += stride;
			if ( ++col == tga->tex.t_x ) {
				row++;
				col = 0;
			}
		}
	}

	con_printf("tga: %s: uncompressed %ubpp\n",
			tga->tex.t_name, stride * 8);
	tga->tex.t_mipmap[0].m_pixels = buf;
	tga->tex.t_mipmap[0].m_height = tga->tex.t_y;
	tga->tex.t_mipmap[0].m_width = tga->tex.t_x;

	return 1;
}
#endif

static struct _texture *do_tga_load(const char *name)
{
	uint8_t *b;
	size_t sz;
	struct _tga_img *tga;
	struct tga hdr;

	tga = calloc(1, sizeof(*tga));
	if ( tga == NULL )
		return NULL;

	b = blob_from_file(name, &sz);
	if ( NULL == b )
		goto err_free;

	if ( sz < sizeof(hdr) )
		goto err;

	/* Check the header out */
	memcpy(&hdr, b, sizeof(hdr));

	hdr.tga_color_map_start = le16toh(hdr.tga_color_map_start);
	hdr.tga_color_map_len = le16toh(hdr.tga_color_map_len);
	hdr.tga_xstart = le16toh(hdr.tga_xstart);
	hdr.tga_ystart = le16toh(hdr.tga_ystart);
	hdr.tga_width = le16toh(hdr.tga_width);
	hdr.tga_height = le16toh(hdr.tga_height);

	tga->tex.t_name = strdup(name);
	tga->tex.t_x = hdr.tga_width;
	tga->tex.t_y = hdr.tga_height;

	switch(hdr.tga_image_type) {
	case 2: /* non-indexed pixel map */
		switch(hdr.tga_bpp) {
		case 24:
			//tga->tex.t_ops = &ops_24;
			break;
		case 32:
			//tga->tex.t_ops = &ops_32;
			break;
		default:
			goto err;
		}
		break;
	case 10: /* RLE compressed */
		switch(hdr.tga_bpp) {
		case 24:
			//tga->tex.t_ops = &ops_rle24;
			break;
		case 32:
			//tga->tex.t_ops = &ops_rle32;
			break;
		default:
			goto err;
		}
		break;
	default:
		printf("hai %u\n", hdr.tga_image_type);
		goto err;
	}
	tga->pixels = b + sizeof(hdr) +
			hdr.tga_color_map_len + hdr.tga_identsize;

	if ( tga->tex.t_x == 0 || tga->tex.t_y == 0 )
		goto err;

	printf("tga: %s (%ux%u %ubpp)\n",
			tga->tex.t_name, tga->tex.t_x, tga->tex.t_y,
			hdr.tga_bpp);

	list_add_tail(&tga->tex.t_list, &tga_list);

	tex_get(&tga->tex);
	return &tga->tex;
err:
	blob_free(b, sz);
err_free:
	free(tga);
	return NULL;
}

texture_t tga_get_by_name(const char *name)
{
	struct _tga_img *tga;

	list_for_each_entry(tga, &tga_list, tex.t_list) {
		if ( !strcmp(name, tga->tex.t_name) ) {
			tex_get(&tga->tex);
			return &tga->tex;
		}
	}

	return do_tga_load(name);
}
