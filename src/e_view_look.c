#include "util.h"
#include "desktops.h"
#include "e_view_look.h"
#include "view.h"
#include "observer.h"
#include "e_file.h"
#include "e_dir.h"
#include "view_layout.h"
#include "e_view_machine.h"

static void e_view_look_file_delete(E_View_Look *l, E_File *f);
static void e_view_look_file_change(E_View_Look *l, E_File *f);
static void e_view_look_cleanup(E_View_Look *l);
static void e_view_look_objects_cleanup(E_View_Look_Objects *o);
static void e_view_look_event_handler(E_Observer *obs, E_Observee *o, E_Event_Type event, void *data);


E_View_Look *
e_view_look_new(void)
{
   E_View_Look *l;
   D_ENTER;

   l = NEW(E_View_Look, 1);
   ZERO(l, E_View_Look, 1);
   e_observer_init(E_OBSERVER(l),
	           E_EVENT_FILE_ADD | E_EVENT_FILE_DELETE |
		   E_EVENT_FILE_CHANGE,
		   (E_Notify_Func) e_view_look_event_handler,
		   (E_Cleanup_Func) e_view_look_cleanup);

   l->obj = NEW(E_View_Look_Objects, 1);
   ZERO(l->obj, E_View_Look_Objects, 1);
	 
   l->obj->bg = NULL;
   l->obj->icb = NULL;
   l->obj->icb_bits = NULL;
   l->obj->layout = NULL;
   e_observee_init(E_OBSERVEE(l->obj), (E_Cleanup_Func) e_view_look_objects_cleanup);

   e_view_machine_register_look(l);

   D_RETURN_(l);
}

static void
e_view_look_cleanup(E_View_Look *l)
{
   D_ENTER;
   e_view_machine_unregister_look(l);
   e_object_unref(E_OBJECT(l->obj));
   e_observer_cleanup(E_OBSERVER(l));
   D_RETURN;
}

static void
e_view_look_objects_cleanup(E_View_Look_Objects *l)
{
   D_ENTER;
   IF_FREE(l->bg);
   IF_FREE(l->icb);
   IF_FREE(l->icb_bits);
   IF_FREE(l->layout);
   e_observee_cleanup(E_OBSERVEE(l));
   D_RETURN;
}

static void
e_view_look_event_handler(E_Observer *obs, E_Observee *o, E_Event_Type event, void *data)
{
   E_View_Look *l = (E_View_Look *) obs;
   E_File *f = (E_File *) data;
   D_ENTER;

   if (event & E_EVENT_FILE_CHANGE || event & E_EVENT_FILE_ADD)
      e_view_look_file_change(l, f);
   else if (event & E_EVENT_FILE_DELETE)
      e_view_look_file_delete(l, f);
   
   D_RETURN;
   UN(o);
}


void
e_view_look_set_dir(E_View_Look *l, char *dir)
{
   E_Dir *d = NULL;
   D_ENTER;

   if (!(d = e_view_machine_dir_lookup(dir)))
     {
	D("Model for this dir doesn't exist, make a new one\n");
	d = e_dir_new();
	e_dir_set_dir(d, dir);
     }
   else	   
      e_object_ref(E_OBJECT(d));
   
   if (d)
     {
	l->dir = d;
	e_observer_register_observee(E_OBSERVER(l), E_OBSERVEE(l->dir));   
     }
   else
     {
	D("Couldnt set dir for E_View_Look! Bad!\n");
     }
   D_RETURN;
}


/* React to file event */

static void
e_view_look_file_change(E_View_Look *l, E_File *f)
{
   char buf[PATH_MAX];
   D_ENTER; 
   snprintf(buf, PATH_MAX, "%s/%s", l->dir->dir, f->file);
   if (!strncmp(f->file, "background.db", PATH_MAX))	 
   {
      IF_FREE(l->obj->bg);
      l->obj->bg = strdup(buf);
      e_observee_notify_observers(E_OBSERVEE(l->obj), E_EVENT_BG_CHANGED, f);
   }
   else if (!strncmp(f->file, "iconbar.db", PATH_MAX))
   {
      IF_FREE(l->obj->icb);
      l->obj->icb = strdup(buf);
      e_observee_notify_observers(E_OBSERVEE(l->obj), E_EVENT_ICB_CHANGED, f);
   }
   else if (!strncmp(f->file, "iconbar.bits.db", PATH_MAX))
   {
      IF_FREE(l->obj->icb_bits);
      l->obj->icb_bits = strdup(buf);
      e_observee_notify_observers(E_OBSERVEE(l->obj), E_EVENT_ICB_CHANGED, f);
   }
   else if (!strncmp(f->file, "layout.db", PATH_MAX))
   {
      IF_FREE(l->obj->layout);
      l->obj->layout = strdup(buf);
      e_observee_notify_observers(E_OBSERVEE(l->obj), E_EVENT_LAYOUT_CHANGED, f);
   }
   D_RETURN;
}

static void
e_view_look_file_delete(E_View_Look *l, E_File *f)
{
   D_ENTER;
   
   if (!strcmp(f->file, "background.db"))	 
   {
      IF_FREE(l->obj->bg);
      l->obj->bg = NULL; 
      e_observee_notify_observers(E_OBSERVEE(l->obj), E_EVENT_BG_CHANGED, f);
   }
   else if ( !strcmp(f->file, "iconbar.db"))
   {
      IF_FREE(l->obj->icb);  
      l->obj->icb = NULL;
      e_observee_notify_observers(E_OBSERVEE(l->obj), E_EVENT_ICB_CHANGED, f);
   }

   else if (!strcmp(f->file, "iconbar.bits.db"))	 
   {
      IF_FREE(l->obj->icb_bits);
      l->obj->icb_bits = NULL;
      e_observee_notify_observers(E_OBSERVEE(l->obj), E_EVENT_ICB_CHANGED, f);
   }
   else if (!strcmp(f->file, "layout.db"))	 
   {
      IF_FREE(l->obj->layout);
      l->obj->layout = NULL;
      e_observee_notify_observers(E_OBSERVEE(l->obj), E_EVENT_LAYOUT_CHANGED, f);
   }

   D_RETURN;
}
