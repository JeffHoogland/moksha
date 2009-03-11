#ifndef E_MOD_SYSTEM_H
#define E_MOD_SYSTEM_H

#include <Evas.h>

typedef void E_Mixer_System;
typedef void E_Mixer_Channel;

struct E_Mixer_Channel_State
{
    int mute;
    int left;
    int right;
};
typedef struct E_Mixer_Channel_State E_Mixer_Channel_State;

Eina_List *e_mixer_system_get_cards(void);
void e_mixer_system_free_cards(Eina_List *cards);
const char *e_mixer_system_get_default_card(void);
const char *e_mixer_system_get_card_name(const char *card);


E_Mixer_System *e_mixer_system_new(const char *card);
void e_mixer_system_del(E_Mixer_System *self);

int e_mixer_system_callback_set(E_Mixer_System *self, int (*func)(void *data, E_Mixer_System *self), void *data);

Eina_List *e_mixer_system_get_channels(E_Mixer_System *self);
void e_mixer_system_free_channels(Eina_List *channels);
Eina_List *e_mixer_system_get_channels_names(E_Mixer_System *self);
void e_mixer_system_free_channels_names(Eina_List *channels_names);
const char *e_mixer_system_get_default_channel_name(E_Mixer_System *self);
E_Mixer_Channel *e_mixer_system_get_channel_by_name(E_Mixer_System *self, const char *name);
const char *e_mixer_system_get_channel_name(E_Mixer_System *self, E_Mixer_Channel *channel);
void e_mixer_system_channel_del(E_Mixer_Channel *channel);


int e_mixer_system_get_state(E_Mixer_System *self, E_Mixer_Channel *channel, E_Mixer_Channel_State *state);
int e_mixer_system_set_state(E_Mixer_System *self, E_Mixer_Channel *channel, const E_Mixer_Channel_State *state);
int e_mixer_system_get_volume(E_Mixer_System *self, E_Mixer_Channel *channel, int *left, int *right);
int e_mixer_system_set_volume(E_Mixer_System *self, E_Mixer_Channel *channel, int left, int right);
int e_mixer_system_get_mute(E_Mixer_System *self, E_Mixer_Channel *channel, int *mute);
int e_mixer_system_set_mute(E_Mixer_System *self, E_Mixer_Channel *channel, int mute);
int e_mixer_system_can_mute(E_Mixer_System *self, E_Mixer_Channel *channel);
int e_mixer_system_has_capture(E_Mixer_System *self, E_Mixer_Channel *channel);


#endif /* E_MOD_SYSTEM_H */
