#ifndef _TEXTURE_INTERNAL_H
#define _TEXTURE_INTERNAL_H

#include "list.h"

struct _texture {
	struct list_head t_list;
	const char *t_name;
	unsigned int t_x;
	unsigned int t_y;
	unsigned int t_ref;
};

void tex_get(struct _texture *tex);

#endif /* _TEXTURE_INTERNAL_H */
