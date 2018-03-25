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
};

/* Local Function Prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

/* External Functions */
E_Config_Dialog *
e_int_config_places_module(E_Container *con, const char *params)
{
   E_Config_Dialog *cfd = NULL;
   E_Config_Dialog_View *v = NULL;
   char buf[PATH_MAX];

   /* is this config dialog already visible ? */
   if (e_config_dialog_find("Places", "fileman/places")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   /* Icon in the theme */
   snprintf(buf, sizeof(buf), "%s/e-module-places.edj", places_conf->module->dir);

   /* create new config dialog */
   cfd = e_config_dialog_new(con, D_("Places Settings"), "Places",
                             "fileman/places", buf, 0, v, NULL);
   places_conf->cfd = cfd;
   return cfd;
}

/* Local Functions */
static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata = NULL;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
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

   if (places_conf->fm)
     cfdata->fm = strdup(places_conf->fm);
   else
     cfdata->fm = strdup("");
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

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o = NULL, *of = NULL, *ow = NULL, *ow1 = NULL;

   o = e_widget_list_add(evas, 0, 0);

   //General frame
   of = e_widget_framelist_add(evas, D_("General"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);

   ow = e_widget_check_add(evas, D_("Show in main menu"),
                           &(cfdata->show_menu));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, D_("Hide the gadget header"),
                           &(cfdata->hide_header));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, D_("Mount volumes at boot"),
                           &(cfdata->boot_mount));
   e_widget_framelist_object_append(of, ow);

   ow1 = e_widget_check_add(evas, D_("Mount volumes on insert"),
                           &(cfdata->auto_mount));
   e_widget_framelist_object_append(of, ow1);

   ow = e_widget_check_add(evas, D_("Open filemanager on insert"),
                           &(cfdata->auto_open));
   e_widget_framelist_object_append(of, ow);
   e_widget_on_change_hook_set(ow1, _mount_on_insert_click, ow);
   if (!cfdata->auto_mount)
     e_widget_disabled_set(ow, 1);

   ow = e_widget_check_add(evas, D_("Use a custom file manager"), &(cfdata->fm_chk));
   e_widget_check_checked_set(ow, strlen(cfdata->fm) > 0 ? 1 : 0);
   e_widget_on_change_hook_set(ow, _custom_fm_click, cfdata);
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_entry_add(evas, &(cfdata->fm), NULL, NULL, NULL);
   e_widget_disabled_set(ow, strlen(cfdata->fm) > 0 ? 0 : 1);
   cfdata->entry = ow;
   e_widget_framelist_object_append(of, ow);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   //Display frame
   of = e_widget_framelist_add(evas, D_("Show in menu"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);

   ow = e_widget_check_add(evas, D_("Home"), &(cfdata->show_home));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, D_("Desktop"), &(cfdata->show_desk));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, D_("Trash"), &(cfdata->show_trash));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, D_("Filesystem"), &(cfdata->show_root));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, D_("Temp"), &(cfdata->show_temp));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, D_("Favorites"), &(cfdata->show_bookm));
   e_widget_framelist_object_append(of, ow);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
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

   const char *fm = eina_stringshare_add(cfdata->fm);
   eina_stringshare_del(places_conf->fm);
   places_conf->fm = fm;

   e_config_save_queue();
   places_update_all_gadgets();
   places_menu_augmentation();
   return 1;
}
