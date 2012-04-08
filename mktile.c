/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "list.h"

#include "tilefile.h"

static const char *cmd = "mktile";

struct item {
	struct list_head i_list;
	char *i_name;
	unsigned int i_idx;
	float i_x;
	float i_y;
};

struct tile {
	struct list_head t_items;
	char **t_assets;
	unsigned int t_num_items;
	unsigned int t_num_assets;
};

static struct tile *tile_new(void)
{
	struct tile *t;

	t = calloc(1, sizeof(*t));
	if ( NULL == t)
		goto out;

	INIT_LIST_HEAD(&t->t_items);
	/* success */

out:
	return t;
}

static void tile_free(struct tile *t)
{
	if ( t ) {
		struct item *item, *tmp;
		list_for_each_entry_safe(item, tmp, &t->t_items, i_list) {
			list_del(&item->i_list);
			free(item->i_name);
			free(item);
		}
		free(t->t_assets);
		free(t);
	}
}

static int ncmp(const void *A, const void *B)
{
	const char * const *a = A;
	const char * const *b = B;
	return strcmp(*a, *b);
}

static int indexify(struct tile *t)
{
	struct item *item;
	unsigned int i, j;
	char **ret;

	ret = malloc(t->t_num_items * sizeof(*ret));
	if ( ret == NULL )
		return 0;

	i = 0;
	list_for_each_entry(item, &t->t_items, i_list) {
		ret[i] = item->i_name;
		i++;
	}

	qsort(ret, t->t_num_items, sizeof(*ret), ncmp);

	for(j = i = 1; i < t->t_num_items; i++) {
		if ( strcmp(ret[j], ret[i]) ) {
			ret[j] = ret[i];
			j++;
		}
	}

	t->t_assets = ret;
	t->t_num_assets = j;

	list_for_each_entry(item, &t->t_items, i_list) {
		char *key, **val;
		key = item->i_name;
		val = bsearch(&key, ret, t->t_num_assets, sizeof(*ret), ncmp);
		assert(val != NULL);
		item->i_idx = val - ret;
	}

	return 1;
}

static int tile_dump(struct tile *t, const char *fn)
{
	char name[TILEFILE_NAMELEN];
	struct tile_hdr hdr;
	struct tile_item x;
	struct item *item;
	unsigned int i;
	FILE *fout;

	if ( !indexify(t) )
		goto err;

	hdr.h_num_assets = t->t_num_assets;
	hdr.h_num_items = t->t_num_items;
	hdr.h_magic = TILEFILE_MAGIC;

	fout = fopen(fn, "w");
	if ( NULL == fout ) {
		fprintf(stderr, "%s: %s: %s\n",
			cmd, fn, strerror(errno));
		return 0;
	}

	printf("Writing %ld byte header\n", sizeof(hdr));
	if ( fwrite(&hdr, sizeof(hdr), 1, fout) != 1 )
		goto err_close;

	printf("Writing %u asset names\n", t->t_num_assets);
	for(i = 0; i < t->t_num_assets; i++) {
		memset(name, 0, TILEFILE_NAMELEN);
		snprintf(name, sizeof(name), "%s", t->t_assets[i]);
		printf(" - %s\n", name);
		if ( fwrite(name, sizeof(name), 1, fout) != 1 )
			goto err_close;
	}

	printf("Writing %u x %ld byte items\n", t->t_num_items, sizeof(x));
	list_for_each_entry(item, &t->t_items, i_list) {
		x.i_asset = item->i_idx;
		x.i_flags = 0;
		x.i_x = item->i_x;
		x.i_y = item->i_y;
		if ( fwrite(&x, sizeof(x), 1, fout) != 1 )
			goto err_close;
	}

	fclose(fout);
	return 1;
err_close:
	fclose(fout);
err:
	return 0;
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

static struct item *rip_asset(struct tile *t, char *str)
{
	struct item *item = NULL;
	char *tok[3];
	int ntok;

	ntok = easy_explode(str, 0, tok, 3);
	if ( ntok != 3 )
		goto out;

	item = calloc(1, sizeof(*item));
	if ( NULL == item )
		goto out;

	item->i_name = strdup(tok[2]);
	if ( NULL == item->i_name )
		goto out_free;

	parse_float(tok[0], &item->i_x);
	parse_float(tok[1], &item->i_y);

	t->t_num_items++;
	list_add_tail(&item->i_list, &t->t_items);
	goto out;
out_free:
	free(item);
	item = NULL;
out:
	return item;
}

static struct tile *rip_file(const char *fn)
{
	struct tile *t = NULL;
	FILE *fin;
	char buf[1024];
	char *ptr, *end;
	unsigned int line;

	fin = fopen(fn, "r");
	if ( NULL == fin ) {
		fprintf(stderr, "%s: %s: %s\n", cmd, fn, strerror(errno));
		goto out_close;
	}

	t = tile_new();
	if ( NULL == t ) {
		fprintf(stderr, "%s: %s: tile_new: %s\n",
			cmd, fn, strerror(errno));
		goto out_free;
	}

	for(line = 1; fgets(buf, sizeof(buf), fin); line++ ) {
		struct item *item;
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

		if ( !strcmp(tok[0], "a") ) {
			item = rip_asset(t, tok[1]);
		}else{
			fprintf(stderr, "%s: %s:%u: unknown command '%s'\n",
				cmd, fn, line, tok[0]);
			goto out_free;
		}

		if ( NULL == item )
			goto out_free;
	}

	goto out_close;

out_free:
	tile_free(t);
	t = NULL;
out_close:
	fclose(fin);
	return t;
}

int main(int argc, char **argv)
{
	struct tile *t;

	if ( argc )
		cmd = argv[1];

	if ( argc != 3 ) {
		fprintf(stderr, "Usage:\n\t%s <out> <in>\n", cmd);
		return EXIT_FAILURE;
	}

	t = rip_file(argv[2]);
	if ( NULL == t )
		return EXIT_FAILURE;

	if ( !tile_dump(t, argv[1]) )
		return EXIT_FAILURE;

	tile_free(t);
	return EXIT_SUCCESS;
}
