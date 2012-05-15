/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#ifndef _SND_H
#define _SND_H

typedef struct _snd *snd_t;
typedef struct _sndchan *sndchan_t;

snd_t snd_load(const char *fn);
sndchan_t snd_play(snd_t snd, int loops);
void snd_put(snd_t s);
int snd_init(void);
void snd_fini(void);

void snd_stop(sndchan_t chan);

#endif /* _SND_H */
