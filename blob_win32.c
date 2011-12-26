/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/blob.h>
#include <windows.h>

uint8_t *blob_from_file(const char *fn, size_t *size)
{
	BY_HANDLE_FILE_INFORMATION info;
	HANDLE *f;
	DWORD done;
	uint8_t *b = NULL;
	uint64_t sz;

	f = CreateFile(fn, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
					 FILE_ATTRIBUTE_NORMAL, NULL);
	if ( f == INVALID_HANDLE_VALUE )
		goto err;

	if ( !GetFileInformationByHandle(f, &info) )
		goto err;

	if ( info.nFileSizeHigh )
		goto err;

	sz = info.nFileSizeLow;

	b = malloc(sz);
	if ( NULL == b )
		goto err;

	if ( !ReadFile(f, b, sz, &done, NULL) )
		goto err;
	if ( done != sz )
		goto err;

	*size = sz;
	CloseHandle(f);
	return b;
err:
	fprintf(stderr, "%s: %s\n", fn, strerror(errno));
	if ( f != INVALID_HANDLE_VALUE )
		CloseHandle(f);
	free(b);
	*size = 0;
	return NULL;
}

void blob_free(uint8_t *blob, size_t sz)
{
	free(blob);
}
