/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <ctype.h>
#include <math.h>

#include "list.h"
#include "hgang.h"
#include "assetfile.h"

static const char *cmd = "spankassets";

#define D 3

struct rcmd {
	struct list_head r_list;
	unsigned int r_idx;
	struct asset_vbo r_vbo;
};

struct asset {
	struct list_head a_list;
	struct list_head a_rcmd;
	char *a_name;
	unsigned int a_num_verts;
	unsigned int a_num_norms;
	unsigned int a_offset;
	float a_norm[D];
};

struct asset_list {
	struct list_head l_assets;
	hgang_t l_amem;
	hgang_t l_rmem;
	struct asset_vbo *l_verts;
	unsigned int l_num_assets;
	unsigned int l_num_verts;
	unsigned int l_num_idx;
};

static struct rcmd *rcmd_new(struct asset_list *l, struct asset *a)
{
	struct rcmd *r;

	r = hgang_alloc0(l->l_rmem);
	if ( NULL == r )
		goto out;

out:
	return r;
}

static void rcmd_free(struct asset_list *l, struct rcmd *r)
{
	if ( r ) {
		list_del(&r->r_list);
		hgang_return(l->l_rmem, r);
	}
}

static struct asset *asset_new(struct asset_list *l, const char *fn)
{
	struct asset *a;

	a = hgang_alloc0(l->l_amem);
	if ( NULL == a)
		goto out;

	a->a_name = strdup(fn);
	if ( NULL == a->a_name )
		goto out_free;

	INIT_LIST_HEAD(&a->a_rcmd);
	list_add_tail(&a->a_list, &l->l_assets);

	goto out;

out_free:
	free(a);
	a = NULL;
out:
	return a;
}

static void asset_free(struct asset_list *l, struct asset *a)
{
	if ( a ) {
		struct rcmd *r, *tmp;
		list_for_each_entry_safe(r, tmp, &a->a_rcmd, r_list) {
			rcmd_free(l, r);
		}
		list_del(&a->a_list);
		hgang_return(l->l_amem, a);
	}
}

/* Easy string tokeniser */
static int easy_explode(char *str, char split,
			char **toks, int max_toks)
{
	char *tmp;
	int tok;
	int state;

	for(tmp=str,state=tok=0; *tmp && tok <= max_toks; tmp++) {
		if ( state == 0 ) {
			if ( *tmp == split && (tok < max_toks)) {
				toks[tok++] = NULL;
			}else if ( !isspace(*tmp) ) {
				state = 1;
				toks[tok++] = tmp;
			}
		}else if ( state == 1 ) {
			if ( tok < max_toks ) {
				if ( *tmp == split || isspace(*tmp) ) {
					*tmp = '\0';
					state = 0;
				}
			}else if ( *tmp == '\n' )
				*tmp = '\0';
		}
	}

	return tok;
}

static int parse_float(const char *str, float *val)
{
	char *end;

	*val = strtod(str, &end);
	if ( end == str || (*end != '\0' && *end != 'f') )
		return 0;

	return 1;
}

static struct rcmd *parse_record(struct asset_list *l, struct asset *a,
				 char *str)
{
	struct rcmd *r;
	unsigned int i;
	char *tok[D];
	float vec[D];
	int ntok;

	ntok = easy_explode(str, 0, tok, D);
	if ( ntok != D ) {
		errno = 0;
		return NULL;
	}

	for(i = 0; i < D; i++) {
		if ( !parse_float(tok[i], &vec[i]) )
			return NULL;
	}

	r = rcmd_new(l, a);
	if ( NULL == r )
		return r;

	for(i = 0; i < D; i++) {
		r->r_vbo.v_vert[i] = (float)vec[i];
	}

	for(i = 0; i < D; i++) {
		r->r_vbo.v_norm[i] = a->a_norm[i];
	}

	list_add_tail(&r->r_list, &a->a_rcmd);
	return r;
}

static struct rcmd *rcmd_vert(struct asset_list *l, struct asset *a, char *str)
{
	struct rcmd *r;
	r = parse_record(l, a, str);
	if ( r )
		a->a_num_verts++;
	return r;
}

static int rcmd_norm(struct asset *a, char *str)
{
	unsigned int i;
	char *tok[D];
	float vec[D];
	int ntok;

	ntok = easy_explode(str, 0, tok, D);
	if ( ntok != D ) {
		errno = 0;
		return 0;
	}

	for(i = 0; i < D; i++) {
		if ( !parse_float(tok[i], &vec[i]) )
			return 0;
	}

	for(i = 0; i < D; i++) {
		a->a_norm[i] = vec[i];
	}

	return 1;
}

static int rip_file(struct asset_list *l, const char *fn)
{
	struct asset *a;
	FILE *fin;
	char buf[1024];
	char *ptr, *end;
	unsigned int line;
	int ret = 0;

	fin = fopen(fn, "rb");
	if ( NULL == fin ) {
		fprintf(stderr, "%s: %s: %s\n", cmd, fn, strerror(errno));
		goto out;
	}

	a = asset_new(l, fn);
	if ( NULL == a ) {
		fprintf(stderr, "%s: %s: asset_new: %s\n",
			cmd, fn, strerror(errno));
		goto out_free;
	}

	for(line = 1; fgets(buf, sizeof(buf), fin); line++ ) {
		struct rcmd *r;
		char *tok[2];
		int ntok;

		end = strchr(buf, '\r');
		if ( NULL == end )
			end = strchr(buf, '\n');

		if ( NULL == end ) {
			fprintf(stderr, "%s: %s:%u: Line too long\n",
				cmd, fn, line);
			goto out_free;
		}

		*end = '\0';

		/* strip trailing whitespace */
		for(end = end - 1; isspace(*end); end--)
			*end= '\0';
		/* strip leading whitespace */
		for(ptr = buf; isspace(*ptr); ptr++)
			/* nothing */;

		if ( *ptr == '\0' || *ptr == '#' )
			continue;

		ntok = easy_explode(ptr, 0, tok, 2);
		if ( ntok != 2 ) {
			fprintf(stderr, "%s: %s:%u: Parse error\n",
				cmd, fn, line);
			goto out_free;
		}

		if ( !strcmp(tok[0], "v") ) {
			r = rcmd_vert(l, a, tok[1]);
			if ( NULL == r )
				goto out_free;
		}else if ( !strcmp(tok[0], "n") ) {
			if ( !rcmd_norm(a, tok[1]) )
				goto out_free;
		}else{
			fprintf(stderr, "%s: %s:%u: unknown command '%s'\n",
				cmd, fn, line, tok[0]);
			goto out_free;
		}
	}

	l->l_num_assets++;
	ret = 1;
	goto out_close;

out_free:
	asset_free(l, a);
out_close:
	fclose(fin);
out:
	return ret;
}

static struct asset_list *asset_list_new(void)
{
	struct asset_list *l;

	l = calloc(1, sizeof(*l));
	if ( NULL == l )
		goto out;

	l->l_amem = hgang_new(sizeof(struct asset), 0);
	if ( NULL == l->l_amem )
		goto out_free;

	l->l_rmem = hgang_new(sizeof(struct rcmd), 0);
	if ( NULL == l->l_rmem )
		goto out_free_amem;

	/* success */
	INIT_LIST_HEAD(&l->l_assets);
	goto out;

//out_free_rmem:
//	hgang_free(l->l_rmem);
out_free_amem:
	hgang_free(l->l_amem);
out_free:
	free(l);
	l = NULL;
out:
	return l;
}

static void count_rcmd(struct asset_list *l)
{
	struct asset *a;

	l->l_num_verts = 0;

	list_for_each_entry(a, &l->l_assets, a_list) {
		l->l_num_verts += a->a_num_verts;
	}

	l->l_num_idx = l->l_num_verts;
}

static int vcmp(const void *A, const void *B)
{
	const struct asset_vbo *a = A;
	const struct asset_vbo *b = B;
	unsigned int i;

	for(i = 0; i < D; i++) {
		if ( a->v_vert[i] < b->v_vert[i] )
			return -1;
		if ( a->v_vert[i] > b->v_vert[i] )
			return 1;
	}

	for(i = 0; i < D; i++) {
		if ( a->v_norm[i] < b->v_norm[i] )
			return -1;
		if ( a->v_norm[i] > b->v_norm[i] )
			return 1;
	}

	return 0;
}

static unsigned int uniqify(struct asset_list *l, struct asset_vbo *v)
{
	struct asset *a;
	unsigned int n, i, cnt = 0;
	struct asset_vbo *out;

	list_for_each_entry(a, &l->l_assets, a_list) {
		struct rcmd *r;
		list_for_each_entry(r, &a->a_rcmd, r_list) {
			v[cnt] = r->r_vbo;
			cnt++;
		}
	}

	qsort(v, cnt, sizeof(*v), vcmp);
	for(out = v + 1, n = i = 1; i < cnt; i++) {
		struct asset_vbo *in = v + i;
		struct asset_vbo *prev = v + (i - 1);
		if ( memcmp(in, prev, sizeof(*v)) ) {
			memcpy(out, in, sizeof(*v));
			out++;
			n++;
		}
	}

	/* go and assign index values */
	list_for_each_entry(a, &l->l_assets, a_list) {
		struct rcmd *r;
		list_for_each_entry(r, &a->a_rcmd, r_list) {
			out = bsearch(&r->r_vbo, v, n, sizeof(r->r_vbo), vcmp);
			assert(out != NULL);
			r->r_idx = (out - v);
			assert(r->r_idx < n);
		}
	}

	return n;
}

static int indexify(struct asset_list *l)
{
	count_rcmd(l);

	printf("total_verts = %u\n", l->l_num_verts);

	l->l_verts = malloc(sizeof(*l->l_verts) * l->l_num_verts);
	if ( NULL == l->l_verts )
		return 0;

	l->l_num_verts = uniqify(l, l->l_verts);

	printf("num_verts = %u\n", l->l_num_verts);
	return 1;
}

static int acmp(const void *A, const void *B)
{
	const struct asset * const *a = A;
	const struct asset * const *b = B;
	return strcmp((*a)->a_name, (*b)->a_name);
}

static int sort_assets(struct asset_list *l)
{
	struct asset *a, *tmp, **s;
	unsigned int i = 0, off = 0;

	s = calloc(sizeof(*s), l->l_num_assets);
	if ( NULL == s )
		return 0;

	/* move from list to array */
	list_for_each_entry_safe(a, tmp, &l->l_assets, a_list) {
		list_del(&a->a_list);
		s[i++] = a;
	}

	/* sort */
	qsort(s, l->l_num_assets, sizeof(*s), acmp);

	/* put back in to list in sort-order */
	for(i = 0; i < l->l_num_assets; i++) {
		list_add_tail(&s[i]->a_list, &l->l_assets);
		s[i]->a_offset = off;
		off += s[i]->a_num_verts + s[i]->a_num_norms;
	}

	free(s);

	return 1;
}

static int write_hdr(struct asset_list *l, FILE *fout)
{
	struct assetfile_hdr h;

	h.h_num_assets = l->l_num_assets;
	h.h_num_idx = l->l_num_idx;
	h.h_verts = l->l_num_verts;
	h.h_magic = ASSETFILE_MAGIC;

	printf("Writing %lu byte header\n", sizeof(h));
	return (fwrite(&h, sizeof(h), 1, fout) == 1);
}

static int write_asset_descs(struct asset_list *l, FILE *fout)
{
	struct asset_desc d;
	struct asset *a;

	printf("Writing %lu byte asset descriptors:\n", sizeof(d));
	list_for_each_entry(a, &l->l_assets, a_list) {
		memset(&d, 0, sizeof(d));
		snprintf((char *)d.a_name, sizeof(d.a_name), "%s", a->a_name);
		d.a_off = a->a_offset;
		d.a_num_idx = a->a_num_verts;
		printf(" - %s\n", d.a_name);
		if ( fwrite(&d, sizeof(d), 1, fout) != 1 )
			return 0;
	}

	return 1;
}

static int write_geom(struct asset_list *l, FILE *fout)
{
	printf("Writing %lu bytes of geometry\n",
		l->l_num_verts * sizeof(*l->l_verts));
	if ( fwrite(l->l_verts, sizeof(*l->l_verts),
			l->l_num_verts, fout) != l->l_num_verts )
		return 0;

	return 1;
}

static int write_assets(struct asset_list *l, FILE *fout)
{
	struct asset *a;

	printf("Writing %lu bytes of render indices\n",
		sizeof(idx_t) * l->l_num_idx);

	list_for_each_entry(a, &l->l_assets, a_list) {
		struct rcmd *r;
		list_for_each_entry(r, &a->a_rcmd, r_list) {
			idx_t cmd;
			cmd = r->r_idx;
			if ( fwrite(&cmd, sizeof(cmd), 1, fout) != 1 )
				return 0;
		}
	}

	return 1;
}

static int asset_list_dump(struct asset_list *l, const char *fn)
{
	FILE *fout;

	if ( !indexify(l) )
		return 0;
	if ( !sort_assets(l) )
		return 0;

	fout = fopen(fn, "wb");
	if ( NULL == fout ) {
		fprintf(stderr, "%s: %s: %s\n",
			cmd, fn, strerror(errno));
		return 0;
	}

	if ( !write_hdr(l, fout) )
		goto write_err;
	if ( !write_asset_descs(l, fout) )
		goto write_err;
	if ( !write_geom(l, fout) )
		goto write_err;
	if ( !write_assets(l, fout) )
		goto write_err;

	fclose(fout);
	return 1;
write_err:
	fprintf(stderr, "%s: %s: fwrite: %s\n",
		cmd, fn, strerror(errno));
	fclose(fout);
	return 0;
}

static void asset_list_free(struct asset_list *l)
{
	if ( l ) {
		struct asset *a;
		list_for_each_entry(a, &l->l_assets, a_list) {
			free(a->a_name);
		}
		hgang_free(l->l_rmem);
		hgang_free(l->l_amem);
		free(l->l_verts);
		free(l);
	}
}

int main(int argc, char **argv)
{
	struct asset_list *l;
	int i;

	if ( argc )
		cmd = argv[0];

	if ( argc < 2 ) {
		fprintf(stderr, "Usage:\n\t%s <outfile> <infiles...>\n", cmd);
		return EXIT_FAILURE;
	}

	l = asset_list_new();
	if ( NULL == l ) {
		fprintf(stderr, "%s: %s: asset_list_new: %s\n",
			cmd, argv[1], strerror(errno));
		return EXIT_FAILURE;
	}

	for(i = 2; i < argc; i++) {
		if ( !rip_file(l, argv[i]) )
			return EXIT_FAILURE;
	}

	if ( !asset_list_dump(l, argv[1]) )
		return EXIT_FAILURE;
	asset_list_free(l);
	return EXIT_SUCCESS;
}
