/* This file is part of punani-strike
 * Copyright (c) 2011 Gianni Tedesco
 * Released under the terms of GPLv3
*/
#include <punani/punani.h>
#include <punani/snd.h>
#include <SDL_mixer.h>
#include "list.h"

struct _snd {
	struct list_head s_list;
	char *s_name;
	Mix_Chunk *s_chunk;
	unsigned int s_ref;
};

struct _sndchan {
	struct _snd *c_snd;
	unsigned int c_chan;
};

static LIST_HEAD(sounds);


static struct _snd *snd_get(struct _snd *s)
{
	s->s_ref++;
	return s;
}

snd_t snd_load(const char *fn)
{
	struct _snd *s;

	list_for_each_entry(s, &sounds, s_list) {
		if ( !strcmp(s->s_name, fn) )
			return snd_get(s);
	}

	s = calloc(1, sizeof(*s));
	if ( NULL == s )
		goto out;

	s->s_name = strdup(fn);
	if ( NULL == s->s_name )
		goto out_free;

	s->s_chunk = Mix_LoadWAV(fn);
	if ( NULL == s->s_chunk ) {
		con_printf("snd: load: %s: %s\n", s->s_name, Mix_GetError());
		goto out_free_name;
	}

	/* success */
	s->s_ref = 1;
	list_add_tail(&s->s_list, &sounds);
	goto out;

out_free_name:
	free(s->s_name);
out_free:
	free(s);
	s = NULL;
out:
	return s;
}

void snd_put(snd_t s)
{
	if ( s ) {
		s->s_ref--;
		if ( !s->s_ref ) {
			list_del(&s->s_list);
			free(s->s_name);
			Mix_FreeChunk(s->s_chunk);
			free(s);
		}
	}
}

static struct _sndchan *channel;
static unsigned int num_channels;

int snd_init(void)
{
	unsigned int i;
	int ret;

	ret = Mix_OpenAudio(MIX_DEFAULT_FREQUENCY,
				MIX_DEFAULT_FORMAT,
				MIX_DEFAULT_CHANNELS, 8192);
	if ( ret < 0 )
		goto err;

	ret = Mix_AllocateChannels(128);
	if ( ret <= 0 )
		goto err;

	num_channels = ret;
	channel = calloc(num_channels, sizeof(*channel));
	for(i = 0; i < num_channels; i++) {
		channel[i].c_chan = i;
	}

	con_printf("snd: %u chans allocated\n", num_channels);
	return 1;
err:
	con_printf("snd: init: %s\n", Mix_GetError());
	return 0;
}

void snd_fini(void)
{
	Mix_HaltChannel(-1);
	Mix_CloseAudio();
	free(channel);
}

sndchan_t snd_play(snd_t s, int loops)
{
	int c;

	c = Mix_PlayChannel(-1, s->s_chunk, -1);
	if ( c < 0 ) {
		con_printf("snd: play: %s: %s\n", s->s_name, Mix_GetError());
		return NULL;
	}

	channel[c].c_snd = snd_get(s);
	return &channel[c];
}

void snd_stop(sndchan_t chan)
{
	Mix_HaltChannel(chan->c_chan);
	snd_put(chan->c_snd);
}
