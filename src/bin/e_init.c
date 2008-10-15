/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#if 1

static const char *title = NULL;
static const char *version = NULL;
static Ecore_Exe *init_exe = NULL;
static Ecore_Event_Handler *exe_del_handler = NULL;
static Ecore_Ipc_Client *client = NULL;
static int done = 0;
static int undone = 0;
static Evas_List *stats = NULL;

static int
_e_init_cb_exe_event_del(void *data, int type, void *event)
{
   Ecore_Exe_Event_Del *ev;

   ev = event;
   if (ev->exe == init_exe)
     {
	/* init exited */
//	ecore_exe_free(init_exe);
	init_exe = NULL;
     }
   return 1;
}

EAPI int
e_init_init(void)
{
   exe_del_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
					      _e_init_cb_exe_event_del,
					      NULL);
   client = NULL;
   done = 0;
   return 1;
}

EAPI int
e_init_shutdown(void)
{
   /* if not killed, kill init */
   e_init_hide();
   if (title) eina_stringshare_del(title);
   if (version) eina_stringshare_del(version);
   title = NULL;
   version = NULL;
   ecore_event_handler_del(exe_del_handler);
   exe_del_handler = NULL;
   return 1;
}

EAPI void
e_init_show(void)
{
   char buf[8192], *theme, *tit, *ver;
   const char *s = NULL;

   /* exec init */

   if (!e_config->init_default_theme)
     s = e_path_find(path_init, "default.edj");
   else if (e_config->init_default_theme[0] == '/')
     s = eina_stringshare_add(e_config->init_default_theme);
   else
     s = e_path_find(path_init, e_config->init_default_theme);

   if (s) theme = strdup(e_util_filename_escape(s));
   else theme = strdup("XdX");
   if (s) eina_stringshare_del(s);

   if (title) tit = strdup(e_util_filename_escape(title));
   else tit = strdup("XtX");

   if (version) ver = strdup(e_util_filename_escape(version));
   else ver = strdup("XvX");

   snprintf(buf, sizeof(buf), "%s/enlightenment_init \'%s\' \'%i\' \'%i\' \'%s\' \'%s\'",
	    e_prefix_bin_get(),
	    theme,
	    e_canvas_engine_decide(e_config->evas_engine_init),
	    e_config->font_hinting,
	    tit, ver);
   printf("RUN INIT: %s\n", buf);
   free(theme);
   free(tit);
   free(ver);
   /* FIXME: add font path to cmd-line */
   init_exe = ecore_exe_run(buf, NULL);
}

EAPI void
e_init_hide(void)
{
   if (init_exe) ecore_exe_terminate(init_exe);
}

EAPI void
e_init_title_set(const char *str)
{
   if (title) eina_stringshare_del(title);
   title = eina_stringshare_add(str);
}

EAPI void
e_init_version_set(const char *str)
{
   if (version) eina_stringshare_del(version);
   version = eina_stringshare_add(str);
}

EAPI void
e_init_status_set(const char *str)
{
   if (!init_exe) return;
//   printf("---STAT %p %s\n", client, str);
   if (!client)
     {
	stats = evas_list_append(stats, eina_stringshare_add(str));
	return;
     }
//   printf("---SEND\n");
   ecore_ipc_client_send(client, E_IPC_DOMAIN_INIT, 1, 0, 0, 0, (void *)str,
			 strlen(str) + 1);
   ecore_ipc_client_flush(client);
}

EAPI void
e_init_done(void)
{
   undone--;
   if (undone > 0) return;
   done = 1;
//   printf("---DONE %p\n", client);
   if (!client) return;
   ecore_ipc_client_send(client, E_IPC_DOMAIN_INIT, 2, 0, 0, 0, NULL, 0);
   ecore_ipc_client_flush(client);
}

EAPI void
e_init_undone(void)
{
   undone++;
}


EAPI void
e_init_client_data(Ecore_Ipc_Event_Client_Data *e)
{
//   printf("---new init client\n");
   if (!client) client = e->client;
   if (e->minor == 1)
     {
	if (e->data)
	  {
	     int i, num;
	     Ecore_X_Window *initwins;

	     num = e->size / sizeof(Ecore_X_Window);
	     initwins = e->data;
	     for (i = 0; i < num; i+= 2)
	       {
		  Evas_List *l;

		  for (l = e_manager_list(); l; l = l->next)
		    {
		       E_Manager *man;

		       man = l->data;
		       if (man->root == initwins[i + 0])
			 {
			    man->initwin = initwins[i + 1];
			    ecore_x_window_raise(man->initwin);
			 }
		    }
	       }
	  }
	while (stats)
	  {
	     const char *s;

	     s = stats->data;
	     stats = evas_list_remove_list(stats, stats);
//	     printf("---SPOOL %s\n", s);
	     e_init_status_set(s);
	     eina_stringshare_del(s);
	  }
     }
   else if (e->minor == 2)
     {
	e_config->show_splash = e->ref;
	e_config_save_queue();
     }
   if (done) e_init_done();
}

EAPI void
e_init_client_del(Ecore_Ipc_Event_Client_Del *e)
{
//   printf("---del init client\n");
   if (e->client == client)
     {
	Evas_List *l;

	client = NULL;
	for (l = e_manager_list(); l; l = l->next)
	  {
	     E_Manager *man;

	     man = l->data;
	     man->initwin = 0;
	  }
     }
}




































#else /* OLD INIT CODE */
static void _e_init_icons_del(void);
static void _e_init_cb_signal_disable(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_init_cb_signal_enable(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_init_cb_signal_done_ok(void *data, Evas_Object *obj, const char *emission, const char *source);
static int _e_init_cb_window_configure(void *data, int ev_type, void *ev);
static int _e_init_cb_timeout(void *data);

/* local subsystem globals */
static Ecore_X_Window  _e_init_root_win = 0;
static Ecore_X_Window  _e_init_win = 0;
static Ecore_Evas     *_e_init_ecore_evas = NULL;
static Evas           *_e_init_evas = NULL;
static Evas_Object    *_e_init_object = NULL;
static Evas_Object    *_e_init_icon_box = NULL;
static E_Pointer      *_e_init_pointer = NULL;
static Ecore_Event_Handler *_e_init_configure_handler = NULL;
static Ecore_Timer         *_e_init_timeout_timer = NULL;

/* startup icons */
static Evas_Coord _e_init_icon_size = 0;
static Evas_List *_e_init_icon_list = NULL;

/* externally accessible functions */
EAPI int
e_init_init(void)
{
   int w, h;
   Ecore_X_Window root;
   Ecore_X_Window *roots;
   int num;
   Evas_Object *o;
   Evas_List *l, *screens;
   const char *s;

   _e_init_configure_handler =
     ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE,
			     _e_init_cb_window_configure, NULL);

   num = 0;
   roots = ecore_x_window_root_list(&num);
   if ((!roots) || (num <= 0))
     {
        e_error_message_show(_("X reports there are no root windows and %i screens!\n"),
			     num);
	return 0;
     }
   root = roots[0];
   _e_init_root_win = root;

   ecore_x_window_size_get(root, &w, &h);
   _e_init_ecore_evas = e_canvas_new(e_config->evas_engine_init, root,
				     0, 0, w, h, 1, 1,
				     &(_e_init_win), NULL);
   e_canvas_add(_e_init_ecore_evas);
   _e_init_evas = ecore_evas_get(_e_init_ecore_evas);
   ecore_evas_name_class_set(_e_init_ecore_evas, "E", "Init_Window");
   ecore_evas_title_set(_e_init_ecore_evas, "Enlightenment Init");

   _e_init_pointer = e_pointer_window_new(_e_init_win, 1);

   ecore_evas_raise(_e_init_ecore_evas);
   ecore_evas_show(_e_init_ecore_evas);

   if (!e_config->init_default_theme)
     s = e_path_find(path_init, "default.edj");
   else if (e_config->init_default_theme[0] == '/')
     s = eina_stringshare_add(e_config->init_default_theme);
   else
     s = e_path_find(path_init, e_config->init_default_theme);

   screens = (Evas_List *)e_xinerama_screens_get();
   if (screens)
     {
	for (l = screens; l; l = l->next)
	  {
	     E_Screen *scr;

	     scr = l->data;
	     o = edje_object_add(_e_init_evas);
	     /* first screen */
	     if (l == screens)
	       {
		  edje_object_file_set(o, s, "e/init/splash");
		  _e_init_object = o;
	       }
	     /* other screens */
	     else
	       edje_object_file_set(o, s, "e/init/extra_screen");
	     evas_object_move(o, scr->x, scr->y);
	     evas_object_resize(o, scr->w, scr->h);
	     evas_object_show(o);
	  }
     }
   else
     {
	o = edje_object_add(_e_init_evas);
	edje_object_file_set(o, s, "e/init/splash");
	if (s) eina_stringshare_del(s);
	_e_init_object = o;
	evas_object_move(o, 0, 0);
	evas_object_resize(o, w, h);
	evas_object_show(o);
     }
   if (s) eina_stringshare_del(s);

   edje_object_part_text_set(_e_init_object, "e.text.disable_text",
			     _("Disable splash screen"));
   edje_object_signal_callback_add(_e_init_object, "e,action,init,disable", "e",
				   _e_init_cb_signal_disable, NULL);
   edje_object_signal_callback_add(_e_init_object, "e,action,init,enable", "e",
				   _e_init_cb_signal_enable, NULL);
   edje_object_signal_callback_add(_e_init_object, "e,state,done_ok", "e",
				   _e_init_cb_signal_done_ok, NULL);
   free(roots);
   return 1;
}

EAPI int
e_init_shutdown(void)
{
   ecore_event_handler_del(_e_init_configure_handler);
   _e_init_configure_handler = NULL;
   e_init_hide();
   e_canvas_cache_flush();
   return 1;
}

EAPI void
e_init_show(void)
{
   if (!_e_init_ecore_evas) return;
   ecore_evas_raise(_e_init_ecore_evas);
   ecore_evas_show(_e_init_ecore_evas);
}

EAPI void
e_init_hide(void)
{
   /* FIXME: emit signal to edje and wait for it to respond or until a */
   /* in case the edje was badly created and never responds */
   if (!_e_init_ecore_evas) return;
   _e_init_icons_del();
   ecore_evas_hide(_e_init_ecore_evas);
   evas_object_del(_e_init_object);
   e_canvas_del(_e_init_ecore_evas);
   ecore_evas_free(_e_init_ecore_evas);

   if (_e_init_pointer)
     {
	e_object_del(E_OBJECT(_e_init_pointer));
	_e_init_pointer = NULL;
     }

   _e_init_ecore_evas = NULL;
   _e_init_evas = NULL;
   _e_init_win = 0;
   _e_init_object = NULL;
   e_canvas_cache_flush();
}

EAPI void
e_init_title_set(const char *str)
{
   if (!_e_init_object) return;
   edje_object_part_text_set(_e_init_object, "e.text.title", str);
}

EAPI void
e_init_version_set(const char *str)
{
   if (!_e_init_object) return;
   edje_object_part_text_set(_e_init_object, "e.text.version", str);
}

EAPI void
e_init_status_set(const char *str)
{
   if (!_e_init_object) return;
   edje_object_part_text_set(_e_init_object, "e.text.status", str);
}

EAPI Ecore_X_Window
e_init_window_get(void)
{
   return _e_init_win;
}

EAPI void
e_init_done(void)
{
   if (!_e_init_object) return;
   edje_object_signal_emit(_e_init_object, "e,state,done", "e");
   _e_init_timeout_timer = ecore_timer_add(30.0, _e_init_cb_timeout, NULL);
}

EAPI void
e_init_icons_desktop_add(Efreet_Desktop *desktop)
{
   Evas_Object *o;
   char buf[128];

   if (!_e_init_evas) return;

   if (!_e_init_icon_box)
     {
	Evas_Coord w, h;

	o = e_box_add(_e_init_evas);
	_e_init_icon_box = o;
	e_box_homogenous_set(o, 1);
	e_box_align_set(o, 0.5, 0.5);
	edje_object_part_swallow(_e_init_object, "e.swallow.icons", o);
	evas_object_geometry_get(o, NULL, NULL, &w, &h);
	if (w > h)
	  {
	     _e_init_icon_size = h;
	     e_box_orientation_set(o, 1);
	  }
	else
	  {
	     _e_init_icon_size = w;
	     e_box_orientation_set(o, 0);
	  }
	evas_object_show(o);
     }

   snprintf(buf, sizeof(buf), "%dx%d", _e_init_icon_size, _e_init_icon_size);
   o = e_util_desktop_icon_add(desktop, buf, _e_init_evas);
   if (o)
     {
	evas_object_resize(o, _e_init_icon_size, _e_init_icon_size);
	e_box_pack_end(_e_init_icon_box, o);
	e_box_pack_options_set(o,
			       0, 0,
			       0, 0,
			       0.5, 0.5,
			       _e_init_icon_size, _e_init_icon_size,
			       _e_init_icon_size, _e_init_icon_size);
	evas_object_show(o);
	_e_init_icon_list = evas_list_append(_e_init_icon_list, o);
     }
}

static void
_e_init_icons_del(void)
{
   Evas_Object *next;

   while (_e_init_icon_list)
     {
	next = _e_init_icon_list->data;
	evas_object_del(next);
	_e_init_icon_list = evas_list_remove(_e_init_icon_list, next);
     }
   if (_e_init_icon_box)
     evas_object_del(_e_init_icon_box);
   _e_init_icon_box = NULL;
}

static void
_e_init_cb_signal_disable(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   e_config->show_splash = 0;
   e_config_save_queue();
}

static void
_e_init_cb_signal_enable(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   e_config->show_splash = 1;
   e_config_save_queue();
}

static void
_e_init_cb_signal_done_ok(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   e_init_hide();
   if (_e_init_timeout_timer)
     {
	ecore_timer_del(_e_init_timeout_timer);
	_e_init_timeout_timer = NULL;
     }
}

static int
_e_init_cb_window_configure(void *data, int ev_type, void *ev)
{
   Ecore_X_Event_Window_Configure *e;

   e = ev;
   /* really simple - don't handle xinerama - because this event will only
    * happen in single head */
   if (e->win != _e_init_root_win) return 1;
   ecore_evas_resize(_e_init_ecore_evas, e->w, e->h);
   evas_object_resize(_e_init_object, e->w, e->h);
   return 1;
}

static int
_e_init_cb_timeout(void *data)
{
   e_init_hide();
   _e_init_timeout_timer = NULL;
   e_util_dialog_internal(_("Theme Bug Detected"),
			  _("The theme you are using for your init splash<br>"
			    "has a bug. It does not respond to signals when<br>"
			    "startup is complete. You should use an init<br>"
			    "splash theme that is correctly made or fix the<br>"
			    "one you use."));
   return 0;
}
#endif
