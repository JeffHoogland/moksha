#include <Ecore.h>
#include "e_view_machine.h"
#include "e_dir.h"
#include "e_view_look.h"
#include "util.h"
#include "globals.h"
#include "file.h"

void
e_view_machine_init()
{
   D_ENTER;

   if (VM == NULL)
     {
	VM = NEW(E_Dir, 1);
	VM->views = NULL;
	VM->dirs = NULL;
	VM->looks = NULL;
	e_view_init();
	e_dir_init();
     }

   D_RETURN;
}

void
e_view_machine_register_dir(E_Dir * d)
{
   D_ENTER;
   VM->dirs = evas_list_append(VM->dirs, d);
   D_RETURN;
}

void
e_view_machine_unregister_dir(E_Dir * d)
{
   D_ENTER;
   VM->dirs = evas_list_remove(VM->dirs, d);
   D_RETURN;
}

void
e_view_machine_register_view(E_View * v)
{
   D_ENTER;
   VM->views = evas_list_append(VM->views, v);
   D_RETURN;
}

void
e_view_machine_unregister_view(E_View * v)
{
   D_ENTER;
   VM->views = evas_list_remove(VM->views, v);
   D_RETURN;
}

void
e_view_machine_register_look(E_View_Look * l)
{
   D_ENTER;
   VM->looks = evas_list_append(VM->looks, l);
   D_RETURN;
}

void
e_view_machine_unregister_look(E_View_Look * l)
{
   D_ENTER;
   VM->looks = evas_list_remove(VM->looks, l);
   D_RETURN;
}

void
e_view_machine_close_all_views(void)
{
   Evas_List *           l;

   D_ENTER;
   /* Copy the list of views and unregister them */
   for (l = VM->views; l; l = l->next)
     {
	E_View             *v = l->data;

	e_object_unref(E_OBJECT(v->dir));
     }
   D_RETURN;
}

E_Dir       *
e_view_machine_dir_lookup(char *path)
{
   E_Dir       *d;
   Evas_List *           l;
   char               *realpath = NULL;

   D_ENTER;

   if (!path)
      D_RETURN_(NULL);

   realpath = e_file_realpath(path);

   for (l = VM->dirs; l; l = l->next)
     {
	d = l->data;
	if (!strcmp(d->dir, realpath))
	  {
	     D("E_Dir for this dir already exists\n");

	     IF_FREE(realpath);
	     D_RETURN_(d);
	  }
     }

   IF_FREE(realpath);
   D_RETURN_(NULL);
}

E_View_Look       *
e_view_machine_look_lookup(char *path)
{
   E_View_Look       *vl;
   Evas_List *           l;
   char               *realpath = NULL;

   D_ENTER;

   if (!path)
      D_RETURN_(NULL);

   realpath = e_file_realpath(path);

   for (l = VM->looks; l; l = l->next)
     {
	vl = l->data;
	if (!strcmp(vl->dir->dir, realpath))
	  {
	     D("E_Dir for this dir already exists\n");

	     IF_FREE(realpath);
	     D_RETURN_(vl);
	  }
     }

   IF_FREE(realpath);
   D_RETURN_(NULL);
}


E_View             *
e_view_machine_get_view_by_main_window(Window win)
{
   Evas_List *           l;

   D_ENTER;
   for (l = VM->views; l; l = l->next)
     {
	E_View             *v = l->data;

	if (v && win == v->win.main)
	   D_RETURN_(v);
     }
   D_RETURN_(NULL);
}

E_View             *
e_view_machine_get_view_by_base_window(Window win)
{
   Evas_List *           l;

   D_ENTER;
   for (l = VM->views; l; l = l->next)
     {
	E_View             *v = l->data;

	if (v && win == v->win.base)
	   D_RETURN_(v);
     }
   D_RETURN_(NULL);
}
