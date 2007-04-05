/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/*
 * TODO:
 * - Update E_Order object if a .desktop file becomes available
 */

/* local subsystem functions */
static void _e_order_free       (E_Order *eo);
static void _e_order_cb_monitor (void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path);
static void _e_order_read       (E_Order *eo);
static void _e_order_save       (E_Order *eo);

static Evas_List *orders = NULL;

/* externally accessible functions */
EAPI int
e_order_init(void)
{
   return 1;
}

EAPI int
e_order_shutdown(void)
{
   orders = evas_list_free(orders);
   return 1;
}

EAPI E_Order *
e_order_new(const char *path)
{
   E_Order *eo;
 
   if ((!path) || (!ecore_file_exists(path))) return NULL;

   eo = E_OBJECT_ALLOC(E_Order, E_ORDER_TYPE, _e_order_free);
   if (!eo) return NULL;

   eo->path = evas_stringshare_add(path);
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

   f = fopen(eo->path, "wb");
   if (!f) return;

   for (l = eo->desktops; l; l = l->next)
     {
	Efreet_Desktop *desktop;
	const char *id;

	/* TODO: This only allows us to save .desktop files which are in
	 * the default paths. If it isn't, we should copy it to the users
	 * application directory. Or store the full path in the .order file */
	desktop = l->data;
	id = efreet_util_path_to_file_id(desktop->orig_path);
	if (!id) continue;
	fprintf(f, "%s\n", id);
     }

   fclose(f);
}
