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
   /* FIXME make this a singleton */
   VM= NEW(E_View_Model, 1);
   VM->views = NULL;
   VM->models = NULL;
   e_view_init();
   e_view_model_init();
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


static E_View_Model *
get_model_from_realpath(char *path)   
{
   E_View_Model *m;
   Evas_List l;

   D_ENTER;
   if (path)
   {
      for (l=VM->models; l; l = l->next)
      {
	 m = l->data;
	 if (!strcmp(m->dir, path))
	 {
	    D("Model for this dir already exists\n");
	    e_object_ref (E_OBJECT(m));
	    D_RETURN_(m);
	 }
      }
   }
   D_RETURN_(NULL);
}

void
e_view_machine_get_model(E_View *v, char *path, int is_desktop)
{
   E_View_Model *m = NULL;
   char *realpath;
   char buf[PATH_MAX];

   D_ENTER;
   realpath = e_file_realpath(path);
   if (!(m = get_model_from_realpath(realpath)))
   {
      D("Model for this dir doesnt exist, make a new one\n");
      m = e_view_model_new();
      VM->models = evas_list_append(VM->models, m);
      e_view_model_set_dir(m, path);
      snprintf(buf, PATH_MAX, "%s/.e_background.bg.db", realpath);
      if (!e_file_exists(buf))
      {
	 if (is_desktop)
	 {
	    snprintf(buf, PATH_MAX, "%s/default.bg.db", e_config_get("backgrounds"));
	 }
	 else
	 {
	    snprintf(buf, PATH_MAX, "%s/view.bg.db", e_config_get("backgrounds"));
	 }
      }
      e_strdup(m->bg_file, buf);
      m->is_desktop = is_desktop;
   }
   if (m)
   {
      v->model = m;   
      v->model->views = evas_list_append(v->model->views, v);
      /* FIXME do a real naming scheme here */
      snprintf(buf, PATH_MAX, "%s:%d", v->model->dir, e_object_get_usecount(E_OBJECT(v->model))); 
      e_strdup(v->name, buf);
      D("assigned name to view: %s\n",v->name);    
   }  
   else
   {
      /* FIXME error handling */
   }  
   D_RETURN;
}
