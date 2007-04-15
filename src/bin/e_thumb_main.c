/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
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
static int _e_ipc_init(void);
static int _e_ipc_cb_server_add(void *data, int type, void *event);
static int _e_ipc_cb_server_del(void *data, int type, void *event);
static int _e_ipc_cb_server_data(void *data, int type, void *event);
static int _e_cb_timer(void *data);
static void _e_thumb_generate(E_Thumb *eth);
static char *_e_thumb_file_id(char *file, char *key);

/* local subsystem globals */
static Ecore_Ipc_Server *_e_ipc_server = NULL;
static Evas_List *_thumblist = NULL;
static Ecore_Timer *_timer = NULL;
static char _thumbdir[4096] = "";

/* externally accessible functions */
int
main(int argc, char **argv)
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

   snprintf(_thumbdir, sizeof(_thumbdir), "%s/.e/e/fileman/thumbnails", 
	    e_user_homedir_get());
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

static int
_e_ipc_cb_server_add(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Add *e;
   
   e = event;
   ecore_ipc_server_send(e->server, 
			 5/*E_IPC_DOMAIN_THUMB*/, 
			 1/*hello*/, 
			 0, 0, 0, NULL, 0); /* send hello */
   return 1;
}

static int
_e_ipc_cb_server_del(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Del *e;
   
   e = event;
   /* quit now */
   ecore_main_loop_quit();
   return 1;
}

static int
_e_ipc_cb_server_data(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Data *e;
   E_Thumb *eth;
   Evas_List *l;
   char *file = NULL;
   char *key = NULL;
   
   e = event;
   if (e->major != 5/*E_IPC_DOMAIN_THUMB*/) return 1;
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
		  _thumblist = evas_list_append(_thumblist, eth);
		  if (!_timer) _timer = ecore_timer_add(0.001, _e_cb_timer, NULL);
	       }
	  }
	break;
      case 2:
	/* end thumb */
	for (l = _thumblist; l; l = l->next)
	  {
	     eth = l->data;
	     if (eth->objid == e->ref)
	       {
		  _thumblist = evas_list_remove_list(_thumblist, l);
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
   return 1;
}

static int
_e_cb_timer(void *data)
{
   E_Thumb *eth;
   /*
   Evas_List *del_list = NULL, *l;
   */

   /* take thumb at head of list */
   if (_thumblist)
     {
	eth = _thumblist->data;
	_thumblist = evas_list_remove_list(_thumblist, _thumblist);
	_e_thumb_generate(eth);
	if (eth->file) free(eth->file);
	if (eth->key) free(eth->key);
	free(eth);

	if (_thumblist) _timer = ecore_timer_add(0.01, _e_cb_timer, NULL);
	else _timer = NULL;
     }
   else
     _timer = NULL;
   return 0;
}

static void
_e_thumb_generate(E_Thumb *eth)
{
   char buf[4096], dbuf[4096], *id, *td, *ext = NULL;
   Evas *evas = NULL, *evas_im = NULL;
   Ecore_Evas *ee = NULL, *ee_im = NULL;
   Evas_Object *im = NULL, *edje = NULL;
   Eet_File *ef;
   int iw, ih, alpha, ww, hh;
   int *data = NULL;
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
	     (!strcasecmp(ext, ".eap")))
	    )
	  {
	     ww = eth->w;
	     hh = eth->h;
	     im = ecore_evas_object_image_new(ee);
	     ee_im = evas_object_data_get(im, "Ecore_Evas");
	     evas_im = ecore_evas_get(ee_im);
	     evas_image_cache_set(evas_im, 0);
	     evas_font_cache_set(evas_im, 0);
	     evas_object_image_size_set(im, ww * 8, hh * 8);
	     evas_object_image_fill_set(im, 0, 0, ww, hh);
	     edje = edje_object_add(evas_im);
	     if ((eth->key) && 
		 ( (!strcmp(eth->key, "e/desktop/background")) ||
		   (!strcmp(eth->key, "e/init/splash")) ))
	       alpha = 0;
	     if (edje_object_file_set(edje, eth->file, eth->key))
	       {
		  evas_object_move(edje, 0, 0);
		  evas_object_resize(edje, ww * 8, hh * 8);
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
	     data = (int *)ecore_evas_buffer_pixels_get(ee);
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
		       eet_close(ef);
		    }
	       }
	  }
	/* will free all */
	if (edje) evas_object_del(edje);
	if (ee_im) ecore_evas_free(ee_im);
	else if (im) evas_object_del(im);
	ecore_evas_free(ee);
	eet_clearcache();
     }
   /* send back path to thumb */
   ecore_ipc_server_send(_e_ipc_server, 5, 2, eth->objid, 0, 0, buf, strlen(buf) + 1);
}

static char *
_e_thumb_file_id(char *file, char *key)
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
	s[(i * 2) + 1] = chmap[(id[i]     ) & 0xf];
     }
   s[(i * 2)] = 0;
   return strdup(s);
}
