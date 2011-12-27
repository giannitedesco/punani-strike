#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "mapfile.h"

#define HDR_NAME	"# name "
#define HDR_DIM		"# dimensions = "
#define HDR_TILESZ	"# tilesize = "
#define HDR_NUMTILES	"# num_tiles = "
#define HDR_TILEMAP	"# tilemap = "

#define STATE_HDR	0
#define STATE_DATA	1
static int hdr_line(struct mapfile_hdr *hdr, const char *ptr, unsigned line)
{
	const char *buf = ptr;

	if ( !strncmp(ptr, HDR_NAME, strlen(HDR_NAME)) ) {
		/* informational only */
		return 1;
	}else if ( !strncmp(ptr, HDR_DIM, strlen(HDR_DIM)) ) {
		unsigned int x, y;
		ptr += strlen(HDR_DIM);
		if ( !sscanf(ptr, "%u %u", &x, &y) )
			goto err;
		if ( x > 0xffff || y > 0xffff )
			goto err;
		hdr->xtiles = x;
		hdr->ytiles = y;
	}else if ( !strncmp(ptr, HDR_TILESZ, strlen(HDR_TILESZ)) ) {
		unsigned int x, y;
		ptr += strlen(HDR_TILESZ);
		if ( !sscanf(ptr, "%u %u", &x, &y) )
			goto err;
		if ( x > 0xff || y > 0xff )
			goto err;
		hdr->tile_width = x;
		hdr->tile_height = y;
	}else if ( !strncmp(ptr, HDR_NUMTILES, strlen(HDR_NUMTILES)) ) {
#if 0
		unsigned int num;
		ptr += strlen(HDR_NUMTILES);
		if ( !sscanf(ptr, "%u", &num) )
			goto err;
#endif
		/* inferred from tilemap PNG */
	}else if ( !strncmp(ptr, HDR_TILEMAP, strlen(HDR_TILEMAP)) ) {
		ptr += strlen(HDR_TILEMAP);
		if ( strlen(ptr) >= sizeof(hdr->tilemap) )
			goto err;
		strcpy((char *)hdr->tilemap, ptr);
	}else{
		printf("warn: unknown header on line %d: %s\n", line, ptr);
	}
	return 1;
err:
	printf("Bad header on line %u: %s\n", line, buf);
	return 0;
}

static int dump_hdr(struct mapfile_hdr *hdr, FILE *f)
{
	hdr->magic = MAPFILE_MAGIC;

	if ( !hdr->xtiles || !hdr->ytiles ) {
		fprintf(stderr, "bad dimensions\n");
		return 0;
	}
	if ( !hdr->tile_width || !hdr->tile_height ) {
		fprintf(stderr, "bad tile size\n");
		return 0;
	}

	if ( fwrite(hdr, sizeof(*hdr), 1, f) != 1 ) {
		fprintf(stderr, "OUTPUT: %s\n", strerror(errno));
		return 0;
	}
	return 1;
}

static int rip_file(FILE *fin, FILE *fout)
{
	struct mapfile_hdr hdr;
	unsigned int state = STATE_HDR;
	char buf[1024];
	char *ptr, *end;
	unsigned int line;

	memset(&hdr, 0, sizeof(hdr));

	for(line = 1; fgets(buf, sizeof(buf), fin); line++ ) {
		end = strchr(buf, '\r');
		if ( NULL == end )
			end = strchr(buf, '\n');

		if ( NULL == end ) {
			fprintf(stderr, "INPUT:%u: Line too long\n", line);
			return 0;
		}

		*end = '\0';

		/* strip trailing whitespace */
		for(end = end - 1; isspace(*end); end--)
			*end= '\0';
		/* strip leading whitespace */
		for(ptr = buf; isspace(*ptr); ptr++)
			/* nothing */;

		if ( ptr == '\0' )
			continue;

again:
		if ( state == STATE_HDR ) {
			if ( ptr[0] == '#' ) {
				if ( !hdr_line(&hdr, ptr, line) )
					return 0;
			}else{
				if ( !dump_hdr(&hdr, fout) )
					return 0;
				state = STATE_DATA;
				goto again;
			}
		}else{
			unsigned int tile_id;
			uint16_t tid;
			sscanf(ptr, "%u", &tile_id);
			if ( tile_id > 0xffff ) {
				fprintf(stderr, "Bad tile ID on line %u: %u\n",
					line, tile_id);
				return 0;
			}
			tid = tile_id;
			if ( fwrite(&tid, sizeof(tid), 1, fout) != 1 ) {
				fprintf(stderr, "OUTPUT: %s\n",
					strerror(errno));
			}
		}
	}

	return 1;
}

int main(int argc, char **argv)
{
	FILE *fin, *fout;

	if ( argc < 3 ) {
		fprintf(stderr, "Usage:\n\t%s <infile> <outfile>\n", argv[0]);
		return EXIT_FAILURE;
	}

	fin = fopen(argv[1], "r");
	if ( NULL == fin ) {
		fprintf(stderr, "%s: %s: %s\n", argv[0], argv[1],
			strerror(errno));
		return EXIT_FAILURE;
	}

	fout = fopen(argv[2], "w");
	if ( NULL == fout ) {
		fprintf(stderr, "%s: %s: %s\n", argv[0], argv[2],
			strerror(errno));
		return EXIT_FAILURE;
	}

	if ( !rip_file(fin, fout) )
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
