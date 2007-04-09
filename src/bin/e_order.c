/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_order_free       (E_Order *eo);
static void _e_order_cb_monitor (void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path);
static void _e_order_read       (E_Order *eo);
static void _e_order_save       (E_Order *eo);

static int  _e_order_cb_efreet_desktop_change(void *data, int ev_type, void *ev);
static int  _e_order_cb_efreet_desktop_list_change(void *data, int ev_type, void *ev);

static Evas_List *orders = NULL;
static Evas_List *handlers = NULL;

/* externally accessible functions */
EAPI int
e_order_init(void)
{
   handlers = evas_list_append(handlers, ecore_event_handler_add(EFREET_EVENT_DESKTOP_CHANGE, _e_order_cb_efreet_desktop_change, NULL));
   handlers = evas_list_append(handlers, ecore_event_handler_add(EFREET_EVENT_DESKTOP_LIST_CHANGE, _e_order_cb_efreet_desktop_list_change, NULL));

   return 1;
}

EAPI int
e_order_shutdown(void)
{
   orders = evas_list_free(orders);

   while (handlers)
     {
	ecore_event_handler_del(handlers->data);
	handlers = evas_list_remove_list(handlers, handlers);
     }
   return 1;
}

EAPI E_Order *
e_order_new(const char *path)
{
   E_Order *eo;
 
   eo = E_OBJECT_ALLOC(E_Order, E_ORDER_TYPE, _e_order_free);
   if (!eo) return NULL;

   if (path) eo->path = evas_stringshare_add(path);
   _e_order_read(eo);
   eo->monitor = ecore_file_monitor_add(path, _e_order_cb_monitor, eo);

   orders = evas_list_append(orders, eo);

   return eo;
}

EAPI void
e_order_update_callback_set(E_Order *eo, void (*cb)(void *data, E_Order *eo), void *data)
{
   E_OBJECT_CHECK(eo);
   E_OBJECT_TYPE_CHECK(eo, E_ORDER_TYPE);

   eo->cb.update = cb;
   eo->cb.data = data;
}

EAPI void
e_order_remove(E_Order *eo, Efreet_Desktop *desktop)
{
   E_OBJECT_CHECK(eo);
   E_OBJECT_TYPE_CHECK(eo, E_ORDER_TYPE);

   eo->desktops = evas_list_remove(eo->desktops, desktop);
   _e_order_save(eo);
}

EAPI void
e_order_append(E_Order *eo, Efreet_Desktop *desktop)
{
   E_OBJECT_CHECK(eo);
   E_OBJECT_TYPE_CHECK(eo, E_ORDER_TYPE);

   eo->desktops = evas_list_append(eo->desktops, desktop);
   _e_order_save(eo);
}

EAPI void
e_order_prepend_relative(E_Order *eo, Efreet_Desktop *desktop, Efreet_Desktop *before)
{
   E_OBJECT_CHECK(eo);
   E_OBJECT_TYPE_CHECK(eo, E_ORDER_TYPE);

   eo->desktops = evas_list_prepend_relative(eo->desktops, desktop, before);
   _e_order_save(eo);
}

EAPI void
e_order_files_append(E_Order *eo, Evas_List *files)
{
   Evas_List *l;

   E_OBJECT_CHECK(eo);
   E_OBJECT_TYPE_CHECK(eo, E_ORDER_TYPE);

   for (l = files; l ; l = l->next)
     {
	Efreet_Desktop *desktop;
	const char *file;

	file = l->data;
	desktop = efreet_desktop_get(file);
	if (!desktop) continue;
	eo->desktops = evas_list_append(eo->desktops, desktop);
     }
   _e_order_save(eo);
}

EAPI void
e_order_files_prepend_relative(E_Order *eo, Evas_List *files, Efreet_Desktop *before)
{
   Evas_List *l;

   E_OBJECT_CHECK(eo);
   E_OBJECT_TYPE_CHECK(eo, E_ORDER_TYPE);

   for (l = files; l ; l = l->next)
     {
	Efreet_Desktop *desktop;
	const char *file;

	file = l->data;
	desktop = efreet_desktop_get(file);
	if (!desktop) continue;
	eo->desktops = evas_list_prepend_relative(eo->desktops, desktop, before);
     }
   _e_order_save(eo);
}

/* local subsystem functions */
static void
_e_order_free(E_Order *eo)
{
   evas_list_free(eo->desktops);
   if (eo->path) evas_stringshare_del(eo->path);
   if (eo->monitor) ecore_file_monitor_del(eo->monitor);
   orders = evas_list_remove(orders, eo);
   free(eo);
}

static void
_e_order_cb_monitor(void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path)
{
   E_Order *eo;

   eo = data;

   /* It doesn't really matter what the change is, just re-read the file */
   _e_order_read(eo);
   if (eo->cb.update) eo->cb.update(eo->cb.data, eo);
}

static void
_e_order_read(E_Order *eo)
{
   FILE *f;
   char *dir;

   eo->desktops = evas_list_free(eo->desktops);
   if (!eo->path) return;

   dir = ecore_file_get_dir(eo->path);
   f = fopen(eo->path, "rb");
   if (f)
     {
	char buf[4096], buf2[PATH_MAX];

	while (fgets(buf, sizeof(buf), f))
	  {
	     int len;

	     len = strlen(buf);
	     if (len > 0)
	       {
		  if (buf[len - 1] == '\n')
		    {
		       buf[len - 1] = 0;
		       len--;
		    }
		  if (len > 0)
		    {
		       Efreet_Desktop *desktop;
		       
		       desktop = NULL;
		       if ((dir) && (buf[0] != '/'))
			 {
			    snprintf(buf2, sizeof(buf2), "%s/%s", dir, buf);
			    desktop = efreet_desktop_get(buf2);
			 }
		       if (!desktop)
			 {
			    snprintf(buf2, sizeof(buf2), 
				     "%s/.e/e/applications/all/%s",
				     e_user_homedir_get(), buf);
			    desktop = efreet_desktop_get(buf2);
			 }
		       if (!desktop)
			 desktop = efreet_util_desktop_file_id_find(buf);
		       if (desktop) eo->desktops = evas_list_append(eo->desktops, desktop);
		    }
	       }
	  }
	fclose(f);
     }
   if (dir) free(dir);
}

static void
_e_order_save(E_Order *eo)
{
   FILE *f;
   Evas_List *l;

   if (!eo->path) return;
   f = fopen(eo->path, "wb");
   if (!f) return;

   for (l = eo->desktops; l; l = l->next)
     {
	Efreet_Desktop *desktop;
	const char *id;

	desktop = l->data;
	id = efreet_util_path_to_file_id(desktop->orig_path);
	if (id)
	  {
	     fprintf(f, "%s\n", id);
	  }
	else
	  {
	     /* TODO: Check if the file is in ~/.e/e/applications/all */
	     /* TODO: Consider copying the file to $XDG_DATA_HOME/applications */
	     fprintf(f, "%s\n", desktop->orig_path);
	  }
     }

   fclose(f);
}

static int list_changed = 0;

static int
_e_order_cb_efreet_desktop_change(void *data, int ev_type, void *ev)
{
   Efreet_Event_Desktop_Change *event;
   Evas_List *l;

   event = ev;
   if (!list_changed) return 1;
   switch (event->change)
     {
      case EFREET_DESKTOP_CHANGE_ADD:
	 /* If a desktop is added, reread all .order files */
	 for (l = orders; l; l = l->next)
	   {
	      E_Order *eo;

	      eo = l->data;
	      _e_order_read(eo);
	      if (eo->cb.update) eo->cb.update(eo->cb.data, eo);
	   }
	 break;
      case EFREET_DESKTOP_CHANGE_REMOVE:
	 /* If a desktop is removed, drop the .desktop pointer */
	 for (l = orders; l; l = l->next)
	   {
	      E_Order   *eo;
	      Evas_List *l2;
	      int changed = 0;

	      eo = l->data;
	      for (l2 = eo->desktops; l2; l2 = l2->next)
		{
		   if (l2->data == event->current)
		     {
			eo->desktops = evas_list_remove_list(eo->desktops, l2);
			changed = 1;
		     }
		}
	      if ((changed) && (eo->cb.update)) eo->cb.update(eo->cb.data, eo);
	   }
	 break;
      case EFREET_DESKTOP_CHANGE_UPDATE:
	 /* If a desktop is updated, point to the new desktop and update */
	 for (l = orders; l; l = l->next)
	   {
	      E_Order *eo;
	      Evas_List *l2;
	      int changed = 0;

	      eo = l->data;
	      for (l2 = eo->desktops; l2; l2 = l2->next)
		{
		   if (l2->data == event->previous)
		     {
			l2->data = event->current;
			changed = 1;
		     }
		}
	      if ((changed) && (eo->cb.update)) eo->cb.update(eo->cb.data, eo);
	   }
	 break;
     }
   return 1;
}

static int
_e_order_cb_efreet_desktop_list_change(void *data, int ev_type, void *ev)
{
   Evas_List *l;
   
   if (!list_changed)
     {
	list_changed = 1;
	for (l = orders; l; l = l->next)
	  {
	     E_Order *eo;
	     
	     eo = l->data;
	     if (eo->cb.update) eo->cb.update(eo->cb.data, eo);
	  }
     }
   return 1;
}
