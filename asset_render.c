/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/renderer.h>
#include <punani/asset.h>
#include <punani/blob.h>
#include <math.h>

#include "assetfile.h"

#include <GL/gl.h>

void asset_file_render_begin(asset_file_t f)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
#if ASSET_USE_FLOAT
	glVertexPointer(3, GL_FLOAT, 0, f->f_verts);
#else
#endif
	glNormalPointer(GL_FLOAT, 0, f->f_norms);
}

void asset_file_render_end(asset_file_t f)
{
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void asset_render(asset_t a)
{
	const struct asset_desc *d = a->a_owner->f_desc + a->a_idx;
	glDrawElements(GL_TRIANGLES, d->a_num_idx,
			GL_UNSIGNED_SHORT, a->a_indices);
}
