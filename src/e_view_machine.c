#include <Ecore.h>
#include "e_view_machine.h"
#include "e_view_model.h"
#include "util.h"
#include "globals.h"
#include "file.h"

void
e_view_machine_init()
{
   D_ENTER;
   
   if (VM == NULL)
     {
       VM = NEW(E_View_Model, 1);
       VM->views = NULL;
       VM->models = NULL;
       e_view_init();
       e_view_model_init();
     }

   D_RETURN;
}

void
e_view_machine_register_view(E_View *v)
{
   D_ENTER;
   /* Add view to the list of views */
   VM->views = evas_list_append(VM->views, v);
   D_RETURN;
}

void
e_view_machine_unregister_view(E_View *v)
{
   D_ENTER;
   /* Remove the view from the global list of views
    * and from the list of its model. */
   VM->views = evas_list_remove(VM->views, v);
   v->model->views = evas_list_remove(v->model->views, v);
   e_object_unref (E_OBJECT(v->model));
   D_RETURN;
}


E_View_Model *
e_view_machine_model_lookup(char *path)
{
   E_View_Model *m;
   Evas_List l;
   char      *realpath = NULL;

   D_ENTER;

   if (!path)
     D_RETURN_(NULL);
     
   realpath = e_file_realpath(path);

   for (l=VM->models; l; l = l->next)
     {
       m = l->data;
       if (!strcmp(m->dir, realpath))
	 {
	   D("Model for this dir already exists\n");

	   IF_FREE(realpath);

	   e_object_ref (E_OBJECT(m));	   
	   D_RETURN_(m);
	 }
     }

   IF_FREE(realpath);
   D_RETURN_(NULL);
}



