#include "e.h"

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _advanced_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _advanced_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

/* Actual config data we will be playing with whil the dialog is active */
struct _E_Config_Dialog_Data
{
   /*- BASIC -*/
   int maximize_policy;
   /*- ADVANCED -*/
   int maximize_direction;
   int allow_manip;
   int border_fix_on_shelf_toggle;
};

E_Config_Dialog *
e_int_config_window_maxpolicy(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "windows/window_maxpolicy_dialog")) 
     return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);

   /* methods */
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;
   v->basic.check_changed = _basic_check_changed;
   v->advanced.apply_cfdata = _advanced_apply;
   v->advanced.create_widgets = _advanced_create;
   v->advanced.check_changed = _advanced_check_changed;

   /* create config diaolg for NULL object/data */
   cfd = e_config_dialog_new(con, _("Window Maximize Policy"),
			     "E", "windows/window_maxpolicy_dialog",
			     "preferences-window-manipulation", 0, v, NULL);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   if (!cfdata) return NULL;

   cfdata->maximize_policy = (e_config->maximize_policy & E_MAXIMIZE_TYPE);
   if (cfdata->maximize_policy == E_MAXIMIZE_NONE)
     cfdata->maximize_policy = E_MAXIMIZE_FULLSCREEN;
   cfdata->maximize_direction = 
     (e_config->maximize_policy & E_MAXIMIZE_DIRECTION);
   if (!cfdata->maximize_direction)
     cfdata->maximize_direction = E_MAXIMIZE_BOTH;
   cfdata->allow_manip = e_config->allow_manip;
   cfdata->border_fix_on_shelf_toggle = e_config->border_fix_on_shelf_toggle;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata);
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->maximize_policy =
     (cfdata->maximize_policy | cfdata->maximize_direction);
   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return (e_config->maximize_policy !=
	   (cfdata->maximize_policy | cfdata->maximize_direction));
}

static int
_advanced_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->maximize_policy =
     (cfdata->maximize_policy | cfdata->maximize_direction);
   e_config->allow_manip = cfdata->allow_manip;
   e_config->border_fix_on_shelf_toggle = cfdata->border_fix_on_shelf_toggle;
   e_config_save_queue();
   return 1;
}

static int
_advanced_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return ((e_config->maximize_policy != (cfdata->maximize_policy | cfdata->maximize_direction)) ||
	   (e_config->allow_manip != cfdata->allow_manip) ||
	   (e_config->border_fix_on_shelf_toggle != cfdata->border_fix_on_shelf_toggle));
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ob, *of;
   E_Radio_Group *rg;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Maximize Policy"), 0);
   rg = e_widget_radio_group_new(&(cfdata->maximize_policy));
   ob = e_widget_radio_add(evas, _("Fullscreen"), E_MAXIMIZE_FULLSCREEN, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Smart expansion"), E_MAXIMIZE_SMART, rg);
   e_widget_framelist_object_append(of, ob);
   /* ob = e_widget_radio_add(evas, _("Expand the window"), E_MAXIMIZE_EXPAND, rg);
    * e_widget_list_object_append(o, ob, 1, 1, 0.5); */
   ob = e_widget_radio_add(evas, _("Fill available space"), E_MAXIMIZE_FILL, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 0, 0.5);

   return o;
}

static Evas_Object *
_advanced_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *otb, *ol, *ow;
   E_Radio_Group *rg;

   otb = e_widget_toolbook_add(evas, (24 * e_scale), (24 * e_scale));

   /* Policy */
   ol = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->maximize_policy));
   ow = e_widget_radio_add(evas, _("Fullscreen"), E_MAXIMIZE_FULLSCREEN, rg);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   /* FIXME smart is nothing else than expand - dont confuse users */
   ow = e_widget_radio_add(evas, _("Smart expansion"), E_MAXIMIZE_SMART, rg);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   /* ow = e_widget_radio_add(evas, _("Expand the window"), E_MAXIMIZE_EXPAND, rg);
    e_widget_list_object_append(ol, ow, 1, 0, 0.5); */
   ow = e_widget_radio_add(evas, _("Fill available space"), 
                           E_MAXIMIZE_FILL, rg);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Policy"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Directions */
   ol = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->maximize_direction));
   ow = e_widget_radio_add(evas, _("Horizontal"), E_MAXIMIZE_HORIZONTAL, rg);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_radio_add(evas, _("Vertical"), E_MAXIMIZE_VERTICAL, rg);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_radio_add(evas, _("Both"), E_MAXIMIZE_BOTH, rg);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Direction"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   /* Misc */
   ol = e_widget_list_add(evas, 0, 0);
   /* FIXME this should be default imho. no big deal if one resizes
      a maximized window by mistake and then it's not maximized
      anymore.. people will rather wonder why they can't shade
      their window (hannes)

      k-s: often this also mean disable such border decoration, so makes sense.
           I'd say it makes no sense to move/resize maximized windows :-)
   */
   ow = e_widget_check_add(evas, _("Allow manipulation of maximized windows"), 
                           &(cfdata->allow_manip));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_check_add(evas, _("Adjust windows on shelf hide"), 
                           &(cfdata->border_fix_on_shelf_toggle));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Miscellaneous"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);
   return otb;
}
