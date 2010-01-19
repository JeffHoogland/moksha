#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_comp.h"
#include "config.h"

struct _E_Config_Dialog_Data
{
   int use_shadow;
   int engine;
};

/* Protos */
static void        *_create_data          (E_Config_Dialog *cfd);
static void         _free_data            (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data     (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

E_Config_Dialog *
e_int_config_comp_module(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[4096];
   Mod *mod = _comp_mod;

   if (e_config_dialog_find("E", "appearance/comp")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;

   snprintf(buf, sizeof(buf), "%s/e-module-comp.edj", 
            e_module_dir_get(mod->module));
   cfd = e_config_dialog_new(con,
			     _("Composite Settings"),
			     "E", "appearance/comp",
			     buf, 0, v, mod);
   mod->config_dialog = cfd;
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);

   cfdata->use_shadow = _comp_mod->conf->use_shadow;
   cfdata->engine = _comp_mod->conf->engine;
   if ((cfdata->engine != E_EVAS_ENGINE_SOFTWARE_X11) &&
       (cfdata->engine != E_EVAS_ENGINE_GL_X11))
     cfdata->engine = E_EVAS_ENGINE_SOFTWARE_X11;

   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   _comp_mod->config_dialog = NULL;
   free(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *ob, *of, *ot;
   E_Radio_Group *rg;
   int engine;
   
   o = e_widget_list_add(evas, 0, 0);
   ot = e_widget_table_add(evas, 1);
   
   of = e_widget_framelist_add(evas, _("Shadow"), 0);
   e_widget_framelist_content_align_set(of, 0.5, 0.0);
   ob = e_widget_check_add(evas, _("Enabled"), &(cfdata->use_shadow));
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);
   
   of = e_widget_framelist_add(evas, _("Engine"), 0);
   e_widget_framelist_content_align_set(of, 0.5, 0.0);
   
   rg = e_widget_radio_group_new(&(cfdata->engine));
   
   engine = E_EVAS_ENGINE_SOFTWARE_X11;
   ob = e_widget_radio_add(evas, _("Software"), engine, rg);
   e_widget_framelist_object_append(of, ob);

   if (ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_OPENGL_X11))
     {
        engine = E_EVAS_ENGINE_GL_X11;
        ob = e_widget_radio_add(evas, _("OpenGL"), engine, rg);
        e_widget_framelist_object_append(of, ob);
     }
   
   e_widget_table_object_append(ot, of, 0, 1, 1, 1, 1, 1, 1, 1);
   
   e_widget_list_object_append(o, ot, 1, 1, 0.5);   

   return o;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (_comp_mod->conf->use_shadow != cfdata->use_shadow)
     {
        _comp_mod->conf->use_shadow = cfdata->use_shadow;
        e_mod_comp_shadow_set();
     }
   if (_comp_mod->conf->engine != cfdata->engine)
     {
        _comp_mod->conf->engine = cfdata->engine;
        e_mod_comp_shutdown();
        e_mod_comp_init();
     }
   e_config_save_queue();
   return 1;
}
