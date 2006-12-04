/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* FIXME:
 * 
 * basic -
 * * show file
 * * show size
 * * show modified date
 * * show permissions
 * * show preview
 * * show icon
 * * show symlink/fifo/socket/etc. status
 * * show broken link status
 * * change icon for mime type
 * * change icon for just this file
 * * change permissions
 * 
 * advanced (extra) -
 * * change app to open THIS file with
 * * show access date
 * * show change date
 * * show pseudolink status
 * * show pseudolink src
 * * show comment
 * * show generic
 * * show mount status
 * * show link destination (if symlink or link)
 * * change link destination
 * 
 */

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
   E_Fm2_Icon_Info *fi;
   /*- BASIC -*/
   /*- ADVANCED -*/
};

/* a nice easy setup function that does the dirty work */
EAPI E_Config_Dialog *
e_fm_prop_file(E_Container *con, E_Fm2_Icon_Info *fi)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
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
			     _("File Properties"),
			     "E", "_fm_prop",
			     "enlightenment/file_properties", 0, v, fi);
   return cfd;
}

/**--CREATE--**/
static void
_fill_data(E_Config_Dialog_Data *cfdata, E_Fm2_Icon_Info *fi)
{
   cfdata->fi = fi;
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
   _fill_data(cfdata, cfd->data);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   /* Free the cfdata */
   free(cfdata);
}

/**--APPLY--**/
static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   return 1; /* Apply was OK */
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   return 1; /* Apply was OK */
}

/**--GUI--**/
static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for a basic dialog */
   Evas_Object *o, *ot, *ob;
   char buf[4096];
   
   snprintf(buf, sizeof(buf), "%s/%s", 
	    e_fm2_real_path_get(cfdata->fi->fm), cfdata->fi->file);
   o = e_widget_table_add(evas, 0);

   ot = e_widget_table_add(evas, 0);
   
   ob = e_widget_label_add(evas, _("File:"));
   e_widget_table_object_append(ot, ob, 0, 0, 1, 1, 1, 0, 1, 0);
   ob = e_widget_entry_add(evas, NULL);
   e_widget_min_size_set(ob, 80, -1);
   e_widget_entry_readonly_set(ob, 1);
   e_widget_table_object_append(ot, ob, 1, 0, 1, 1, 1, 0, 1, 0);
   
   ob = e_widget_label_add(evas, _("Size:"));
   e_widget_table_object_append(ot, ob, 0, 1, 1, 1, 1, 0, 1, 0);
   ob = e_widget_entry_add(evas, NULL);
   e_widget_min_size_set(ob, 80, -1);
   e_widget_entry_readonly_set(ob, 1);
   e_widget_table_object_append(ot, ob, 1, 1, 1, 1, 1, 0, 1, 0);

   e_widget_table_object_append(o, ot, 0, 0, 1, 1, 1, 1, 1, 1);
   
   ot = e_widget_table_add(evas, 0);

   ob = e_widget_preview_add(evas, 128, 128);
   e_widget_table_object_append(ot, ob, 0, 0, 1, 1, 1, 1, 1, 1);
   e_widget_preview_thumb_set(ob, buf,
			      "e/desktop/background", 128, 128);
   
   e_widget_table_object_append(o, ot, 1, 0, 1, 1, 1, 1, 1, 1);
   
/*   
   o = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->mode));
   ob = e_widget_radio_add(evas, _("Click Window to Focus"), E_FOCUS_CLICK, rg);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_radio_add(evas, _("Window under the Mouse"), E_FOCUS_MOUSE, rg);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_radio_add(evas, _("Most recent Window under the Mouse"), E_FOCUS_SLOPPY, rg);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
 */
   return o;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   /* generate the core widget layout for an advanced dialog */
   Evas_Object *o;
      
   o = e_widget_table_add(evas, 0);
/*   
   o = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("Focus"), 0);
   rg = e_widget_radio_group_new(&(cfdata->focus_policy));
   ob = e_widget_radio_add(evas, _("Click to focus"), E_FOCUS_CLICK, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Pointer focus"), E_FOCUS_MOUSE, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Sloppy focus"), E_FOCUS_SLOPPY, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("New Window Focus"), 0);
   rg = e_widget_radio_group_new(&(cfdata->focus_setting));
   ob = e_widget_radio_add(evas, _("No new windows get focus"), E_FOCUS_NONE, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("All new windows get focus"), E_FOCUS_NEW_WINDOW, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Only new dialogs get focus"), E_FOCUS_NEW_DIALOG, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Only new dialogs get focus if the parent has focus"), E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Other Settings"), 0);
   ob = e_widget_check_add(evas, _("Always pass on caught click events to programs"), &(cfdata->pass_click_on));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("A click on a window always raises it"), &(cfdata->always_click_to_raise));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("A click in a window always focuses it"), &(cfdata->always_click_to_focus));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Refocus last focused window on desktop switch"), &(cfdata->focus_last_focused_per_desktop));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Revert focus when hiding or closing a window"), &(cfdata->focus_revert_on_hide_or_close));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
*/   
   return o;
}
