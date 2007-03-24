/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/*
 * TODO:
 * - Update E_Order object if a .desktop file becomes available
 */

static void _e_order_free       (E_Order *eo);
static void _e_order_cb_monitor (void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path);

EAPI E_Order *
e_order_new(const char *path)
{
   E_Order *eo;
   FILE *f;
   
   if ((!path) || (!ecore_file_exists(path))) return NULL;

   eo = E_OBJECT_ALLOC(E_Order, E_ORDER_TYPE, _e_order_free);
   if (!eo) return NULL;

   f = fopen(path, "rb");
   if (f)
     {
	char buf[4096];

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
		       desktop = efreet_util_desktop_by_file_id_get(buf);
		       if (desktop) evas_list_append(eo->desktops, desktop);
		    }
	       }
	  }
	fclose(f);
     }
   eo->monitor = ecore_file_monitor_add(path, _e_order_cb_monitor, eo);

   return eo;
}

static void
_e_order_free(E_Order *eo)
{
   evas_list_free(eo->desktops);
   if (eo->monitor) ecore_file_monitor_del(eo->monitor);
   free(eo);
}

static void
_e_order_cb_monitor(void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path)
{
   /* TODO */
}
