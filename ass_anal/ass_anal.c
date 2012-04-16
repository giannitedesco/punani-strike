/*
 * ass_anal.c
 *
 *  Created on: 15 Apr 2012
 *      Author: steven
 */

#include <stdio.h>
#include <stdlib.h>
#include "iolib.h"

#define ASSETFILE_MAGIC	0x55daba00
#define ASSET_NAMELEN 32

typedef struct {
	int num_assets;
	int num_verts;
	int magic;
} ass_hdr_t;

typedef struct {
	char name[ASSET_NAMELEN];
	int offset;
	int num_idx;
} asset_desc_t;

typedef struct {
	ass_hdr_t header;
	asset_desc_t *desc;
	float *verts;
	float *norms;
} asset_t;


int analyze_header(memstream_t *in, asset_t *asset) {
	asset->header.num_assets = read_int(in);
	asset->header.num_verts = read_int(in);
	asset->header.magic = read_int(in);

	if (asset->header.magic != ASSETFILE_MAGIC) {
		msg(LERROR, "header failure: magic is %x expected %x", asset->header.magic, ASSETFILE_MAGIC);
		return 0;
	}

	msg(LINFO, "header: expecting %d assets, %d verts\n", asset->header.num_assets, asset->header.num_verts);

	return 1;
}

int analyze_descs(memstream_t *in, asset_t *asset) {
	asset->desc = (asset_desc_t *) malloc(sizeof(asset_desc_t) * asset->header.num_assets);
	for(int i = 0; i < asset->header.num_assets; i++) {
		read_arraystring(in, 32, asset->desc[i].name);
		asset->desc[i].offset = read_int(in);
		asset->desc[i].num_idx = read_int(in);
		msg(LDEBUG, "  asset %32s has %d indexes, starting at %d\n", asset->desc[i].name, asset->desc[i].num_idx, asset->desc[i].offset);
	}

	msg(LDEBUG, "read %d assets\n", asset->header.num_assets);

	return 1;
}

int analyze_vertarray(memstream_t *in, asset_t *asset) {
	asset->verts = (float *) malloc(sizeof(*asset->verts) * 3 * asset->header.num_verts);
	for(int i = 0; i < 3 * asset->header.num_verts; i++) {
		asset->verts[i] = read_float(in);
	}
}

int analyze_indexes(memstream_t *in, asset_t *asset, int normals) {
	int total_indexes = 0;
	for(int i = 0; i < asset->header.num_assets; i++) {
		total_indexes += asset->desc[i].num_idx;
	}

	if (total_indexes != asset->header.num_verts) {
		msg(LERROR, "total indexes loaded in all assets != header.num_verts: %d != %d\n", total_indexes, asset->header.num_verts);
		return 0;
	}
}



void analyze(const char *filename) {
	memstream_t *contents = memstream_from_file(filename);

	asset_t asset;

	if (!analyze_header(contents, &asset)) {
		msg(LERROR, "header failure\n");
		return;
	}
	if (!analyze_descs(contents, &asset)) {
		msg(LERROR, "asset descs failure\n");
		return;
	}
	if (!analyze_vertarray(contents, &asset)) {
		msg(LERROR, "asset verts failure\n");
		return;
	}
}

int main(int argc, char *argv[]) {

	printf("ass_anal, asset analyzer, %d \n", argc);

	if (argc == 1) {
		printf("usage:\n    ass_anal (assetfile) ...\n");
		return 0;
	}

	for(int i = 1; i < argc; i++)  {
		analyze(argv[i]);
	}

	return 0;
}






