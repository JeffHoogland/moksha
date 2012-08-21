#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_physics.h"

struct _E_Config_Dialog_Data
{
   double delay; /* delay before applying physics */
   double max_mass; /* maximum mass for a window */
   double gravity; /* maximum mass for a window */
   Eina_Bool ignore_fullscreen;
   Eina_Bool ignore_maximized;
   Eina_Bool ignore_shelves;
   struct
   {
      Eina_Bool disable_rotate;
      Eina_Bool disable_move;
   } shelf;
};

/* Protos */
static int _basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

E_Config_Dialog *
e_int_config_physics_module(E_Container *con,
                         const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[4096];
   Mod *mod = _physics_mod;

   if (mod->config_dialog) return NULL;
   snprintf(buf, sizeof(buf), "%s/e-module-physics.edj", e_module_dir_get(mod->module));
   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->basic.check_changed = _basic_check_changed;

   cfd = e_config_dialog_new(con, _("Physics Settings"),
                             "E", "appearance/physics", buf, 32, v, mod);
   mod->config_dialog = cfd;
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);

   cfdata->delay = _physics_mod->conf->delay;
   cfdata->max_mass = _physics_mod->conf->max_mass;

   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd  __UNUSED__,
           E_Config_Dialog_Data *cfdata)
{
   _physics_mod->config_dialog = NULL;
   free(cfdata);
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
#define CHECK(X) \
   if (cfdata->X != _physics_mod->conf->X) return 1

   CHECK(max_mass);
   CHECK(gravity);
   CHECK(ignore_fullscreen);
   CHECK(ignore_maximized);
   CHECK(ignore_shelves);
   CHECK(shelf.disable_move);
   CHECK(shelf.disable_rotate);
   if ((unsigned int)cfdata->delay != _physics_mod->conf->delay) return 1;

#undef CHECK
   return 0;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd,
                      Evas *evas,
                      E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ob, *ol, *otb, *tab;

   tab = e_widget_table_add(evas, 0);
   evas_object_name_set(tab, "dia_table");

   otb = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);

   ///////////////////////////////////////////

   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_label_add(evas, _("Physics delay after drag"));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%2.0f Frames"), 5, 20, 1, 0, &(cfdata->delay), NULL, 150);
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_label_add(evas, _("Maximum window mass"));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%2.1f Kg"), 1, 6, 1, 0, &(cfdata->max_mass), NULL, 150);
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_label_add(evas, _("Desktop gravity"));
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f m/s^2"), -5, 5, 1, 0, &(cfdata->gravity), NULL, 150);
   e_widget_list_object_append(ol, ob, 1, 1, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("General"), ol, 1, 1, 1, 1, 0.5, 0.5);

   ///////////////////////////////////////////

   ol = e_widget_list_add(evas, 0, 0);

   ob = e_widget_check_add(evas, _("Disable Movement"), (int*)&(cfdata->shelf.disable_move));
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   ob = e_widget_check_add(evas, _("Disable Rotation"), (int*)&(cfdata->shelf.disable_rotate));
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Shelves"), ol, 1, 1, 1, 1, 0.5, 0.5);

   ///////////////////////////////////////////

   ol = e_widget_list_add(evas, 0, 0);
   ob = e_widget_check_add(evas, _("Ignore Fullscreen"), (int*)&(cfdata->ignore_fullscreen));
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   ob = e_widget_check_add(evas, _("Ignore Maximized"), (int*)&(cfdata->ignore_maximized));
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   ob = e_widget_check_add(evas, _("Ignore Shelves"), (int*)&(cfdata->ignore_shelves));
   e_widget_list_object_append(ol, ob, 1, 0, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Ignore"), ol, 1, 1, 1, 1, 0.5, 0.5);

   e_widget_toolbook_page_show(otb, 0);

   e_dialog_resizable_set(cfd->dia, 1);

   e_widget_table_object_append(tab, otb, 0, 0, 1, 1, 1, 1, 1, 1);
   return tab;
}


static int
_basic_apply_data(E_Config_Dialog *cfd  __UNUSED__,
                  E_Config_Dialog_Data *cfdata)
{
   if ((cfdata->delay != _physics_mod->conf->delay) ||
       (cfdata->max_mass != _physics_mod->conf->max_mass) ||
       (cfdata->ignore_fullscreen != _physics_mod->conf->ignore_fullscreen) ||
       (cfdata->ignore_maximized != _physics_mod->conf->ignore_maximized) ||
       (cfdata->gravity != _physics_mod->conf->gravity)
      )
     {
#define SET(X) _physics_mod->conf->X = cfdata->X
        _physics_mod->conf->delay = (unsigned int)cfdata->delay;
        SET(max_mass);
        SET(ignore_fullscreen);
        SET(ignore_maximized);
        SET(ignore_shelves);
        SET(gravity);
        SET(shelf.disable_move);
        SET(shelf.disable_rotate);
        e_mod_physics_shutdown();
        e_mod_physics_init();
     }
   e_config_save_queue();
   return 1;
}

