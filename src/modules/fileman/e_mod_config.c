#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_fm_device.h"

struct _E_Config_Dialog_Data
{
   /* general view mode */
    struct
    {
       E_Fm2_View_Mode mode;
       int open_dirs_in_place;
       int selector;
       int single_click;
       int no_subdir_jump;
       int no_subdir_drop;
       int always_order;
       int link_drop;
       int fit_custom_pos;
       int show_full_path;
       int show_desktop_icons;
       int show_toolbar;
       int show_sidebar;
       int desktop_navigation;
       int menu_shows_files;
    } view;
   struct
   {
      double delay;
      double size;
      Eina_Bool enable;
      Evas_Object *delay_slider;
      Evas_Object *size_slider;
   } tooltip;
    /* display of icons */
    struct
    {
       struct
       {
          int w, h;
       } icon, list, fixed;
       struct
       {
          int show;
       } extension;
       const char *key_hint;
    } icon;
    /* how to sort files */
    struct
    {
       struct
       {
          int case_sen;
          int extension;
          int mtime;
          int size;
          struct
          {
             int first, last;
          } dirs;
       } sort;
    } list;
    /* control how you can select files */
    struct
    {
       int single;
       int windows_modifiers;
    } selection;
    /* the background - if any, and how to handle it */
    /* FIXME: not implemented yet */
    struct
    {
       const char *background;
       const char *frame, *icons;
       int         fixed;
    } theme;

    struct
    {
       int desktop;
       int auto_mount;
       int auto_open;
    } dbus;

    E_Config_Dialog *cfd;
};

static void        *_create_data(E_Config_Dialog *cfd);
static void         _fill_data(E_Config_Dialog_Data *cfdata);
static void         _free_data(E_Config_Dialog      *cfd,
                               E_Config_Dialog_Data *cfdata);
static int          _basic_apply(E_Config_Dialog      *cfd,
                                 E_Config_Dialog_Data *cfdata);
static int          _basic_check_changed(E_Config_Dialog      *cfd,
                                         E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog      *cfd,
                                  Evas                 *evas,
                                  E_Config_Dialog_Data *cfdata);

E_Config_Dialog *
e_int_config_fileman(E_Container       *con,
                     const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "fileman/fileman")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;
   v->basic.check_changed = _basic_check_changed;

   cfd = e_config_dialog_new(con, _("Fileman Settings"), "E",
                             "fileman/fileman",
                             "system-file-manager", 0, v, NULL);
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfd->cfdata = cfdata;
   cfdata->cfd = cfd;
   _fill_data(cfdata);
   return cfdata;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->view.mode = fileman_config->view.mode;
   cfdata->view.open_dirs_in_place = fileman_config->view.open_dirs_in_place;
   cfdata->view.single_click = fileman_config->view.single_click;
   cfdata->view.show_full_path = fileman_config->view.show_full_path;
   cfdata->view.show_desktop_icons = fileman_config->view.show_desktop_icons;
   cfdata->view.show_toolbar = fileman_config->view.show_toolbar;
   cfdata->view.show_sidebar = fileman_config->view.show_sidebar;
   cfdata->view.desktop_navigation = fileman_config->view.desktop_navigation;
   cfdata->view.menu_shows_files = fileman_config->view.menu_shows_files;
   cfdata->icon.icon.w = fileman_config->icon.icon.w;
   cfdata->icon.icon.h = fileman_config->icon.icon.h;
   cfdata->tooltip.delay = fileman_config->tooltip.delay;
   cfdata->tooltip.size = fileman_config->tooltip.size;
   cfdata->tooltip.enable = fileman_config->tooltip.enable;
   cfdata->icon.extension.show = fileman_config->icon.extension.show;
   cfdata->selection.windows_modifiers = fileman_config->selection.windows_modifiers;
   cfdata->list.sort.dirs.first = fileman_config->list.sort.dirs.first;
   cfdata->list.sort.case_sen = !(fileman_config->list.sort.no_case);
   cfdata->list.sort.extension = fileman_config->list.sort.extension;
   cfdata->list.sort.mtime = fileman_config->list.sort.mtime;
   cfdata->list.sort.size = fileman_config->list.sort.size;
   cfdata->dbus.desktop = e_config->device_desktop;
   cfdata->dbus.auto_mount = e_config->device_auto_mount;
   cfdata->dbus.auto_open = e_config->device_auto_open;
}

static void
_free_data(E_Config_Dialog      *cfd,
           E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfd->data);
   E_FREE(cfdata);
}

static int
_basic_apply(E_Config_Dialog *cfd  __UNUSED__,
             E_Config_Dialog_Data *cfdata)
{
   fileman_config->view.mode = cfdata->view.mode;
   fileman_config->view.open_dirs_in_place = cfdata->view.open_dirs_in_place;
   e_config->filemanager_single_click = fileman_config->view.single_click = cfdata->view.single_click;
   fileman_config->view.show_full_path = cfdata->view.show_full_path;
   fileman_config->view.show_desktop_icons = cfdata->view.show_desktop_icons;
   fileman_config->view.show_toolbar = cfdata->view.show_toolbar;
   fileman_config->view.show_sidebar = cfdata->view.show_sidebar;
   fileman_config->view.desktop_navigation = cfdata->view.desktop_navigation;
   fileman_config->view.menu_shows_files = cfdata->view.menu_shows_files;
   fileman_config->icon.extension.show = cfdata->icon.extension.show;

   fileman_config->selection.windows_modifiers = cfdata->selection.windows_modifiers;

   /* Make these two equal so that icons are proportioned correctly */
   fileman_config->icon.icon.w = cfdata->icon.icon.w;
   fileman_config->icon.icon.h = cfdata->icon.icon.w;

   fileman_config->tooltip.delay = cfdata->tooltip.delay;
   fileman_config->tooltip.size = cfdata->tooltip.size;
   fileman_config->tooltip.enable = cfdata->tooltip.enable;

   fileman_config->list.sort.dirs.first = cfdata->list.sort.dirs.first;
   fileman_config->list.sort.dirs.last = !(cfdata->list.sort.dirs.first);
   fileman_config->list.sort.extension = cfdata->list.sort.extension;
   fileman_config->list.sort.mtime = cfdata->list.sort.mtime;
   fileman_config->list.sort.size = cfdata->list.sort.size;
   fileman_config->list.sort.no_case = !(cfdata->list.sort.case_sen);

   e_config->device_desktop = cfdata->dbus.desktop;
   if(e_config->device_desktop)
     e_fm2_device_show_desktop_icons();
   else
     e_fm2_device_hide_desktop_icons();

   e_config->device_auto_mount = cfdata->dbus.auto_mount;
   e_config->device_auto_open = cfdata->dbus.auto_open;

   e_config_save_queue();

   e_fwin_reload_all();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd  __UNUSED__,
                     E_Config_Dialog_Data *cfdata)
{
   return
     (fileman_config->view.mode != cfdata->view.mode) ||
     (fileman_config->view.open_dirs_in_place != cfdata->view.open_dirs_in_place) ||
     (fileman_config->view.single_click != cfdata->view.single_click) ||
     (fileman_config->view.show_full_path != cfdata->view.show_full_path) ||
     (fileman_config->view.show_desktop_icons != cfdata->view.show_desktop_icons) ||
     (fileman_config->view.show_toolbar != cfdata->view.show_toolbar) ||
     (fileman_config->view.show_sidebar != cfdata->view.show_sidebar) ||
     (fileman_config->view.desktop_navigation != cfdata->view.desktop_navigation) ||
     (fileman_config->view.menu_shows_files != cfdata->view.menu_shows_files) ||
     (fileman_config->icon.extension.show != cfdata->icon.extension.show) ||
     (fileman_config->selection.windows_modifiers != cfdata->selection.windows_modifiers) ||
     (fileman_config->icon.icon.w != cfdata->icon.icon.w) ||
     (fileman_config->icon.icon.h != cfdata->icon.icon.w) ||
     (fileman_config->list.sort.dirs.first != cfdata->list.sort.dirs.first) ||
     (fileman_config->list.sort.dirs.last != !(cfdata->list.sort.dirs.first)) ||
     (fileman_config->list.sort.size != cfdata->list.sort.size) ||
     (fileman_config->list.sort.extension != cfdata->list.sort.extension) ||
     (fileman_config->list.sort.mtime != cfdata->list.sort.mtime) ||
     (fileman_config->list.sort.no_case != !(cfdata->list.sort.case_sen)) ||
     (fileman_config->tooltip.delay != !(cfdata->tooltip.delay)) ||
     (fileman_config->tooltip.size != !(cfdata->tooltip.size)) ||
     (fileman_config->tooltip.enable != !(cfdata->tooltip.enable)) ||
     (e_config->device_desktop != cfdata->dbus.desktop) ||
     (e_config->device_auto_mount != cfdata->dbus.auto_mount) ||
     (e_config->device_auto_open != cfdata->dbus.auto_open);
}

static void
_tooltip_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;

   e_widget_disabled_set(cfdata->tooltip.delay_slider, !cfdata->tooltip.enable);
   e_widget_disabled_set(cfdata->tooltip.size_slider, !cfdata->tooltip.enable);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd  __UNUSED__,
              Evas                 *evas,
              E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ob, *of, *otb;
   E_Radio_Group *rg;

   otb = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("View Mode"), 0);
   rg = e_widget_radio_group_new((int*)&(cfdata->view.mode));
/*    ob = e_widget_radio_add(evas, _("Icons"), 0, rg); */
/*    e_widget_disabled_set(ob, 1); */
/*    e_widget_framelist_object_append(of, ob); */
   ob = e_widget_radio_add(evas, _("Grid Icons"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Custom Icons"), 2, rg);
   e_widget_framelist_object_append(of, ob);
/*    ob = e_widget_radio_add(evas, _("Custom Grid Icons"), 3, rg); */
/*    e_widget_disabled_set(ob, 1); */
/*    e_widget_framelist_object_append(of, ob); */
/*    ob = e_widget_radio_add(evas, _("Custom Smart Grid Icons"), 4, rg); */
/*    e_widget_disabled_set(ob, 1); */
/*    e_widget_framelist_object_append(of, ob); */
   ob = e_widget_radio_add(evas, _("List"), 5, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   ob = e_widget_label_add(evas, _("Icon Size"));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 16.0, 256.0, 1.0, 0,
                            NULL, &(cfdata->icon.icon.w), 150);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("View"), o, 0, 0, 0, 0, 0.5, 0.0);

   /////////////////////////////////////////////////////////////
   o = e_widget_list_add(evas, 1, 0);

   ob = e_widget_check_add(evas, _("Directories First"),
                           &(cfdata->list.sort.dirs.first));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("File Extensions"),
                           &(cfdata->icon.extension.show));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Full Path In Title"),
                           &(cfdata->view.show_full_path));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Icons On Desktop"),
                           &(cfdata->view.show_desktop_icons));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Toolbar"),
                           &(cfdata->view.show_toolbar));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Sidebar"),
                           &(cfdata->view.show_sidebar));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Regular Files In Menu (SLOW)"),
                           &(cfdata->view.menu_shows_files));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Display"), o, 0, 0, 0, 0, 0.5, 0.0);
   /////////////////////////////////////////////////////////////
   o = e_widget_list_add(evas, 1, 0);
   ob = e_widget_check_add(evas, _("Open Dirs In Place"),
                           &(cfdata->view.open_dirs_in_place));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Case Sensitive"),
                           &(cfdata->list.sort.case_sen));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Sort By Extension"),
                           &(cfdata->list.sort.extension));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Sort By Modification Time"),
                           &(cfdata->list.sort.mtime));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Sort By Size"),
                           &(cfdata->list.sort.size));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Use Single Click"),
                           &(cfdata->view.single_click));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Use Alternate Selection Modifiers"),
                           &(cfdata->selection.windows_modifiers));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Allow Navigation On Desktop"),
                           &(cfdata->view.desktop_navigation));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Behavior"), o, 0, 0, 0, 0, 0.5, 0.0);
   /////////////////////////////////////////////////////////////

   o = e_widget_list_add(evas, 2, 0);
   {
      char buf[32];
      switch (e_config->device_detect_mode)
        {
         case EFM_MODE_USING_HAL_MOUNT:
           snprintf(buf, sizeof(buf), "%s: HAL", _("Mode"));
           break;
         case EFM_MODE_USING_UDISKS_MOUNT:
           snprintf(buf, sizeof(buf), "%s: UDISKS", _("Mode"));
           break;
         case EFM_MODE_USING_EEZE_MOUNT:
           snprintf(buf, sizeof(buf), "%s: EEZE", _("Mode"));
           break;
         default:
           snprintf(buf, sizeof(buf), "%s: RASTER", _("Mode"));
           break;
        }
      ob = e_widget_label_add(evas, buf);
      e_widget_list_object_append(o, ob, 0, 1, 0.5);
   }
   ob = e_widget_check_add(evas, _("Show device icons on desktop"),
                           &(cfdata->dbus.desktop));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Mount volumes on insert"),
                           &(cfdata->dbus.auto_mount));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Open filemanager on mount"),
                           &(cfdata->dbus.auto_open));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Device"), o, 0, 0, 0, 0, 0.5, 0.0);

   /////////////////////////////////////////////////////////////
   o = e_widget_list_add(evas, 1, 0);
   ob = e_widget_check_add(evas, _("Show tooltip"),
                           (int*)&(cfdata->tooltip.enable));
   e_widget_on_change_hook_set(ob, _tooltip_changed, cfdata);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   cfdata->tooltip.delay_slider = ob = e_widget_label_add(evas, _("Tooltip delay"));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f"), 0.0, 5.0, 0.5, 0,
                            &cfdata->tooltip.delay, NULL, 150);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   cfdata->tooltip.size_slider = ob = e_widget_label_add(evas, _("Tooltip size (Screen percentage)"));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%2.0f"), 10.0, 75.0, 5.0, 0,
                            &cfdata->tooltip.size, NULL, 150);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   _tooltip_changed(cfdata, NULL);
   e_widget_toolbook_page_append(otb, NULL, _("Tooltips"), o, 0, 0, 0, 0, 0.5, 0.0);
   e_widget_toolbook_page_show(otb, 0);
                               return otb;
}

