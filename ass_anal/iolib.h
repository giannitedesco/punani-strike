#ifndef IOLIB_H__
#define IOLIB_H__

typedef struct {
	char *data;
	int size;
	int offs;
	int allocated;
} memstream_t;

memstream_t *memstream_from_file(const char *filename);

unsigned char read_uchar(memstream_t *fd);
int read_int(memstream_t *fd);
unsigned int read_uint(memstream_t *fd);
float read_float(memstream_t *fd);
short read_short(memstream_t *fd);

char *read_string(memstream_t *fd);
char *read_shortstring(memstream_t *fd);
void read_arraystring(memstream_t *fd, int len, char *output);

void memstream_prepare_for_output(memstream_t *mem);
void write_uchar(memstream_t *fd, const unsigned char val);
void write_int(memstream_t *fd, const int val);
void write_uint(memstream_t *fd, const unsigned int val);
void write_float(memstream_t *fd, const float val);
void write_short(memstream_t *fd, const short val);

void write_string(memstream_t *fd, const char * val);
void write_shortstring(memstream_t *fd, const char * val);

extern int loglevel;

#define LEVERYTHING 0
#define LDEBUG 1
#define LRAW 2
#define LPARSED 3
#define LINFO 4
#define LWARN 5
#define LERROR 6

void msg(int level, char *msg, ...);


#endif

