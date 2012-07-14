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
             int dx;
          } move;
        struct
          {
             int dx;
          } resize;
     } border_keyboard;
   struct {
      int    move;
      int    resize;
      int    raise;
      int    lower;
      int    layer;
      int    desktop;
      int    iconify;
   } transient;
   int maximize_policy;
   int maximize_direction;
   int maximized_allow_manip;
   int border_fix_on_shelf_toggle;
   
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
   cfdata->border_keyboard.resize.dx = e_config->border_keyboard.resize.dx;
   cfdata->transient.move = e_config->transient.move;
   cfdata->transient.resize = e_config->transient.resize;
   cfdata->transient.raise = e_config->transient.raise;
   cfdata->transient.lower = e_config->transient.lower;
   cfdata->transient.layer = e_config->transient.layer;
   cfdata->transient.desktop = e_config->transient.desktop;
   cfdata->transient.iconify = e_config->transient.iconify;
   cfdata->maximize_policy = (e_config->maximize_policy & E_MAXIMIZE_TYPE);
   if (cfdata->maximize_policy == E_MAXIMIZE_NONE)
     cfdata->maximize_policy = E_MAXIMIZE_FULLSCREEN;
   cfdata->maximize_direction = 
     (e_config->maximize_policy & E_MAXIMIZE_DIRECTION);
   if (!cfdata->maximize_direction)
     cfdata->maximize_direction = E_MAXIMIZE_BOTH;
   cfdata->maximized_allow_manip = e_config->allow_manip;
   cfdata->border_fix_on_shelf_toggle = e_config->border_fix_on_shelf_toggle;
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
   e_config->border_keyboard.move.dy = cfdata->border_keyboard.move.dx;
   e_config->border_keyboard.resize.dx = cfdata->border_keyboard.resize.dx;
   e_config->border_keyboard.resize.dy = cfdata->border_keyboard.resize.dx;
   e_config->transient.move = cfdata->transient.move;
   e_config->transient.resize = cfdata->transient.resize;
   e_config->transient.raise = cfdata->transient.raise;
   e_config->transient.lower = cfdata->transient.lower;
   e_config->transient.layer = cfdata->transient.layer;
   e_config->transient.desktop = cfdata->transient.desktop;
   e_config->transient.iconify = cfdata->transient.iconify;
   e_config->maximize_policy =
     (cfdata->maximize_policy | cfdata->maximize_direction);
   e_config->allow_manip = cfdata->maximized_allow_manip;
   e_config->border_fix_on_shelf_toggle = cfdata->border_fix_on_shelf_toggle;
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
	   (e_config->border_keyboard.resize.dy != cfdata->border_keyboard.resize.dx) ||
           (e_config->transient.move != cfdata->transient.move) ||
           (e_config->transient.resize != cfdata->transient.resize) ||
           (e_config->transient.raise != cfdata->transient.raise) ||
           (e_config->transient.lower != cfdata->transient.lower) ||
           (e_config->transient.layer != cfdata->transient.layer) ||
           (e_config->transient.desktop != cfdata->transient.desktop) ||
           (e_config->transient.iconify != cfdata->transient.iconify) ||
           (e_config->maximize_policy != (cfdata->maximize_policy | cfdata->maximize_direction)) ||
           (e_config->allow_manip != cfdata->maximized_allow_manip) ||
           (e_config->border_fix_on_shelf_toggle != cfdata->border_fix_on_shelf_toggle));
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *otb, *ol, *of, *ow, *oc;
   E_Radio_Group *rg;

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

   /* Maximization */
   ol = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Policy"), 0);
   rg = e_widget_radio_group_new(&(cfdata->maximize_policy));
   ow = e_widget_radio_add(evas, _("Fullscreen"), E_MAXIMIZE_FULLSCREEN, rg);
   e_widget_framelist_object_append(of, ow);
   /* FIXME smart is nothing else than expand - dont confuse users */
   ow = e_widget_radio_add(evas, _("Smart expansion"), E_MAXIMIZE_SMART, rg);
   e_widget_framelist_object_append(of, ow);
   /* ob = e_widget_radio_add(evas, _("Expand the window"), E_MAXIMIZE_EXPAND, rg);
    * e_widget_list_object_append(o, ob, 1, 1, 0.5); */
   ow = e_widget_radio_add(evas, _("Fill available space"), E_MAXIMIZE_FILL, rg);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(ol, of, 1, 0, 0.5);
   of = e_widget_framelist_add(evas, _("Direction"), 0);
   rg = e_widget_radio_group_new(&(cfdata->maximize_direction));
   ow = e_widget_radio_add(evas, _("Horizontal"), E_MAXIMIZE_HORIZONTAL, rg);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_radio_add(evas, _("Vertical"), E_MAXIMIZE_VERTICAL, rg);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_radio_add(evas, _("Both"), E_MAXIMIZE_BOTH, rg);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(ol, of, 1, 0, 0.5);
   ow = e_widget_check_add(evas, _("Allow manipulation of maximized windows"), 
                           &(cfdata->maximized_allow_manip));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Maximization"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Keyboard Move and resize */
   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_label_add(evas, _("Automatically accept changes after:"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.1f s"), 0.0, 9.9, 0.1, 0, 
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
   ow = e_widget_check_add(evas, _("Adjust windows on shelf hide"), 
                           &(cfdata->border_fix_on_shelf_toggle));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Automatic"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Transient  */
   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_check_add(evas, _("Follow Move"),
                           &(cfdata->transient.move));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_check_add(evas, _("Follow Resize"), 
                           &(cfdata->transient.resize));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_check_add(evas, _("Follow Raise"), 
                           &(cfdata->transient.raise));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_check_add(evas, _("Follow Lower"), 
                           &(cfdata->transient.lower));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_check_add(evas, _("Follow Layer"), 
                           &(cfdata->transient.layer));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_check_add(evas, _("Follow Desktop"), 
                           &(cfdata->transient.desktop));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_check_add(evas, _("Follow Iconify"), 
                           &(cfdata->transient.iconify));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   
   e_widget_toolbook_page_append(otb, NULL, _("Transients"), ol,
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
