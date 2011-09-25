#include "e.h"

/* PROTOTYPES - same all the time */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _cb_disable_check_list(void *data, Evas_Object *obj);

/* Actual config data we will be playing with whil the dialog is active */
struct _E_Config_Dialog_Data
{
   int use_resist;
   int desk_resist;
   int window_resist;
   int gadget_resist;
   int geometry_auto_resize_limit;
   int geometry_auto_move;
   struct
     {
        double timeout;
        struct
          {
             int dx, dy;
          } move;
        struct
          {
             int dx, dy;
          } resize;
     } border_keyboard;

   Eina_List *resistance_list;
};

E_Config_Dialog *
e_int_config_window_geometry(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "windows/window_geometry")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   /* methods */
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;
   v->basic.check_changed = _basic_check_changed;

   /* create config diaolg for NULL object/data */
   cfd = e_config_dialog_new(con, _("Window Geometry"),
			     "E", "windows/window_geometry",
			     "preferences-window-geometry", 0, v, NULL);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;
   cfdata->use_resist = e_config->use_resist;
   cfdata->desk_resist = e_config->desk_resist;
   cfdata->window_resist = e_config->window_resist;
   cfdata->gadget_resist = e_config->gadget_resist;
   cfdata->geometry_auto_resize_limit = e_config->geometry_auto_resize_limit;
   cfdata->geometry_auto_move = e_config->geometry_auto_move;
   cfdata->border_keyboard.timeout = e_config->border_keyboard.timeout;
   cfdata->border_keyboard.move.dx = e_config->border_keyboard.move.dx;
//   cfdata->border_keyboard.move.dy = e_config->border_keyboard.move.dy;
   cfdata->border_keyboard.resize.dx = e_config->border_keyboard.resize.dx;
//   cfdata->border_keyboard.resize.dy = e_config->border_keyboard.resize.dy;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   eina_list_free(cfdata->resistance_list);
   E_FREE(cfdata);
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->use_resist = cfdata->use_resist;
   e_config->desk_resist = cfdata->desk_resist;
   e_config->window_resist = cfdata->window_resist;
   e_config->gadget_resist = cfdata->gadget_resist;
   e_config->geometry_auto_resize_limit = cfdata->geometry_auto_resize_limit;
   e_config->geometry_auto_move = cfdata->geometry_auto_move;  
   e_config->border_keyboard.timeout = cfdata->border_keyboard.timeout;
   e_config->border_keyboard.move.dx = cfdata->border_keyboard.move.dx;
//   e_config->border_keyboard.move.dy = cfdata->border_keyboard.move.dy;
   e_config->border_keyboard.move.dy = cfdata->border_keyboard.move.dx;
   e_config->border_keyboard.resize.dx = cfdata->border_keyboard.resize.dx;
//   e_config->border_keyboard.resize.dy = cfdata->border_keyboard.resize.dy;
   e_config->border_keyboard.resize.dy = cfdata->border_keyboard.resize.dx;
   e_config_save_queue();
   return 1; /* Apply was OK */
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return ((e_config->use_resist != cfdata->use_resist) ||
	   (e_config->desk_resist != cfdata->desk_resist) ||
	   (e_config->window_resist != cfdata->window_resist) ||
	   (e_config->gadget_resist != cfdata->gadget_resist) ||
	   (e_config->geometry_auto_resize_limit != cfdata->geometry_auto_resize_limit) ||
           (e_config->geometry_auto_move != cfdata->geometry_auto_move) ||
	   (e_config->border_keyboard.timeout != cfdata->border_keyboard.timeout) ||
	   (e_config->border_keyboard.move.dx != cfdata->border_keyboard.move.dx) ||
	   (e_config->border_keyboard.move.dy != cfdata->border_keyboard.move.dx) ||
	   (e_config->border_keyboard.resize.dx != cfdata->border_keyboard.resize.dx) ||
	   (e_config->border_keyboard.resize.dy != cfdata->border_keyboard.resize.dx));
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *otb, *ol, *ow, *oc;

   otb = e_widget_toolbook_add(evas, (24 * e_scale), (24 * e_scale));

   /* Resistance */
   ol = e_widget_list_add(evas, 0, 0);
   oc = e_widget_check_add(evas, _("Resist obstacles"), &(cfdata->use_resist));
   e_widget_list_object_append(ol, oc, 1, 0, 0.5);
   ow = e_widget_label_add(evas, _("Other windows"));
   e_widget_disabled_set(ow, !cfdata->use_resist);
   cfdata->resistance_list = eina_list_append (cfdata->resistance_list, ow);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 0, 64.0, 1.0, 0, 
                            NULL, &(cfdata->window_resist), 100);
   cfdata->resistance_list = eina_list_append (cfdata->resistance_list, ow);
   e_widget_disabled_set(ow, !cfdata->use_resist);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_label_add(evas, _("Edge of the screen"));
   cfdata->resistance_list = eina_list_append (cfdata->resistance_list, ow);
   e_widget_disabled_set(ow, !cfdata->use_resist);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 0, 64.0, 1.0, 0, 
                            NULL, &(cfdata->desk_resist), 100);
   cfdata->resistance_list = eina_list_append (cfdata->resistance_list, ow);
   e_widget_disabled_set(ow, !cfdata->use_resist);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_label_add(evas, _("Desktop gadgets"));
   cfdata->resistance_list = eina_list_append (cfdata->resistance_list, ow);
   e_widget_disabled_set(ow, !cfdata->use_resist);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 0, 64.0, 1.0, 0, 
                            NULL, &(cfdata->gadget_resist), 100);
   cfdata->resistance_list = eina_list_append (cfdata->resistance_list, ow);
   e_widget_disabled_set(ow, !cfdata->use_resist);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_on_change_hook_set(oc, _cb_disable_check_list, 
                               cfdata->resistance_list);
   e_widget_toolbook_page_append(otb, NULL, _("Resistance"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Keyboard Move and resize */
   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_label_add(evas, _("Automatically accept changes after:"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.1f sec"), 0.0, 9.9, 0.1, 0, 
                            &(cfdata->border_keyboard.timeout), NULL, 100);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_label_add(evas, _("Move by"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 1, 255, 1, 0, NULL, 
                            &(cfdata->border_keyboard.move.dx), 100);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_label_add(evas, _("Resize by"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 1, 255, 1, 0, NULL, 
                            &(cfdata->border_keyboard.resize.dx), 100);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Keyboard"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);


   /* Automatic Move and resize */
   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_check_add(evas, _("Limit resize to useful geometry"), 
                           &(cfdata->geometry_auto_resize_limit));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_check_add(evas, _("Move after resize"), 
                           &(cfdata->geometry_auto_move));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Automatic"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);
   return otb;
}

static void
_cb_disable_check_list(void *data, Evas_Object *obj)
{
   const Eina_List *list = data;
   const Eina_List *l;
   Evas_Object *o;
   Eina_Bool disable = !e_widget_check_checked_get(obj);

   EINA_LIST_FOREACH(list, l, o)
     e_widget_disabled_set(o, disable);
}
