#include <punani/punani.h>
#include <punani/tex.h>
#include "tex-internal.h"

void tex_get(struct _texture *tex)
{
	tex->t_ref++;
}
