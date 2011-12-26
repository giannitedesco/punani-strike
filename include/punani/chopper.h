#ifndef _PUNANI_CHOPPER_H
#define _PUNANI_CHOPPER_H

typedef struct _chopper *chopper_t;

chopper_t chopper_apache(void);
chopper_t chopper_comanche(void);
void chopper_render(chopper_t chopper, game_t g);
void chopper_free(chopper_t chopper);

#endif /* _PUNANI_CHOPPER_H */
