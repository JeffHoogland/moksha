#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "common.h"

#include <ctype.h>
#include <errno.h>

#include <pulse/pulseaudio.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

/* Ecore mainloop integration start */
struct pa_io_event
{
   pa_mainloop_api *mainloop;
   Ecore_Fd_Handler               *handler;

   void                           *userdata;

   pa_io_event_flags_t             flags;
   pa_io_event_cb_t                callback;
   pa_io_event_destroy_cb_t        destroy_callback;
};

static Ecore_Fd_Handler_Flags
map_flags_to_ecore(pa_io_event_flags_t flags)
{
   return (Ecore_Fd_Handler_Flags)((flags & PA_IO_EVENT_INPUT ? ECORE_FD_READ : 0)   |
                                   (flags & PA_IO_EVENT_OUTPUT ? ECORE_FD_WRITE : 0) |
                                   (flags & PA_IO_EVENT_ERROR ? ECORE_FD_ERROR : 0)  |
                                   (flags & PA_IO_EVENT_HANGUP ? ECORE_FD_READ : 0));
}

static Eina_Bool
_ecore_io_wrapper(void *data, Ecore_Fd_Handler *handler)
{
   char buf[64];
   pa_io_event_flags_t flags = 0;
   pa_io_event *event = (pa_io_event *)data;
   int fd = 0;

   fd = ecore_main_fd_handler_fd_get(handler);
   if (fd < 0) return ECORE_CALLBACK_RENEW;

   if (ecore_main_fd_handler_active_get(handler, ECORE_FD_READ))
     {
        flags |= PA_IO_EVENT_INPUT;

        /* Check for HUP and report */
        if (recv(fd, buf, 64, MSG_PEEK))
          {
             if (errno == ESHUTDOWN || errno == ECONNRESET || errno == ECONNABORTED || errno == ENETRESET)
               {
                  DBG("HUP condition detected");
                  flags |= PA_IO_EVENT_HANGUP;
               }
          }
     }

   if (ecore_main_fd_handler_active_get(handler, ECORE_FD_WRITE))
      flags |= PA_IO_EVENT_OUTPUT;
   if (ecore_main_fd_handler_active_get(handler, ECORE_FD_ERROR))
      flags |= PA_IO_EVENT_ERROR;

   event->callback(event->mainloop, event, fd, flags, event->userdata);

   return ECORE_CALLBACK_RENEW;
}

static pa_io_event *
_ecore_pa_io_new(pa_mainloop_api *api, int fd, pa_io_event_flags_t flags, pa_io_event_cb_t cb, void *userdata)
{
   pa_io_event *event;

   event = calloc(1, sizeof(pa_io_event));
   event->mainloop = api;
   event->userdata = userdata;
   event->callback = cb;
   event->flags = flags;
   event->handler = ecore_main_fd_handler_add(fd, map_flags_to_ecore(flags), _ecore_io_wrapper, event, NULL, NULL);

   return event;
}

static void
_ecore_pa_io_enable(pa_io_event *event, pa_io_event_flags_t flags)
{
   event->flags = flags;
   ecore_main_fd_handler_active_set(event->handler, map_flags_to_ecore(flags));
}

static void
_ecore_pa_io_free(pa_io_event *event)
{
   ecore_main_fd_handler_del(event->handler);
   free(event);
}

static void
_ecore_pa_io_set_destroy(pa_io_event *event, pa_io_event_destroy_cb_t cb)
{
   event->destroy_callback = cb;
}

/* Timed events */
struct pa_time_event
{
   pa_mainloop_api *mainloop;
   Ecore_Timer                    *timer;
   struct timeval                  tv;

   void                           *userdata;

   pa_time_event_cb_t              callback;
   pa_time_event_destroy_cb_t      destroy_callback;
};

Eina_Bool
_ecore_time_wrapper(void *data)
{
   pa_time_event *event = (pa_time_event *)data;

   event->callback(event->mainloop, event, &event->tv, event->userdata);

   return ECORE_CALLBACK_CANCEL;
}

pa_time_event *
_ecore_pa_time_new(pa_mainloop_api *api, const struct timeval *tv, pa_time_event_cb_t cb, void *userdata)
{
   pa_time_event *event;
   struct timeval now;
   double interval;

   event = calloc(1, sizeof(pa_time_event));
   event->mainloop = api;
   event->userdata = userdata;
   event->callback = cb;
   event->tv = *tv;

   if (gettimeofday(&now, NULL) == -1)
     {
        ERR("Failed to get the current time!");
        free(event);
        return NULL;
     }

   interval = (tv->tv_sec - now.tv_sec) + (tv->tv_usec - now.tv_usec) / 1000;
   event->timer = ecore_timer_add(interval, _ecore_time_wrapper, event);

   return event;
}

void
_ecore_pa_time_restart(pa_time_event *event, const struct timeval *tv)
{
   struct timeval now;
   double interval;

   /* If tv is NULL disable timer */
   if (!tv)
     {
        ecore_timer_del(event->timer);
        event->timer = NULL;
        return;
     }

   event->tv = *tv;

   if (gettimeofday(&now, NULL) == -1)
     {
        ERR("Failed to get the current time!");
        return;
     }

   interval = (tv->tv_sec - now.tv_sec) + (tv->tv_usec - now.tv_usec) / 1000;
   if (event->timer)
     {
        event->timer = ecore_timer_add(interval, _ecore_time_wrapper, event);
     }
   else
     {
        ecore_timer_interval_set(event->timer, interval);
        ecore_timer_reset(event->timer);
     }
}

void
_ecore_pa_time_free(pa_time_event *event)
{
   if (event->timer)
      ecore_timer_del(event->timer);

   event->timer = NULL;

   free(event);
}

void
_ecore_pa_time_set_destroy(pa_time_event *event, pa_time_event_destroy_cb_t cb)
{
   event->destroy_callback = cb;
}

/* Deferred events */
struct pa_defer_event
{
   pa_mainloop_api *mainloop;
   Ecore_Idler                    *idler;

   void                           *userdata;

   pa_defer_event_cb_t             callback;
   pa_defer_event_destroy_cb_t     destroy_callback;
};

Eina_Bool
_ecore_defer_wrapper(void *data)
{
   pa_defer_event *event = (pa_defer_event *)data;

   event->idler = NULL;
   event->callback(event->mainloop, event, event->userdata);

   return ECORE_CALLBACK_CANCEL;
}

pa_defer_event *
_ecore_pa_defer_new(pa_mainloop_api *api, pa_defer_event_cb_t cb, void *userdata)
{
   pa_defer_event *event;

   event = calloc(1, sizeof(pa_defer_event));
   event->mainloop = api;
   event->userdata = userdata;
   event->callback = cb;

   event->idler = ecore_idler_add(_ecore_defer_wrapper, event);

   return event;
}

void
_ecore_pa_defer_enable(pa_defer_event *event, int b)
{
   if (!b && event->idler)
     {
        ecore_idler_del(event->idler);
        event->idler = NULL;
     }
   else if (b && !event->idler)
     {
        event->idler = ecore_idler_add(_ecore_defer_wrapper, event);
     }
}

void
_ecore_pa_defer_free(pa_defer_event *event)
{
   if (event->idler)
      ecore_idler_del(event->idler);

   event->idler = NULL;

   free(event);
}

void
_ecore_pa_defer_set_destroy(pa_defer_event *event, pa_defer_event_destroy_cb_t cb)
{
   event->destroy_callback = cb;
}

static void
_ecore_pa_quit(pa_mainloop_api *api EINA_UNUSED, int retval EINA_UNUSED)
{
   /* FIXME: Need to clean up timers, etc.? */
   WRN("Not quitting mainloop, although PA requested it");
}

/* Function table for PA mainloop integration */
const pa_mainloop_api functable = {
   .userdata = NULL,

   .io_new = _ecore_pa_io_new,
   .io_enable = _ecore_pa_io_enable,
   .io_free = _ecore_pa_io_free,
   .io_set_destroy = _ecore_pa_io_set_destroy,

   .time_new = _ecore_pa_time_new,
   .time_restart = _ecore_pa_time_restart,
   .time_free = _ecore_pa_time_free,
   .time_set_destroy = _ecore_pa_time_set_destroy,

   .defer_new = _ecore_pa_defer_new,
   .defer_enable = _ecore_pa_defer_enable,
   .defer_free = _ecore_pa_defer_free,
   .defer_set_destroy = _ecore_pa_defer_set_destroy,

   .quit = _ecore_pa_quit,
};

/* *****************************************************
 * Ecore mainloop integration end
 */
