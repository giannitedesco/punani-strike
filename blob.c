/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/blob.h>

uint8_t *blob_from_file(const char *fn, size_t *size)
{
	FILE *f = NULL;
	uint8_t *b = NULL;
	int sz;

	f = fopen(fn, "rb");
	if ( NULL == f )
		goto err;

	fseek(f, 0, SEEK_END);
	sz = ftell(f);
	fseek(f, 0, SEEK_SET);

	if ( sz < 0 )
		goto err;

	b = malloc(sz);
	if ( NULL == b )
		goto err;

	if ( fread(b, sz, 1, f) <= 0 )
		goto err;

	*size = sz;
	fclose(f);
	return b;
err:
	fprintf(stderr, "%s: %s\n", fn, strerror(errno));
	if ( f )
		fclose(f);
	free(b);
	*size = 0;
	return NULL;
}

int blob_to_file(const uint8_t *b, size_t sz, const char *fn)
{
	FILE *f;
	int ret;

	f = fopen(fn, "wb");
	if ( NULL == f )
		return 0;
	ret = (fwrite(b, sz, 1, f) == 1);
	fclose(f);
	return ret;
}

void blob_free(uint8_t *blob, size_t sz)
{
	free(blob);
}
