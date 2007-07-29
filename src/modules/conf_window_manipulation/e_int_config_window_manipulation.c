/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* PROTOTYPES - same all the time */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

/* Actual config data we will be playing with whil the dialog is active */
struct _E_Config_Dialog_Data
{
   /*- BASIC -*/
   int use_auto_raise;
   int use_resist;
   int maximize_policy;
   /*- ADVANCED -*/
   int maximize_direction;
   double auto_raise_delay;
   int desk_resist;
   int window_resist;
   int gadget_resist;
   int allow_manip;
   int border_fix_on_shelf_toggle;
   int border_raise_on_mouse_action;
   int border_raise_on_focus;
   int allow_above_fullscreen; 
};

/* a nice easy setup function that does the dirty work */
EAPI E_Config_Dialog *
e_int_config_window_manipulation(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_window_manipulation_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   /* methods */
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.apply_cfdata      = _basic_apply_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->advanced.apply_cfdata   = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   /* create config diaolg for NULL object/data */
   cfd = e_config_dialog_new(con,
			     _("Window Manipulation"),
			     "E", "_config_window_manipulation_dialog",
			     "enlightenment/window_manipulation", 0, v, NULL);
   return cfd;
}

/**--CREATE--**/
static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->use_auto_raise = e_config->use_auto_raise;
   cfdata->auto_raise_delay = e_config->auto_raise_delay;
   cfdata->use_resist = e_config->use_resist;
   cfdata->desk_resist = e_config->desk_resist;
   cfdata->window_resist = e_config->window_resist;
   cfdata->gadget_resist = e_config->gadget_resist;
   cfdata->maximize_policy = (e_config->maximize_policy & E_MAXIMIZE_TYPE);
   if (cfdata->maximize_policy == E_MAXIMIZE_NONE)
     cfdata->maximize_policy = E_MAXIMIZE_FULLSCREEN;
   cfdata->maximize_direction = (e_config->maximize_policy & E_MAXIMIZE_DIRECTION);
   if (!cfdata->maximize_direction)
     cfdata->maximize_direction = E_MAXIMIZE_BOTH;
   cfdata->allow_manip = e_config->allow_manip;
   cfdata->border_fix_on_shelf_toggle = e_config->border_fix_on_shelf_toggle;
   cfdata->border_raise_on_mouse_action = e_config->border_raise_on_mouse_action;
   cfdata->border_raise_on_focus = e_config->border_raise_on_focus;
   cfdata->allow_above_fullscreen = e_config->allow_above_fullscreen;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   /* Create cfdata - cfdata is a temporary block of config data that this
    * dialog will be dealing with while configuring. it will be applied to
    * the running systems/config in the apply methods
    */
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Free the cfdata */
   E_FREE(cfdata);
}

/**--APPLY--**/
static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Actually take our cfdata settings and apply them in real life */
   e_config->use_auto_raise = cfdata->use_auto_raise;
   e_config->use_resist = cfdata->use_resist;
   e_config->maximize_policy = cfdata->maximize_policy | cfdata->maximize_direction;
   e_config_save_queue();
   return 1; /* Apply was OK */
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Actually take our cfdata settings and apply them in real life */
   e_config->use_auto_raise = cfdata->use_auto_raise;
   e_config->auto_raise_delay = cfdata->auto_raise_delay;
   e_config->use_resist = cfdata->use_resist;
   e_config->desk_resist = cfdata->desk_resist;
   e_config->window_resist = cfdata->window_resist;
   e_config->gadget_resist = cfdata->gadget_resist;
   e_config->maximize_policy = cfdata->maximize_policy | cfdata->maximize_direction;
   e_config->allow_manip = cfdata->allow_manip;
   e_config->border_fix_on_shelf_toggle = cfdata->border_fix_on_shelf_toggle;
   e_config->border_raise_on_mouse_action = cfdata->border_raise_on_mouse_action;
   e_config->border_raise_on_focus = cfdata->border_raise_on_focus;
   e_config->allow_above_fullscreen = cfdata->allow_above_fullscreen;
   e_config_save_queue();
   return 1; /* Apply was OK */
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   
   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("Miscellaneous Options"), 0);
   ob = e_widget_check_add(evas, _("Automatically raise windows on mouse over"), &(cfdata->use_auto_raise));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("When moving or resizing windows, resist at the boundaries"), &(cfdata->use_resist));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Maximize Policy"), 0);
   rg = e_widget_radio_group_new(&(cfdata->maximize_policy));
   ob = e_widget_radio_add(evas, _("Fullscreen"), E_MAXIMIZE_FULLSCREEN, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Smart expansion"), E_MAXIMIZE_SMART, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Expand the window"), E_MAXIMIZE_EXPAND, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Fill available space"), E_MAXIMIZE_FILL, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   return o;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for an advanced dialog */
   Evas_Object *o, *ob, *of, *ot;
   E_Radio_Group *rg;
   
   o = e_widget_list_add(evas, 0, 0);
   ot = e_widget_table_add(evas, 0);
   
   of = e_widget_framelist_add(evas, _("Autoraise"), 0);
   ob = e_widget_check_add(evas, _("Automatically raise windows on mouse over"), &(cfdata->use_auto_raise));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Delay before raising:"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f sec"), 0.0, 9.9, 0.1, 0, &(cfdata->auto_raise_delay), NULL, 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 0, 1, 1, 1, 1, 1, 1);

   of = e_widget_framelist_add(evas, _("Raise Window"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   ob = e_widget_check_add(evas, _("Raise when starting to move or resize"), &(cfdata->border_raise_on_mouse_action));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Raise when clicking to focus"), &(cfdata->border_raise_on_focus));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Allow windows to be above fullscreen window"), &(cfdata->allow_above_fullscreen));
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);
   
   of = e_widget_framelist_add(evas, _("Resistance"), 0);
   ob = e_widget_check_add(evas, _("Resist moving or resizing a window over an obstacle"), &(cfdata->use_resist));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Resistance between windows:"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 0, 64.0, 1.0, 0, NULL, &(cfdata->window_resist), 200);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Resistance at the edge of the screen:"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 0, 64.0, 1.0, 0, NULL, &(cfdata->desk_resist), 200);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_label_add(evas, _("Resistance to desktop gadgets:"));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 0, 64.0, 1.0, 0, NULL, &(cfdata->gadget_resist), 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 0, 1, 1, 1, 1, 1, 1, 1);
 
   of = e_widget_framelist_add(evas, _("Maximize Policy"), 0);
   rg = e_widget_radio_group_new(&(cfdata->maximize_policy));
   ob = e_widget_radio_add(evas, _("Fullscreen"), E_MAXIMIZE_FULLSCREEN, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Smart expansion"), E_MAXIMIZE_SMART, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Expand the window"), E_MAXIMIZE_EXPAND, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Fill available space"), E_MAXIMIZE_FILL, rg);
   e_widget_framelist_object_append(of, ob);

   rg = e_widget_radio_group_new(&(cfdata->maximize_direction));
   ob = e_widget_radio_add(evas, _("Both directions"), E_MAXIMIZE_BOTH, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Horizontal"), E_MAXIMIZE_HORIZONTAL, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Vertical"), E_MAXIMIZE_VERTICAL, rg);
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_check_add(evas, _("Allow window manipulation"), &(cfdata->allow_manip));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Automatically move/resize windows on shelf autohide"), &(cfdata->border_fix_on_shelf_toggle));
   e_widget_framelist_object_append(of, ob);
   e_widget_table_object_append(ot, of, 1, 1, 1, 1, 1, 1, 1, 1);
   
   e_widget_list_object_append(o, ot, 1, 1, 0.5);
   
   return o;
}
