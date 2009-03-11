#include "e_mod_system.h"
#include <stdlib.h>
#include <string.h>

static const char *_name = NULL;

static void
_e_mixer_dummy_set(void)
{
   if (!_name) _name = eina_stringshare_add("dummy");
}

E_Mixer_System *
e_mixer_system_new(const char *name)
{
   _e_mixer_dummy_set();

   if (name == _name || strcmp(name, _name) == 0)
     return (E_Mixer_System *)-1;
   else
     return NULL;
}

void
e_mixer_system_del(E_Mixer_System *self)
{
}

int
e_mixer_system_callback_set(E_Mixer_System *self, int (*func)(void *data, E_Mixer_System *self), void *data)
{
   return 0;
}

Eina_List *
e_mixer_system_get_cards(void)
{
   _e_mixer_dummy_set();

   return eina_list_append(NULL, _name);
}

void
e_mixer_system_free_cards(Eina_List *cards)
{
   eina_list_free(cards);
}

const char *
e_mixer_system_get_default_card(void)
{
   _e_mixer_dummy_set();

   return eina_stringshare_ref(_name);
}

const char *
e_mixer_system_get_card_name(const char *card)
{
   _e_mixer_dummy_set();

   if (card == _name || strcmp(card, _name) == 0)
     return eina_stringshare_ref(_name);
   else
     return NULL;
}

Eina_List *
e_mixer_system_get_channels(E_Mixer_System *self)
{
   return eina_list_append(NULL, (void *)-2);
}

void
e_mixer_system_free_channels(Eina_List *channels)
{
   eina_list_free(channels);
}

Eina_List *
e_mixer_system_get_channels_names(E_Mixer_System *self)
{
   _e_mixer_dummy_set();

   return eina_list_append(NULL, _name);
}

void
e_mixer_system_free_channels_names(Eina_List *channels_names)
{
   eina_list_free(channels_names);
}

const char *
e_mixer_system_get_default_channel_name(E_Mixer_System *self)
{
   _e_mixer_dummy_set();

   return eina_stringshare_ref(_name);
}

E_Mixer_Channel *
e_mixer_system_get_channel_by_name(E_Mixer_System *self, const char *name)
{
   _e_mixer_dummy_set();

   if (name == _name || strcmp(name, _name) == 0)
     return (E_Mixer_Channel *)-2;
   else
     return NULL;
}

void
e_mixer_system_channel_del(E_Mixer_Channel *channel)
{
}

const char *
e_mixer_system_get_channel_name(E_Mixer_System *self, E_Mixer_Channel *channel)
{
   if (channel == (E_Mixer_Channel *)-2)
     return eina_stringshare_ref(_name);
   else
     return NULL;
}

int
e_mixer_system_get_volume(E_Mixer_System *self, E_Mixer_Channel *channel, int *left, int *right)
{
   if (left)
     *left = 0;
   if (right)
     *right = 0;

   return 1;
}

int
e_mixer_system_set_volume(E_Mixer_System *self, E_Mixer_Channel *channel, int left, int right)
{
   return 0;
}

int
e_mixer_system_can_mute(E_Mixer_System *self, E_Mixer_Channel *channel)
{
   return 1;
}

int
e_mixer_system_get_mute(E_Mixer_System *self, E_Mixer_Channel *channel, int *mute)
{
   if (mute)
     *mute = 1;

   return 1;
}

int
e_mixer_system_set_mute(E_Mixer_System *self, E_Mixer_Channel *channel, int mute)
{
   return 0;
}

int
e_mixer_system_get_state(E_Mixer_System *self, E_Mixer_Channel *channel, E_Mixer_Channel_State *state)
{
   const E_Mixer_Channel_State def = {1, 0, 0};

   if (state)
     *state = def;

   return 1;
}

int
e_mixer_system_set_state(E_Mixer_System *self, E_Mixer_Channel *channel, const E_Mixer_Channel_State *state)
{
   return 0;
}

int
e_mixer_system_has_capture(E_Mixer_System *self, E_Mixer_Channel *channel)
{
   return 0;
}
