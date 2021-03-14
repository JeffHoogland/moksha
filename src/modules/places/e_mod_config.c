#include <e.h>
#include "config.h"
#include "e_mod_main.h"
#include "e_mod_places.h"

struct _E_Config_Dialog_Data
{
   int auto_mount;
   int boot_mount;
   int auto_open;
   char *fm;
   int fm_chk;
   Evas_Object *entry;
   int show_menu;
   int hide_header;
   int show_home;
   int show_desk;
   int show_trash;
   int show_root;
   int show_temp;
   int show_bookm;
   
   int show_alert;
   int alert_percent;
   int dismiss_alert;
   int alert_timeout;
    struct
   {
      Evas_Object *show_alert_label;
      Evas_Object *show_alert_percent;
      Evas_Object *dismiss_alert_label;
      Evas_Object *alert_timeout;
   } ui;
};

/* Local Function Prototypes */
static void *_create_data(E_Config_Dialog *cfd __UNUSED__);
static void _free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);

/* External Functions */
E_Config_Dialog *
e_int_config_places_module(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd = NULL;
   E_Config_Dialog_View *v = NULL;
   //~ char buf[PATH_MAX];

   /* is this config dialog already visible ? */
   if (e_config_dialog_find("Places", "fileman/places")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   /* Icon in the theme */
   //~ snprintf(buf, sizeof(buf), "%s/e-module-places.edj", places_conf->module->dir);

   /* create new config dialog */
   cfd = e_config_dialog_new(con, _("Places Settings"), "Places",
                             "fileman/places", "folder-open", 0, v, NULL);
   places_conf->cfd = cfd;
   return cfd;
}

/* Local Functions */
static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = NULL;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   free(cfdata->fm);
   places_conf->cfd = NULL;
   E_FREE(cfdata);
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   /* load a temp copy of the config variables */
   cfdata->auto_mount = places_conf->auto_mount;
   cfdata->boot_mount = places_conf->boot_mount;
   cfdata->auto_open = places_conf->auto_open;

   cfdata->show_menu = places_conf->show_menu;
   cfdata->hide_header = places_conf->hide_header;
   cfdata->show_home = places_conf->show_home;
   cfdata->show_desk = places_conf->show_desk;
   cfdata->show_trash = places_conf->show_trash;
   cfdata->show_root = places_conf->show_root;
   cfdata->show_temp = places_conf->show_temp;
   cfdata->show_bookm = places_conf->show_bookm;
   cfdata->alert_percent = places_conf->alert_p;
   cfdata->alert_timeout = places_conf->alert_timeout;
   
   if (cfdata->alert_percent > 0)
     cfdata->show_alert = 1;
   else
     cfdata->show_alert = 0;
    
    if (cfdata->alert_timeout > 0)
     cfdata->dismiss_alert = 1;
   else
     cfdata->dismiss_alert = 0;
   
   if (places_conf->fm)
     cfdata->fm = strdup(places_conf->fm);
   else
     cfdata->fm = strdup("");
}

static void
_ensure_alert_time(E_Config_Dialog_Data *cfdata)
{
   if (cfdata->alert_percent > 0)
     return;
}

void _custom_fm_click(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata = data;

   if (e_widget_check_checked_get(obj))
     e_widget_disabled_set(cfdata->entry, 0);
   else
     {
        e_widget_disabled_set(cfdata->entry, 1);
        e_widget_entry_text_set(cfdata->entry, "");
     }
}

void _mount_on_insert_click(void *data, Evas_Object *obj)
{
   Evas_Object *ow = data;

   if (e_widget_check_checked_get(obj))
     e_widget_disabled_set(ow, 0);
   else
     {
        e_widget_check_checked_set(ow, 0);
        e_widget_disabled_set(ow, 1);
     }
}

static void
_cb_show_alert_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   Eina_Bool show_alert = cfdata->show_alert;
   Eina_Bool dismiss_alert = cfdata->show_alert && cfdata->dismiss_alert;
   e_widget_disabled_set(cfdata->ui.show_alert_label, !show_alert);
   e_widget_disabled_set(cfdata->ui.show_alert_percent, !show_alert);
   e_widget_disabled_set(cfdata->ui.dismiss_alert_label, !show_alert);

   e_widget_disabled_set(cfdata->ui.alert_timeout, !dismiss_alert);
}

static void
_cb_dismiss_alert_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   Eina_Bool dismiss_alert = cfdata->show_alert && cfdata->dismiss_alert;

   e_widget_disabled_set(cfdata->ui.alert_timeout, !dismiss_alert);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o = NULL, *of = NULL, *ow = NULL, *ow1 = NULL, *otb = NULL;
   otb = e_widget_toolbook_add(evas, (48 * e_scale), (48 * e_scale));

   o = e_widget_list_add(evas, 0, 0);

   //General frame
   of = e_widget_framelist_add(evas, _("General"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);

   ow = e_widget_check_add(evas, _("Show in main menu"),
                           &(cfdata->show_menu));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, _("Hide the gadget header"),
                           &(cfdata->hide_header));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, _("Mount volumes at boot"),
                           &(cfdata->boot_mount));
   e_widget_framelist_object_append(of, ow);

   ow1 = e_widget_check_add(evas, _("Mount volumes on insert"),
                           &(cfdata->auto_mount));
   e_widget_framelist_object_append(of, ow1);

   ow = e_widget_check_add(evas, _("Open filemanager on insert"),
                           &(cfdata->auto_open));
   e_widget_framelist_object_append(of, ow);
   e_widget_on_change_hook_set(ow1, _mount_on_insert_click, ow);
   if (!cfdata->auto_mount)
     e_widget_disabled_set(ow, 1);

   ow = e_widget_check_add(evas, _("Use a custom file manager"), &(cfdata->fm_chk));
   e_widget_check_checked_set(ow, strlen(cfdata->fm) > 0 ? 1 : 0);
   e_widget_on_change_hook_set(ow, _custom_fm_click, cfdata);
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_entry_add(evas, &(cfdata->fm), NULL, NULL, NULL);
   e_widget_disabled_set(ow, strlen(cfdata->fm) > 0 ? 0 : 1);
   cfdata->entry = ow;
   e_widget_framelist_object_append(of, ow);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   //Display frame
   of = e_widget_framelist_add(evas, _("Show in menu"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);

   ow = e_widget_check_add(evas, _("Home"), &(cfdata->show_home));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, _("Desktop"), &(cfdata->show_desk));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, _("Trash"), &(cfdata->show_trash));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, _("Filesystem"), &(cfdata->show_root));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, _("Temp"), &(cfdata->show_temp));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, _("Favorites"), &(cfdata->show_bookm));
   e_widget_framelist_object_append(of, ow);

   e_widget_list_object_append(o, of, 1, 1, 0.5);
  
   e_widget_toolbook_page_append(otb, NULL, _("Places"), o, 1, 0, 1, 0,
                                 0.5, 0.0);
                                 
  //second toolbook page
   o = e_widget_list_add(evas, 0, 0);

   
   of = e_widget_framelist_add(evas, _("Alert"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);

   ow = e_widget_check_add(evas, _("Show full disk alert"),
                           &(cfdata->show_alert));
   e_widget_on_change_hook_set(ow, _cb_show_alert_changed, cfdata);                           
   e_widget_framelist_object_append(of, ow);    
   ow = e_widget_label_add(evas, _("Capacity limit:"));
   e_widget_framelist_object_append(of, ow);  
   cfdata->ui.show_alert_label = ow;
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f %%"), 0, 100, 1, 0,
                            NULL, &(cfdata->alert_percent), 100);
   cfdata->ui.show_alert_percent = ow;
   e_widget_framelist_object_append(of, ow);   
   ow = e_widget_check_add(evas, _("Auto dismiss in..."),
                           &(cfdata->dismiss_alert));
   cfdata->ui.dismiss_alert_label = ow;
   e_widget_on_change_hook_set(ow, _cb_dismiss_alert_changed, cfdata);
   e_widget_framelist_object_append(of, ow);   
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f s"), 1, 300, 1, 0, 
                            NULL, &(cfdata->alert_timeout), 100);
   cfdata->ui.alert_timeout = ow;
   e_widget_framelist_object_append(of, ow);                           
   e_widget_list_object_append(o, of, 1, 1, 0.5);    
   
   _cb_show_alert_changed(cfdata, NULL);                     
                                 
   e_widget_toolbook_page_append(otb, NULL, _("Alert"), o, 1, 0, 1, 0,
                                 0.5, 0.0);                                 
                                 
   e_widget_toolbook_page_show(otb, 0);
   return otb;
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   places_conf->show_menu = cfdata->show_menu;
   places_conf->hide_header = cfdata->hide_header;
   places_conf->auto_mount = cfdata->auto_mount;
   places_conf->boot_mount = cfdata->boot_mount;
   places_conf->auto_open = cfdata->auto_open;
   places_conf->show_home = cfdata->show_home;
   places_conf->show_desk = cfdata->show_desk;
   places_conf->show_trash = cfdata->show_trash;
   places_conf->show_root = cfdata->show_root;
   places_conf->show_temp = cfdata->show_temp;
   places_conf->show_bookm = cfdata->show_bookm;
   
    if (cfdata->show_alert)
     {
        _ensure_alert_time(cfdata);
        places_conf->alert_p = cfdata->alert_percent;
     }
   else
        places_conf->alert_p = 0;
   
    if ((cfdata->dismiss_alert) && (cfdata->alert_timeout > 0))
     places_conf->alert_timeout = cfdata->alert_timeout;
   else
     places_conf->alert_timeout = 0;
   
   const char *fm = eina_stringshare_add(cfdata->fm);
   eina_stringshare_del(places_conf->fm);
   places_conf->fm = fm;

   e_config_save_queue();
   places_update_all_gadgets();
   places_menu_augmentation();
   return 1;
}
