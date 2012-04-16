#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define max(x,y) (x>y?x:y)
#define min(x,y) (x<y?x:y)

#include "iolib.h"

/*******************
 * Memstream, input
 *******************/
memstream_t *memstream_from_file(const char *filename)
{
	memstream_t *res = (memstream_t *) malloc(sizeof(memstream_t));
	res->offs = 0;
	
	FILE *fd;
	fd = fopen(filename, "r");
	if (!fd) 
	{
		perror("failed to open file for memstream\n");
		exit(1);
	}
	
	if (fseek(fd, 0, SEEK_END))
	{
		perror("failed to seek to end of \n");
		exit(1);
	}
	
	res->size = ftell(fd);
	
	fseek(fd, 0, SEEK_SET);
	
	fprintf(stderr, "read %d bytes into memstream\n", res->size);
	
	res->data = (char*) malloc(res->size);
	fread(res->data, res->size, 1, fd);
	res->allocated = res->size;
	
	fclose(fd);
	
	return res;
}

unsigned char read_uchar(memstream_t *dat)
{
	unsigned char res;
	memcpy(&res, &dat->data[dat->offs], sizeof(res));
	dat->offs += sizeof(res);
	return res;
}

int read_int(memstream_t *dat)
{
	int res;
	memcpy(&res, &dat->data[dat->offs], sizeof(res));
	dat->offs += sizeof(res);
	return res;
}

unsigned int read_uint(memstream_t *dat)
{
	unsigned int res;
	memcpy(&res, &dat->data[dat->offs], sizeof(res));
	dat->offs += sizeof(res);
	return res;
}

float read_float(memstream_t *dat)
{
	float res;
	memcpy(&res, &dat->data[dat->offs], sizeof(res));
	dat->offs += sizeof(res);
	return res;
}

short read_short(memstream_t *dat)
{
	short res;
	memcpy(&res, &dat->data[dat->offs], sizeof(res));
	dat->offs += sizeof(res);
	return res;
}

char *read_string(memstream_t *dat)
{
	char *res;
	int size = read_int(dat);
	if (size > 99999)
	{
		fprintf(stderr, "probably not a string: %d size?\n", size);
		exit(1);
	}
	res = (char*)malloc(size + 1);
	memcpy(res, &dat->data[dat->offs], size);
	dat->offs += size;
	res[size] = 0;
	return res;
}

char *read_shortstring(memstream_t *dat)
{
	char *res;
	unsigned char size = read_uchar(dat);
	res = (char*)malloc(size + 1);
	memcpy(res, &dat->data[dat->offs], size);
	dat->offs += size;
	res[size] = 0;
	return res;
}

void read_arraystring(memstream_t *dat, int len, char *output)
{
	memcpy(output, &dat->data[dat->offs], len);
	dat->offs += len;
}

/*******************
 * Memstream, output
 *******************/

void memstream_extend(memstream_t *mem, int by)
{
	mem->allocated += by;
	mem->data = (char*) realloc(mem->data, mem->allocated);
}

void memstream_write(memstream_t *mem, const void *dat, int size)
{
	if (mem->size + size >= mem->allocated)
	{
		memstream_extend(mem, max(size, 4096));
	}
	memcpy(&mem->data[mem->offs], (const char*)dat, size);
	mem->size += size;
	mem->offs += size;
}
 
void memstream_prepare_for_output(memstream_t *mem)
{
	memset(mem, 0, sizeof(memstream_t));
	mem->allocated = 4096;
	mem->data = (char*) malloc(4096);
	
	memstream_extend(mem, 4096);
}

void write_uchar(memstream_t *dat, const unsigned char val)
{
	memstream_write(dat, &val, sizeof(val));	
}

void write_int(memstream_t *dat, const int val)
{
	memstream_write(dat, &val, sizeof(val));
}
void write_uint(memstream_t *dat, const unsigned int val)
{
	memstream_write(dat, &val, sizeof(val));
}

void write_float(memstream_t *dat, const float val)
{
	memstream_write(dat, &val, sizeof(val));
}

void write_short(memstream_t *dat, const short val)
{
	memstream_write(dat, &val, sizeof(val));
}


void write_string(memstream_t *dat, const char * val)
{
	if (!val)
	{
		write_int(dat, 0);
		return;
	}
	int len = strlen(val);
	write_int(dat, len);
	memstream_write(dat, val, len);
}

void write_shortstring(memstream_t *dat, const char * val)
{
	if (!val)
	{
		write_uchar(dat, 0);
		return;
	}
	int len = strlen(val);
	if (len > 255)
	{
		msg(LINFO, "attempted to write over-size short string '%s'. truncated\n", val);
		len = 255;
	}
	write_uchar(dat, len);
	memstream_write(dat, val, min(255, len));
}

int loglevel;

void msg(int level, char *msg, ...)
{
	va_list args;
	
	if (level < loglevel) return;

	va_start(args, msg);
	vfprintf(stdout, msg, args);
	va_end(args);
}
