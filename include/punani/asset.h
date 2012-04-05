/* This file is part of punani-strike
 * Copyright (c) 2012 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _PUNANI_ASSET_H
#define _PUNANI_ASSET_H

typedef struct _asset_file *asset_file_t;
typedef struct _asset *asset_t;

asset_file_t asset_file_open(const char *fn);
asset_t asset_file_get(asset_file_t f, const char *name);
void asset_file_close(asset_file_t f);

void asset_render(asset_t a);
void asset_put(asset_t a);

#endif /* _PUNANI_ASSET_H */
