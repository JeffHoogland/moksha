#include "config.h"

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
# ifdef  __cplusplus
extern "C"
# endif
void *alloca(size_t);
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_Ipc.h>
#include <Ecore_File.h>
#include <Evas.h>
#include <Eet.h>
#include <Edje.h>
#include "e_sha1.h"
#include "e_user.h"

typedef struct _E_Thumb E_Thumb;

struct _E_Thumb
{
   int   objid;
   int   w, h;
   char *file;
   char *key;
};

/* local subsystem functions */
static int       _e_ipc_init(void);
static Eina_Bool _e_ipc_cb_server_add(void *data,
                                      int   type,
                                      void *event);
static Eina_Bool _e_ipc_cb_server_del(void *data,
                                      int   type,
                                      void *event);
static Eina_Bool _e_ipc_cb_server_data(void *data,
                                       int   type,
                                       void *event);
static Eina_Bool _e_cb_timer(void *data);
static void      _e_thumb_generate(E_Thumb *eth);
static char     *_e_thumb_file_id(char *file,
                                  char *key);

/* local subsystem globals */
static Ecore_Ipc_Server *_e_ipc_server = NULL;
static Eina_List *_thumblist = NULL;
static Ecore_Timer *_timer = NULL;
static char _thumbdir[4096] = "";

/* externally accessible functions */
int
main(int    argc,
     char **argv)
{
   int i;

   for (i = 1; i < argc; i++)
     {
        if ((!strcmp(argv[i], "-h")) ||
            (!strcmp(argv[i], "-help")) ||
            (!strcmp(argv[i], "--help")))
          {
             printf(
               "This is an internal tool for Enlightenment.\n"
               "do not use it.\n"
               );
             exit(0);
          }
        else if (!strncmp(argv[i], "--nice=", 7))
          {
             const char *val;

             val = argv[i] + 7;
             if (*val)
               nice(atoi(val));
          }
     }

   ecore_init();
   ecore_app_args_set(argc, (const char **)argv);
   eet_init();
   evas_init();
   ecore_evas_init();
   edje_init();
   ecore_file_init();
   ecore_ipc_init();

   e_user_dir_concat_static(_thumbdir, "fileman/thumbnails");
   ecore_file_mkpath(_thumbdir);

   if (_e_ipc_init()) ecore_main_loop_begin();

   if (_e_ipc_server)
     {
        ecore_ipc_server_del(_e_ipc_server);
        _e_ipc_server = NULL;
     }

   ecore_ipc_shutdown();
   ecore_file_shutdown();
   ecore_evas_shutdown();
   edje_shutdown();
   evas_shutdown();
   eet_shutdown();
   ecore_shutdown();

   return 0;
}

/* local subsystem functions */
static int
_e_ipc_init(void)
{
   char *sdir;

   sdir = getenv("E_IPC_SOCKET");
   if (!sdir)
     {
        printf("The E_IPC_SOCKET environment variable is not set. This is\n"
               "exported by Enlightenment to all processes it launches.\n"
               "This environment variable must be set and must point to\n"
               "Enlightenment's IPC socket file (minus port number).\n");
        return 0;
     }
   _e_ipc_server = ecore_ipc_server_connect(ECORE_IPC_LOCAL_SYSTEM, sdir, 0, NULL);
   if (!_e_ipc_server) return 0;

   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_ADD, _e_ipc_cb_server_add, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DEL, _e_ipc_cb_server_del, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DATA, _e_ipc_cb_server_data, NULL);

   return 1;
}

static Eina_Bool
_e_ipc_cb_server_add(void *data __UNUSED__,
                     int type   __UNUSED__,
                     void      *event)
{
   Ecore_Ipc_Event_Server_Add *e;

   e = event;
   ecore_ipc_server_send(e->server,
                         5 /*E_IPC_DOMAIN_THUMB*/,
                         1 /*hello*/,
                         0, 0, 0, NULL, 0); /* send hello */
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_ipc_cb_server_del(void *data  __UNUSED__,
                     int type    __UNUSED__,
                     void *event __UNUSED__)
{
   /* quit now */
    ecore_main_loop_quit();
    return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_ipc_cb_server_data(void *data __UNUSED__,
                      int type   __UNUSED__,
                      void      *event)
{
   Ecore_Ipc_Event_Server_Data *e;
   E_Thumb *eth;
   Eina_List *l;
   char *file = NULL;
   char *key = NULL;

   e = event;
   if (e->major != 5 /*E_IPC_DOMAIN_THUMB*/) return ECORE_CALLBACK_PASS_ON;
   switch (e->minor)
     {
      case 1:
        if (e->data)
          {
             /* begin thumb */
             /* don't check stuff. since this connects TO e17 it is connecting */
             /* TO a trusted process that WILL send this message properly */
             /* formatted. if the thumbnailer dies anyway - it's not a big loss */
             /* but it is a sign of a bug in e formattign messages maybe */
                  file = e->data;
                  key = file + strlen(file) + 1;
                  if (!key[0]) key = NULL;
                  eth = calloc(1, sizeof(E_Thumb));
                  if (eth)
                    {
                       eth->objid = e->ref;
                       eth->w = e->ref_to;
                       eth->h = e->response;
                       eth->file = strdup(file);
                       if (key) eth->key = strdup(key);
                       _thumblist = eina_list_append(_thumblist, eth);
                       if (!_timer) _timer = ecore_timer_add(0.001, _e_cb_timer, NULL);
                    }
          }
        break;

      case 2:
        /* end thumb */
        EINA_LIST_FOREACH(_thumblist, l, eth)
          {
             if (eth->objid == e->ref)
               {
                  _thumblist = eina_list_remove_list(_thumblist, l);
                  if (eth->file) free(eth->file);
                  if (eth->key) free(eth->key);
                  free(eth);
                  break;
               }
          }
        break;

      case 3:
        /* quit now */
        ecore_main_loop_quit();
        break;

      default:
        break;
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_cb_timer(void *data __UNUSED__)
{
   E_Thumb *eth;
   /*
      Eina_List *del_list = NULL, *l;
    */

   /* take thumb at head of list */
   if (_thumblist)
     {
        eth = eina_list_data_get(_thumblist);
        _thumblist = eina_list_remove_list(_thumblist, _thumblist);
        _e_thumb_generate(eth);
        if (eth->file) free(eth->file);
        if (eth->key) free(eth->key);
        free(eth);

        if (_thumblist) _timer = ecore_timer_add(0.01, _e_cb_timer, NULL);
        else _timer = NULL;
     }
   else
     _timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

typedef struct _Color Color;

struct _Color
{
   Color        *closest;
   int           closest_dist;
   int           use;
   unsigned char r, g, b;
};

static void
_e_thumb_generate(E_Thumb *eth)
{
   char buf[4096], dbuf[4096], *id, *td, *ext = NULL;
   Evas *evas = NULL, *evas_im = NULL;
   Ecore_Evas *ee = NULL, *ee_im = NULL;
   Evas_Object *im = NULL, *edje = NULL;
   Eet_File *ef;
   int iw, ih, alpha, ww, hh;
   const unsigned int *data = NULL;
   time_t mtime_orig, mtime_thumb;

   id = _e_thumb_file_id(eth->file, eth->key);
   if (!id) return;

   td = strdup(id);
   if (!td)
     {
        free(id);
        return;
     }
   td[2] = 0;

   snprintf(dbuf, sizeof(dbuf), "%s/%s", _thumbdir, td);
   snprintf(buf, sizeof(buf), "%s/%s/%s-%ix%i.thm",
            _thumbdir, td, id + 2, eth->w, eth->h);
   free(id);
   free(td);

   mtime_orig = ecore_file_mod_time(eth->file);
   mtime_thumb = ecore_file_mod_time(buf);
   if (mtime_thumb <= mtime_orig)
     {
        ecore_file_mkdir(dbuf);

        edje_file_cache_set(0);
        edje_collection_cache_set(0);
        ee = ecore_evas_buffer_new(1, 1);
        evas = ecore_evas_get(ee);
        evas_image_cache_set(evas, 0);
        evas_font_cache_set(evas, 0);
        ww = 0;
        hh = 0;
        alpha = 1;
        ext = strrchr(eth->file, '.');

        if ((ext) && (eth->key) &&
            ((!strcasecmp(ext, ".edj")) ||
             (!strcasecmp(ext, ".eap"))))
          {
             ww = eth->w;
             hh = eth->h;
             im = ecore_evas_object_image_new(ee);
             ee_im = evas_object_data_get(im, "Ecore_Evas");
             evas_im = ecore_evas_get(ee_im);
             evas_image_cache_set(evas_im, 0);
             evas_font_cache_set(evas_im, 0);
             evas_object_image_size_set(im, ww * 4, hh * 4);
             evas_object_image_fill_set(im, 0, 0, ww, hh);
             edje = edje_object_add(evas_im);
             if ((eth->key) &&
                 ((!strcmp(eth->key, "e/desktop/background")) ||
                  (!strcmp(eth->key, "e/init/splash"))))
               alpha = 0;
             if (edje_object_file_set(edje, eth->file, eth->key))
               {
                  evas_object_move(edje, 0, 0);
                  evas_object_resize(edje, ww * 4, hh * 4);
                  evas_object_show(edje);
               }
          }
        else
          {
             im = evas_object_image_add(evas);
             evas_object_image_load_size_set(im, eth->w, eth->h);
             evas_object_image_file_set(im, eth->file, NULL);
             iw = 0; ih = 0;
             evas_object_image_size_get(im, &iw, &ih);
             alpha = evas_object_image_alpha_get(im);
             if ((iw > 0) && (ih > 0))
               {
                  ww = eth->w;
                  hh = (eth->w * ih) / iw;
                  if (hh > eth->h)
                    {
                       hh = eth->h;
                       ww = (eth->h * iw) / ih;
                    }
                  evas_object_image_fill_set(im, 0, 0, ww, hh);
               }
          }
        evas_object_move(im, 0, 0);
        evas_object_resize(im, ww, hh);
        ecore_evas_resize(ee, ww, hh);
        evas_object_show(im);
        if (ww > 0)
          {
             data = ecore_evas_buffer_pixels_get(ee);
             if (data)
               {
                  ef = eet_open(buf, EET_FILE_MODE_WRITE);
                  if (ef)
                    {
                       eet_write(ef, "/thumbnail/orig_file",
                                 eth->file, strlen(eth->file), 1);
                       if (eth->key)
                         eet_write(ef, "/thumbnail/orig_key",
                                   eth->key, strlen(eth->key), 1);
                       eet_data_image_write(ef, "/thumbnail/data",
                                            (void *)data, ww, hh, alpha,
                                            0, 91, 1);
                       ww = 4; hh = 4;
                       evas_object_image_fill_set(im, 0, 0, ww, hh);
                       evas_object_resize(im, ww, hh);
                       ecore_evas_resize(ee, ww, hh);
                       data = ecore_evas_buffer_pixels_get(ee);
                       if (data)
                         {
                            unsigned int *data1;

                            data1 = malloc(ww * hh * sizeof(unsigned int));
                            memcpy(data1, data, ww * hh * sizeof(unsigned int));
                            ww = 2; hh = 2;
                            evas_object_image_fill_set(im, 0, 0, ww, hh);
                            evas_object_resize(im, ww, hh);
                            ecore_evas_resize(ee, ww, hh);
                            data = ecore_evas_buffer_pixels_get(ee);
                            if (data)
                              {
                                 unsigned int *data2;

                                 data2 = malloc(ww * hh * sizeof(unsigned int));
                                 memcpy(data2, data, ww * hh * sizeof(unsigned int));
                                 ww = 1; hh = 1;
                                 evas_object_image_fill_set(im, 0, 0, ww, hh);
                                 evas_object_resize(im, ww, hh);
                                 ecore_evas_resize(ee, ww, hh);
                                 data = ecore_evas_buffer_pixels_get(ee);
                                 if (data)
                                   {
                                      unsigned int *data3;
                                      unsigned char id[(21 * 4) + 1];
                                      int n, i;
                                      int hi, si, vi;
                                      float h, s, v;
                                      const int pat2[4] =
                                      {
                                         0, 3, 1, 2
                                      };
                                      const int pat1[16] =
                                      {
                                         5, 10, 6, 9,
                                         0, 15, 3, 12,
                                         1, 14, 7, 8,
                                         4, 11, 2, 13
                                      };

                                      data3 = malloc(ww * hh * sizeof(unsigned int));
                                      memcpy(data3, data, ww * hh * sizeof(unsigned int));
     // sort_id
                                      n = 0;
#define A(v) (((v) >> 24) & 0xff)
#define R(v) (((v) >> 16) & 0xff)
#define G(v) (((v) >> 8) & 0xff)
#define B(v) (((v)) & 0xff)
#define HSV(p)                                         \
  evas_color_rgb_to_hsv(R(p), G(p), B(p), &h, &s, &v); \
  hi = 20 * (h / 360.0);                               \
  si = 20 * s;                                         \
  vi = 20 * v;                                         \
  if (si < 2) hi = 25;
#define SAVEHSV(h, s, v) \
  id[n++] = 'a' + h;     \
  id[n++] = 'a' + v;     \
  id[n++] = 'a' + s;
#define SAVEX(x) \
  id[n++] = 'a' + x;
#if 0
                                      HSV(data3[0]);
                                      SAVEHSV(hi, si, vi);
                                      for (i = 0; i < 4; i++)
                                        {
                                           HSV(data2[pat2[i]]);
                                           SAVEHSV(hi, si, vi);
                                        }
                                      for (i = 0; i < 16; i++)
                                        {
                                           HSV(data1[pat1[i]]);
                                           SAVEHSV(hi, si, vi);
                                        }
#else
                                      HSV(data3[0]);
                                      SAVEX(hi);
                                      for (i = 0; i < 4; i++)
                                        {
                                           HSV(data2[pat2[i]]);
                                           SAVEX(hi);
                                        }
                                      for (i = 0; i < 16; i++)
                                        {
                                           HSV(data1[pat1[i]]);
                                           SAVEX(hi);
                                        }
                                      HSV(data3[0]);
                                      SAVEX(vi);
                                      for (i = 0; i < 4; i++)
                                        {
                                           HSV(data2[pat2[i]]);
                                           SAVEX(vi);
                                        }
                                      for (i = 0; i < 16; i++)
                                        {
                                           HSV(data1[pat1[i]]);
                                           SAVEX(vi);
                                        }
                                      HSV(data3[0]);
                                      SAVEX(si);
                                      for (i = 0; i < 4; i++)
                                        {
                                           HSV(data2[pat2[i]]);
                                           SAVEX(si);
                                        }
                                      for (i = 0; i < 16; i++)
                                        {
                                           HSV(data1[pat1[i]]);
                                           SAVEX(si);
                                        }
#endif
                                      id[n++] = 0;
                                      eet_write(ef, "/thumbnail/sort_id", id, n, 1);
                                      free(data3);
                                   }
                                 free(data2);
                              }
                            free(data1);
                         }
                       eet_close(ef);
                    }
               }
          }

        /* will free all */
        if (edje) evas_object_del(edje);
        if (ee_im) ecore_evas_free(ee_im);
        else if (im)
          evas_object_del(im);
        ecore_evas_free(ee);
        eet_clearcache();
     }
   /* send back path to thumb */
   ecore_ipc_server_send(_e_ipc_server, 5, 2, eth->objid, 0, 0, buf, strlen(buf) + 1);
}

static char *
_e_thumb_file_id(char *file,
                 char *key)
{
   char s[64];
   const char *chmap = "0123456789abcdef";
   unsigned char *buf, id[20];
   int i, len, lenf;

   len = 0;
   lenf = strlen(file);
   len += lenf;
   len++;
   if (key)
     {
        key += strlen(key);
        len++;
     }
   buf = alloca(len);

   strcpy((char *)buf, file);
   if (key) strcpy((char *)(buf + lenf + 1), key);

   e_sha1_sum(buf, len, id);

   for (i = 0; i < 20; i++)
     {
        s[(i * 2) + 0] = chmap[(id[i] >> 4) & 0xf];
        s[(i * 2) + 1] = chmap[(id[i]) & 0xf];
     }
   s[(i * 2)] = 0;
   return strdup(s);
}

