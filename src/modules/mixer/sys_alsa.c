#include <alsa/asoundlib.h>
#include <stdlib.h>
#include <math.h>
#include <poll.h>
#include <alloca.h>
#include <Ecore.h>
#include "e_mod_system.h"

struct e_mixer_callback_desc
{
   int (*func)(void *data, E_Mixer_System *self);
   void *data;
   E_Mixer_System *self;
   Ecore_Idler *idler;
   Evas_List *handlers;
};


static int _mixer_callback_add(E_Mixer_System *self, int (*func)(void *data, E_Mixer_System *self), void *data);
static int _mixer_callback_del(E_Mixer_System *self, struct e_mixer_callback_desc *desc);


static int
_cb_dispatch(void *data)
{
   struct e_mixer_callback_desc *desc;
   int r;

   desc = data;
   snd_mixer_handle_events(desc->self);
   r = desc->func(desc->data, desc->self);
   desc->idler = NULL;

   if (!r)
     _mixer_callback_del(desc->self, desc); /* desc is invalid then. */

   return 0;
}

static int
_cb_fd_handler(void *data, Ecore_Fd_Handler *fd_handler)
{
   struct e_mixer_callback_desc *desc;

   desc = data;

   if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_ERROR))
     {
	desc->handlers = evas_list_remove(desc->handlers, fd_handler);
	if (!desc->handlers)
	  {
	     E_Mixer_System *s;
	     int (*f)(void *, E_Mixer_System *);
	     void *d;

	     s = desc->self;
	     f = desc->func;
	     d = desc->data;
	     _mixer_callback_del(s, desc);
	     _mixer_callback_add(s, f, d);
	  }
	return 0;
     }

   if (!desc->idler)
     desc->idler = ecore_idler_add(_cb_dispatch, desc);
   return 1;
}

static int
_mixer_callback_add(E_Mixer_System *self, int (*func)(void *data, E_Mixer_System *self), void *data)
{
   struct e_mixer_callback_desc *desc;
   struct pollfd *pfds;
   int len;

   len = snd_mixer_poll_descriptors_count(self);
   if (len <= 0)
     return 0;

   desc = malloc(sizeof(struct e_mixer_callback_desc));
   if (!desc)
     return 0;

   desc->func = func;
   desc->data = data;
   desc->self = self;
   desc->idler = NULL;
   desc->handlers = NULL;

   pfds = alloca(len * sizeof(struct pollfd));
   len = snd_mixer_poll_descriptors(self, pfds, len);
   if (len <= 0)
     {
	free(desc);
	return 0;
     }

   while (len > 0)
     {
	Ecore_Fd_Handler *fd_handler;

	len--;
	fd_handler = ecore_main_fd_handler_add(
           pfds[len].fd, ECORE_FD_READ, _cb_fd_handler, desc, NULL, NULL);
	desc->handlers = evas_list_prepend(desc->handlers, fd_handler);
     }

   snd_mixer_set_callback_private(self, desc);

   return 1;
}

static int
_mixer_callback_del(E_Mixer_System *self, struct e_mixer_callback_desc *desc)
{
   Evas_List *l;

   snd_mixer_set_callback_private(self, NULL);

   for (l = desc->handlers; l != NULL; l = l->next)
     ecore_main_fd_handler_del(l->data);

   evas_list_free(desc->handlers);
   free(desc);

   return 1;
}

static int
_mixer_callback_replace(E_Mixer_System *self, struct e_mixer_callback_desc *desc, int (*func)(void *data, E_Mixer_System *self), void *data)
{
   desc->func = func;
   desc->data = data;

   return 1;
}

E_Mixer_System *
e_mixer_system_new(const char *name)
{
   snd_mixer_t *handle;
   int err;

   if (!name)
     return NULL;

   err = snd_mixer_open(&handle, 0);
   if (err < 0)
     goto error_open;

   err = snd_mixer_attach(handle, name);
   if (err < 0)
     goto error_load;

   err = snd_mixer_selem_register(handle, NULL, NULL);
   if (err < 0)
     goto error_load;

   err = snd_mixer_load(handle);
   if (err < 0)
     goto error_load;

   return handle;

 error_load:
   snd_mixer_close(handle);
 error_open:
   fprintf(stderr, "MIXER: Cannot get hardware info: %s\n", snd_strerror(err));
   return NULL;
}

void
e_mixer_system_del(E_Mixer_System *self)
{
   struct e_mixer_callback_desc *desc;

   if (self <= 0)
     return;

   desc = snd_mixer_get_callback_private(self);
   if (desc)
     _mixer_callback_del(self, desc);

   snd_mixer_close(self);
}

int
e_mixer_system_callback_set(E_Mixer_System *self, int (*func)(void *data, E_Mixer_System *self), void *data)
{
   struct e_mixer_callback_desc *desc;

   if (!self)
     return 0;

   desc = snd_mixer_get_callback_private(self);
   if (!desc)
     {
	if (func)
	  return _mixer_callback_add(self, func, data);
	return 1;
     }
   else
     {
	if (func)
	  return _mixer_callback_replace(self, desc, func, data);
	else
	  return _mixer_callback_del(self, desc);
     }
}

Evas_List *
e_mixer_system_get_cards(void)
{
   int err, card_num;
   Evas_List *cards;

   cards = NULL;
   card_num = -1;
   while (((err = snd_card_next(&card_num)) == 0) && (card_num >= 0))
     {
        snd_ctl_t *control;
        char buf[256];

	snprintf(buf, sizeof(buf), "hw:%d", card_num);

	if (snd_ctl_open(&control, buf, 0) < 0)
	  break;
	snd_ctl_close(control);
        cards = evas_list_append(cards, strdup(buf));
     }

   if (err < 0)
     fprintf(stderr, "MIXER: Cannot get available card number: %s\n",
	     snd_strerror(err));

   return cards;
}

void
e_mixer_system_free_cards(Evas_List *cards)
{
   Evas_List *e;

   for (e = cards; e != NULL; e = e->next)
     free(e->data);

   evas_list_free(cards);
}

char *
e_mixer_system_get_default_card(void)
{
   static const char buf[] = "hw:0";
   snd_ctl_t *control;

   if (snd_ctl_open(&control, buf, 0) < 0)
     return NULL;
   snd_ctl_close(control);
   return strdup(buf);
}

char *
e_mixer_system_get_card_name(const char *card)
{
   snd_ctl_card_info_t *hw_info;
   const char *name;
   snd_ctl_t *control;
   int err;

   if (!card)
     return NULL;

   snd_ctl_card_info_alloca(&hw_info);

   err = snd_ctl_open(&control, card, 0);
   if (err < 0)
     return NULL;

   err = snd_ctl_card_info(control, hw_info);
   if (err < 0)
     {
	fprintf(stderr, "MIXER: Cannot get hardware info: %s: %s\n", card,
		snd_strerror(err));
	snd_ctl_close(control);
	return NULL;
     }

   snd_ctl_close(control);
   name = snd_ctl_card_info_get_name(hw_info);
   if (!name)
     {
	fprintf(stderr, "MIXER: Cannot get hardware name: %s\n", card);
	return NULL;
     }

   return strdup(name);
}

Evas_List *
e_mixer_system_get_channels(E_Mixer_System *self)
{
   Evas_List *channels;
   snd_mixer_elem_t *elem;

   if (!self)
     return NULL;

   channels = NULL;

   elem = snd_mixer_first_elem(self);
   for (; elem != NULL; elem = snd_mixer_elem_next(elem))
     {
        if ((!snd_mixer_selem_is_active(elem)) ||
	    (!snd_mixer_selem_has_playback_volume(elem)))
	  continue;

        channels = evas_list_append(channels, elem);
     }

   return channels;
}

void
e_mixer_system_free_channels(Evas_List *channels)
{
   evas_list_free(channels);
}

Evas_List *
e_mixer_system_get_channels_names(E_Mixer_System *self)
{
   Evas_List *channels;
   snd_mixer_elem_t *elem;
   snd_mixer_selem_id_t *sid;

   if (!self)
     return NULL;

   channels = NULL;
   snd_mixer_selem_id_alloca(&sid);

   elem = snd_mixer_first_elem(self);
   for (; elem != NULL; elem = snd_mixer_elem_next(elem))
     {
	const char *name;
        if ((!snd_mixer_selem_is_active(elem)) ||
	    (!snd_mixer_selem_has_playback_volume(elem)))
	  continue;

        snd_mixer_selem_get_id(elem, sid);
	name = snd_mixer_selem_id_get_name(sid);
	if (name)
	  channels = evas_list_append(channels, strdup(name));
     }

   return channels;
}

void
e_mixer_system_free_channels_names(Evas_List *channels_names)
{
   Evas_List *e;

   for (e = channels_names; e != NULL; e = e->next)
     free(e->data);

   evas_list_free(channels_names);
}

char *
e_mixer_system_get_default_channel_name(E_Mixer_System *self)
{
   snd_mixer_elem_t *elem;
   snd_mixer_selem_id_t *sid;

   if (!self)
     return NULL;

   snd_mixer_selem_id_alloca(&sid);

   elem = snd_mixer_first_elem(self);
   for (; elem != NULL; elem = snd_mixer_elem_next(elem))
     {
	const char *name;
        if ((!snd_mixer_selem_is_active(elem)) ||
	    (!snd_mixer_selem_has_playback_volume(elem)))
	  continue;

        snd_mixer_selem_get_id(elem, sid);
	name = snd_mixer_selem_id_get_name(sid);
	if (name)
	  return strdup(name);
     }

   return NULL;
}

E_Mixer_Channel *
e_mixer_system_get_channel_by_name(E_Mixer_System *self, const char *name)
{
   snd_mixer_elem_t *elem;
   snd_mixer_selem_id_t *sid;

   if ((!self) || (!name))
     return NULL;

   snd_mixer_selem_id_alloca(&sid);

   elem = snd_mixer_first_elem(self);
   for (; elem != NULL; elem = snd_mixer_elem_next(elem))
     {
	const char *n;
        if ((!snd_mixer_selem_is_active(elem)) ||
	    (!snd_mixer_selem_has_playback_volume(elem)))
	  continue;

        snd_mixer_selem_get_id(elem, sid);
	n = snd_mixer_selem_id_get_name(sid);
	if (n && (strcmp(n, name) == 0))
	  return elem;
     }

   return NULL;
}

void
e_mixer_system_channel_del(E_Mixer_Channel *channel)
{
}

char *
e_mixer_system_get_channel_name(E_Mixer_System *self, E_Mixer_Channel *channel)
{
   snd_mixer_selem_id_t *sid;
   const char *n;
   char *name;

   if ((!self) || (!channel))
     return NULL;

   snd_mixer_selem_id_alloca(&sid);
   snd_mixer_selem_get_id(channel, sid);
   n = snd_mixer_selem_id_get_name(sid);
   if (n)
     name = strdup(n);
   else
     name = NULL;

   return name;
}

int
e_mixer_system_get_volume(E_Mixer_System *self, E_Mixer_Channel *channel, int *left, int *right)
{
   long lvol, rvol, range, min, max;

   if ((!self) || (!channel) || (!left) || (!right))
     return 0;

   snd_mixer_handle_events(self);
   snd_mixer_selem_get_playback_volume_range(channel, &min, &max);
   range = max - min;
   if (range < 1)
     return 0;

   if (snd_mixer_selem_has_playback_channel(channel, 0))
     snd_mixer_selem_get_playback_volume(channel, 0, &lvol);
   else
     lvol = min;

   if (snd_mixer_selem_has_playback_channel(channel, 1))
     snd_mixer_selem_get_playback_volume(channel, 1, &rvol);
   else
     rvol = min;

   if (snd_mixer_selem_is_playback_mono(channel) ||
       snd_mixer_selem_has_playback_volume_joined(channel))
     rvol = lvol;

   *left = rint((double)(lvol - min) * 100 / (double)range);
   *right = rint((double)(rvol - min) * 100 / (double)range);

   return 1;
}

int
e_mixer_system_set_volume(E_Mixer_System *self, E_Mixer_Channel *channel, int left, int right)
{
   long range, min, max, div;
   int mode;

   if ((!self) || (!channel))
     return 0;

   snd_mixer_handle_events(self);
   snd_mixer_selem_get_playback_volume_range(channel, &min, &max);
   div = 100 + min;
   if (div == 0)
     {
	div = 1; /* no zero-division */
	min++;
     }

   range = max - min;
   if (range < 1)
	return 0;

   mode = 0;
   if (left >= 0)
     {
	left = (((range * left) + (range / 2)) / div) - min;
	mode |= 1;
     }

   if (right >= 0)
     {
	right = (((range * right) + (range / 2)) / div) - min;
	mode |= 2;
     }

   if (mode & 1)
     snd_mixer_selem_set_playback_volume(channel, 0, left);

   if ((!snd_mixer_selem_is_playback_mono(channel)) &&
       (!snd_mixer_selem_has_playback_volume_joined(channel)) &&
       (mode & 2))
     {
	if (snd_mixer_selem_has_playback_channel(channel, 1))
	  snd_mixer_selem_set_playback_volume(channel, 1, right);
     }

   return 1;
}

int
e_mixer_system_can_mute(E_Mixer_System *self, E_Mixer_Channel *channel)
{
   if ((!self) || (!channel))
     return 0;

   snd_mixer_handle_events(self);
   return (snd_mixer_selem_has_playback_switch(channel) ||
	   snd_mixer_selem_has_playback_switch_joined(channel));
}

int
e_mixer_system_get_mute(E_Mixer_System *self, E_Mixer_Channel *channel, int *mute)
{
   if ((!self) || (!channel) || (!mute))
     return 0;

   snd_mixer_handle_events(self);
   if (snd_mixer_selem_has_playback_switch(channel) ||
       snd_mixer_selem_has_playback_switch_joined(channel))
     {
	int m;

	/* XXX: not checking for return, always returns 0 even if it worked.
	 * alsamixer also don't check it. Bug?
	 */
	snd_mixer_selem_get_playback_switch(channel, 0, &m);
	*mute = !m;
     }
   else
     *mute = 0;

   return 1;
}

int
e_mixer_system_set_mute(E_Mixer_System *self, E_Mixer_Channel *channel, int mute)
{
   if ((!self) || (!channel))
     return 0;

   snd_mixer_handle_events(self);
   if (snd_mixer_selem_has_playback_switch(channel) ||
       snd_mixer_selem_has_playback_switch_joined(channel))
     return snd_mixer_selem_set_playback_switch_all(channel, !mute);
   else
     return 0;
}

int
e_mixer_system_get_state(E_Mixer_System *self, E_Mixer_Channel *channel, E_Mixer_Channel_State *state)
{
   int r;

   if (!state)
     return 0;

   r = e_mixer_system_get_mute(self, channel, &state->mute);
   r &= e_mixer_system_get_volume(self, channel, &state->left, &state->right);
   return r;
}

int
e_mixer_system_set_state(E_Mixer_System *self, E_Mixer_Channel *channel, const E_Mixer_Channel_State *state)
{
   int r;

   if (!state)
     return 0;

   r = e_mixer_system_set_mute(self, channel, state->mute);
   r &= e_mixer_system_set_volume(self, channel, state->left, state->right);
   return r;
}

int
e_mixer_system_has_capture(E_Mixer_System *self, E_Mixer_Channel *channel)
{
   if ((!self) || (!channel))
     return 0;

   return snd_mixer_selem_has_capture_switch(channel);
}
