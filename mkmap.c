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

#include "mapfile.h"

static const char *cmd = "mkmap";

struct tile {
	struct list_head t_list;
	char *t_name;
	unsigned int t_id;
	unsigned int t_index;
};

struct map {
	struct list_head m_tiles;
	midx_t *m_indices;
	struct tile **m_uniq;
	unsigned int m_num_uniq;
	unsigned int m_num_tiles;
	unsigned int m_x, m_y;
	unsigned int m_cur;
};

static struct map *map_new(void)
{
	struct map *m;

	m = calloc(1, sizeof(*m));
	if ( NULL == m)
		goto out;

	INIT_LIST_HEAD(&m->m_tiles);
	/* success */

out:
	return m;
}

static void map_free(struct map *m)
{
	if ( m ) {
		struct tile *tile, *tmp;
		list_for_each_entry_safe(tile, tmp, &m->m_tiles, t_list) {
			list_del(&tile->t_list);
			free(tile->t_name);
			free(tile);
		}
		free(m->m_indices);
		free(m->m_uniq);
		free(m);
	}
}

static int idcmp(const void *A, const void *B)
{
	const struct tile * const *a = A;
	const struct tile * const *b = B;
	if ( (*a)->t_id < (*b)->t_id )
		return -1;
	if ( (*a)->t_id > (*b)->t_id )
		return 1;
	return 0;
}

static int tile_lookup(struct map *m, struct tile **t, midx_t *idx)
{
	struct tile k, *key, **ret;
	k.t_id = *idx;
	key = &k;

	ret = bsearch(&key, t, m->m_num_tiles, sizeof(*t), idcmp);
	if ( ret ) {
		*idx = (*ret)->t_index;
		return 1;
	}

	fprintf(stderr, "%s: tile ID %u not found\n", cmd, *idx);
	return 0;
}

static int ncmp(const void *A, const void *B)
{
	const struct tile * const *a = A;
	const struct tile * const *b = B;
	return strcmp((*a)->t_name, (*b)->t_name);
}

static int indexify(struct map *m)
{
	struct tile *tile;
	unsigned int i, j;
	struct tile **arr, **out;
	int ret = 0;

	arr = malloc(m->m_num_tiles * sizeof(*arr));
	if ( arr == NULL )
		goto out;

	out = malloc(m->m_num_tiles * sizeof(*arr));
	if ( out == NULL )
		goto out_free;

	i = 0;
	list_for_each_entry(tile, &m->m_tiles, t_list) {
		arr[i] = tile;
		i++;
	}

	qsort(arr, m->m_num_tiles, sizeof(*arr), ncmp);
	out[0] = arr[0];

	for(j = 1, i = 1; i < m->m_num_tiles; i++) {
		if ( strcmp(out[j - 1]->t_name, arr[i]->t_name) ) {
			arr[i]->t_index = j;
			out[j] = arr[i];
			j++;
		}else{
			arr[i]->t_index = j - 1;
		}
	}

	if ( j > MAP_IDX_MAX ) {
		fprintf(stderr, "%s: too many tiles\n", cmd);
		goto out_free_out;
	}

	m->m_num_uniq = j;

	qsort(arr, m->m_num_tiles, sizeof(*arr), idcmp);
	printf("Reverse map %u ID's to %u unique tiles:\n",
		m->m_num_tiles, m->m_num_uniq);
	for(i = 0; i < m->m_num_tiles; i++) {
		printf(" %u: %s -> %u\n",
			arr[i]->t_id, arr[i]->t_name, arr[i]->t_index);
	}
	for(i = 0; i < m->m_x * m->m_y; i++) {
		if ( !tile_lookup(m, arr, &m->m_indices[i]) )
			goto out_free;
	}

	m->m_uniq = out;
	ret = 1;
	goto out_free;

out_free_out:
	free(out);
out_free:
	free(arr);
out:
	return ret;
}

static int map_dump(struct map *m, const char *fn)
{
	unsigned int i;
	struct map_hdr hdr;
	FILE *fout;

	if ( !indexify(m) )
		goto err;

	hdr.h_num_tiles = m->m_num_uniq;
	hdr.h_magic = MAPFILE_MAGIC;
	hdr.h_x = m->m_x;
	hdr.h_y = m->m_y;

	fout = fopen(fn, "wb");
	if ( NULL == fout ) {
		fprintf(stderr, "%s: %s: %s\n",
			cmd, fn, strerror(errno));
		return 0;
	}

	printf("Writing %ld byte header\n", sizeof(hdr));
	if ( fwrite(&hdr, sizeof(hdr), 1, fout) != 1 )
		goto err_close;

	printf("Writing %u bytes of tile names\n",
		m->m_num_uniq * MAPFILE_NAMELEN);
	for(i = 0; i < m->m_num_uniq; i++) {
		char name[MAPFILE_NAMELEN];
		memset(name, 0, sizeof(name));
		snprintf(name, sizeof(name), "%s", m->m_uniq[i]->t_name);
		if ( fwrite(name, sizeof(name), 1, fout) != 1 )
			goto err_close;
	}

	printf("Writing %ld byte matrix (%u x %u)\n",
		m->m_x * m->m_y * sizeof(midx_t), m->m_x, m->m_y);
	if ( fwrite(m->m_indices, sizeof(*m->m_indices),
			m->m_x * m->m_y, fout) != m->m_x * m->m_y )
		goto err_close;
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

static int parse_int(const char *str, unsigned int *val)
{
	char *end;

	*val = strtoul(str, &end, 0);
	if ( end == str || (*end != '\0' && *end != 'f') )
		return 0;

	return 1;
}

static struct tile *rip_tile(struct map *m, char *str)
{
	struct tile *tile = NULL;
	char *tok[2];
	int ntok;

	ntok = easy_explode(str, 0, tok, 2);
	if ( ntok != 2 )
		goto out;

	tile = calloc(1, sizeof(*tile));
	if ( NULL == tile )
		goto out;

	if ( !parse_int(tok[0], &tile->t_id) )
		goto out;

	tile->t_name = strdup(tok[1]);
	if ( NULL == tile->t_name )
		goto out_free;

	m->m_num_tiles++;
	list_add_tail(&tile->t_list, &m->m_tiles);
	goto out;
out_free:
	free(tile);
	tile = NULL;
out:
	return tile;
}

static int rip_map_line(struct map *m, char *str)
{
	char *tok[m->m_x];
	unsigned int i;
	int ntok;
	midx_t *ptr;

	if ( m->m_cur >= m->m_y )
		return 0;

	ntok = easy_explode(str, 0, tok, m->m_x);
	if ( ntok < 0 || (unsigned)ntok != m->m_x ) {
		fprintf(stderr, "%s: %d != %d\n", cmd, ntok, m->m_x);
		return 0;
	}

	ptr = m->m_indices + m->m_cur * m->m_x;
	for(i = 0; i < m->m_x; i++) {
		unsigned int num;
		if ( !parse_int(tok[i], &num) ) {
			fprintf(stderr, "%s: '%s' ??\n", cmd, tok[i]);
			return 0;
		}
		ptr[i] = num;
	}

	m->m_cur++;
	return 1;
}

static struct map *rip_file(const char *fn)
{
	struct map *m = NULL;
	FILE *fin;
	char buf[1024];
	char *ptr, *end;
	unsigned int line, begin = 0;

	fin = fopen(fn, "rb");
	if ( NULL == fin ) {
		fprintf(stderr, "%s: %s: %s\n", cmd, fn, strerror(errno));
		goto out_close;
	}

	m = map_new();
	if ( NULL == m ) {
		fprintf(stderr, "%s: %s: map_new: %s\n",
			cmd, fn, strerror(errno));
		goto out_free;
	}

	for(line = 1; fgets(buf, sizeof(buf), fin); line++ ) {
		struct tile *tile;
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

		if ( begin ) {
			if ( rip_map_line(m, ptr) )
				continue;
			goto parse_err;
		}

		ntok = easy_explode(ptr, 0, tok, 2);
		if ( ntok < 1 ) {
			fprintf(stderr, "%s: %s:%u: Parse error\n",
				cmd, fn, line);
			goto out_free;
		}

		if ( !strcmp(tok[0], "t") ) {
			tile = rip_tile(m, tok[1]);
			if ( NULL == tile )
				goto parse_err;
		}else if ( !strcmp(tok[0], "x") ) {
			if ( !parse_int(tok[1], &m->m_x) )
				goto parse_err;
		}else if ( !strcmp(tok[0], "y") ) {
			if ( !parse_int(tok[1], &m->m_y) )
				goto parse_err;
		}else if ( !strcmp(tok[0], "begin") ) {
			if ( !m->m_x || !m->m_y || !m->m_num_tiles ) {
				fprintf(stderr, "%s: %s:%u: unexpected '%s'\n",
					cmd, fn, line, tok[0]);
				goto out_free;
			}
			m->m_indices = calloc(m->m_x * m->m_y,
						sizeof(*m->m_indices));
			if ( NULL == m->m_indices )
				goto out_free;
			begin = 1;
		}else{
			fprintf(stderr, "%s: %s:%u: unknown '%s'\n",
				cmd, fn, line, tok[0]);
			goto out_free;
		}
	}

	goto out_close;

parse_err:
	fprintf(stderr, "%s: %s:%u: error\n",
		cmd, fn, line);
out_free:
	map_free(m);
	m = NULL;
out_close:
	fclose(fin);
	return m;
}

int main(int argc, char **argv)
{
	struct map *m;

	if ( argc )
		cmd = argv[0];

	if ( argc != 3 ) {
		fprintf(stderr, "Usage:\n\n%s <out> <in>\n", cmd);
		return EXIT_FAILURE;
	}

	m = rip_file(argv[2]);
	if ( NULL == m )
		return EXIT_FAILURE;

	if ( !map_dump(m, argv[1]) )
		return EXIT_FAILURE;

	map_free(m);
	printf("OK\n");
	return EXIT_SUCCESS;
}
