/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static const char *title = NULL;
static const char *version = NULL;
static Ecore_Exe *init_exe = NULL;
static Ecore_Event_Handler *exe_del_handler = NULL;
static Ecore_Ipc_Client *client = NULL;
static int done = 0;
static int undone = 0;
static Eina_List *stats = NULL;

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
     s = e_path_find(path_themes, "default.edj");
   else if (e_config->init_default_theme[0] == '/')
     s = eina_stringshare_add(e_config->init_default_theme);
   else
     s = e_path_find(path_themes, e_config->init_default_theme);

   if (s) theme = strdup(e_util_filename_escape(s));
   else theme = strdup("XdX");
   if (s) eina_stringshare_del(s);

   if (title) tit = strdup(e_util_filename_escape(title));
   else tit = strdup("XtX");

   if (version) ver = strdup(e_util_filename_escape(version));
   else ver = strdup("XvX");

   snprintf(buf, sizeof(buf), "%s/enlightenment/utils/enlightenment_init \'%s\' \'%i\' \'%i\' \'%s\' \'%s\'",
	    e_prefix_lib_get(),
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
	stats = eina_list_append(stats, eina_stringshare_add(str));
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
		  Eina_List *l;

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
	     stats = eina_list_remove_list(stats, stats);
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
	Eina_List *l;

	client = NULL;
	for (l = e_manager_list(); l; l = l->next)
	  {
	     E_Manager *man;

	     man = l->data;
	     man->initwin = 0;
	  }
     }
}
