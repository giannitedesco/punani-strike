/*
* This file is part of blackbloc
* Copyright (c) 2003-2011 Gianni Tedesco
* Released under the terms of the GNU GPL version 2
*
* Load PNG image files. PNG is a standard image format which
* supports lossless compression and alpha channel amongst other
* nifty features.
*
* TODO:
*  o Libpng error handling crap
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/blob.h>
#include "tex-internal.h"

#include <png.h>

struct _png_img {
	struct _texture tex;
	struct list_head list;
	char *name;
};

static LIST_HEAD(png_list);

struct png_io_ffs {
	uint8_t *ptr;
	size_t sz;
	size_t fptr;
};

/* Replacement read function for libpng */
static void png_read_data_fn(png_structp pngstruct, png_bytep data, png_size_t len)
{
	struct png_io_ffs *ffs = png_get_io_ptr(pngstruct);

	if ( ffs->fptr > ffs->sz )
		return;
	if ( ffs->fptr + len > ffs->sz )
		len = ffs->sz - ffs->fptr;
	memcpy(data, ffs->ptr + ffs->fptr, len);
	ffs->fptr += len;
}

static void dtor(struct _texture *t)
{
	struct _png_img *png = (struct _png_img *)t;
	free(png->name);
	list_del(&png->list);
	free(png);
}

static struct _texture *do_png_load(renderer_t r, const char *name)
{
	struct _png_img *png;
	png_structp pngstruct;
	png_infop info = NULL;
	int bits, color, inter;
	png_uint_32 w, h;
	unsigned int x, rb;
	struct png_io_ffs ffs;
	uint8_t *pixels;

	pngstruct = png_create_read_struct(PNG_LIBPNG_VER_STRING,
						NULL, NULL, NULL);
	if ( NULL == pngstruct )
		goto err;

	info = png_create_info_struct(pngstruct);
	if ( NULL == info )
		goto err_png;

	png = calloc(1, sizeof(*png));
	if ( NULL == png )
		goto err_png;

	tex_init(&png->tex, r);

	ffs.ptr = blob_from_file(name, &ffs.sz);
	ffs.fptr = 0;

	if ( NULL == ffs.ptr )
		goto err_img;

	if ( ffs.sz < 8 || png_sig_cmp((void *)ffs.ptr, 0, 8) ) {
		printf("pngstruct: %s: bad signature\n", name);
		goto err_close;
	}

	png_set_read_fn(pngstruct, &ffs, png_read_data_fn);
	png_read_info(pngstruct, info);

	/* Get the information we need */
	png_get_IHDR(pngstruct, info, &w, &h, &bits,
			&color, &inter, NULL, NULL);

	/* o Must be power of two in dimensions (for OpenGL)
	 * o Must be 8 bits per pixel
	 * o Must not be interlaced
	 * o Must be either RGB or RGBA
	 */
	if ( bits != 8 || inter != PNG_INTERLACE_NONE ||
		(color != PNG_COLOR_TYPE_RGB &&
		 color != PNG_COLOR_TYPE_RGB_ALPHA &&
		 color != PNG_COLOR_TYPE_PALETTE) ) {
		printf("pngstruct: %s: bad format (%ux%ux%ibit) %i\n",
			name, (unsigned)w, (unsigned)h, bits, color);
		goto err_close;
	}

	if ( color == PNG_COLOR_TYPE_PALETTE )
		png_set_palette_to_rgb(pngstruct);

	png_read_update_info(pngstruct, info);

	/* Allocate buffer and read image */
	rb = png_get_rowbytes(pngstruct, info);
	if (color & PNG_COLOR_MASK_ALPHA) {
		if ( !tex_alloc_rgba(&png->tex, w, h) )
			goto err_close;
	}else{
		if ( !tex_alloc_rgb(&png->tex, w, h) )
			goto err_close;
	}

	tex_lock(&png->tex);
	pixels = tex_pixels(&png->tex);
	for(x = 0; x < h; x++) {
		uint8_t *row = pixels + (x * rb);
		png_read_row(pngstruct, row, NULL);
	}
	tex_unlock(&png->tex);

	png->name = strdup(name);
	if ( NULL == png->name )
		goto err_free_buf;

	png->tex.t_y = h;
	png->tex.t_x = w;
	png->tex.t_dtor = dtor;

	png_read_end(pngstruct, info);
	png_destroy_read_struct(&pngstruct, &info, NULL);
	blob_free(ffs.ptr, ffs.sz);

	list_add_tail(&png->list, &png_list);

//	printf("png: %s: loaded (%u x %u x %ibit)\n",
//		name, (unsigned)w, (unsigned)h, bits);
	tex_get(&png->tex);
	return &png->tex;

err_free_buf:
	tex_free(&png->tex);
err_close:
	blob_free(ffs.ptr, ffs.sz);
err_img:
	free(png);
err_png:
	png_destroy_read_struct(&pngstruct, &info, NULL);
err:
	return NULL;
}

texture_t png_get_by_name(renderer_t r, const char *name)
{
	struct _png_img *png;

	list_for_each_entry(png, &png_list, list) {
		if ( !strcmp(name, png->name) ) {
			tex_get(&png->tex);
			return &png->tex;
		}
	}

	return do_png_load(r, name);
}
