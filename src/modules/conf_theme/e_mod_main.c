#include "e.h"
#include "e_mod_main.h"

static Eio_File *eio_ls[2] = {NULL};
static Eio_Monitor *eio_mon[2] = {NULL};
static Eina_List *handlers = NULL;
static Eina_List *sthemes = NULL;
static Eina_List *themes = NULL;
static const char *cur_theme = NULL;

static E_Module *conf_module = NULL;
static E_Int_Menu_Augmentation *maug[8] = {0};

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Settings - Theme"
};

/* menu item add hook */
static void
_e_mod_run_wallpaper_cb(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   e_configure_registry_call("appearance/wallpaper", m->zone->container, NULL);
}

static void
_e_mod_menu_wallpaper_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Wallpaper"));
   e_util_menu_item_theme_icon_set(mi, "preferences-desktop-wallpaper");
   e_menu_item_callback_set(mi, _e_mod_run_wallpaper_cb, NULL);
}

static void
_e_mod_run_theme_cb(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   e_configure_registry_call("appearance/theme", m->zone->container, NULL);
}


static void
_init_main_cb(void *data __UNUSED__, Eio_File *handler, const char *file)
{
   if (handler == eio_ls[0])
     themes = eina_list_append(themes, strdup(file));
   else if (handler == eio_ls[1])
     sthemes = eina_list_append(sthemes, strdup(file));
}

static int
_sort_cb(const char *a, const char *b)
{
   const char *f1, *f2;

   f1 = ecore_file_file_get(a);
   f2 = ecore_file_file_get(b);
   return e_util_strcasecmp(f1, f2);
}

static void
_theme_set(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Action *a;

   if (!e_util_strcmp(data, cur_theme)) return;

   e_theme_config_set("theme", data);
   e_config_save_queue();

   a = e_action_find("restart");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}

static void
_init_error_cb(void *data __UNUSED__, Eio_File *handler, int error __UNUSED__)
{
   if ((!eio_ls[0]) && (!eio_ls[1])) goto out;
   if (eio_ls[0] == handler)
     {
        eio_ls[0] = NULL;
        E_FREE_LIST(themes, free);
     }
   else
     {
        eio_ls[1] = NULL;
        E_FREE_LIST(sthemes, free);
     }
   return;
out:
   E_FREE_LIST(themes, free);
   E_FREE_LIST(sthemes, free);
}

static void
_item_new(char *file, E_Menu *m)
{
   E_Menu_Item *mi;
   char *name, *sfx;
   Eina_Bool used;

   used = !e_util_strcmp(file, cur_theme);
   name = (char*)ecore_file_file_get(file);
   if (!name) return;
   sfx = strrchr(name, '.');
   name = strndupa(name, sfx - name);
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, name);
   if (used)
     e_menu_item_disabled_set(mi, 1);
   else
     e_menu_item_callback_set(mi, _theme_set, file);
   e_menu_item_check_set(mi, 1);
   e_menu_item_toggle_set(mi, used);
}

static void
_init_done_cb(void *data __UNUSED__, Eio_File *handler)
{
   if ((!eio_ls[0]) && (!eio_ls[1])) goto out;
   if (eio_ls[0] == handler)
     {
        eio_ls[0] = NULL;
        themes = eina_list_sort(themes, 0, (Eina_Compare_Cb)_sort_cb);
     }
   else
     {
        eio_ls[1] = NULL;
        sthemes = eina_list_sort(sthemes, 0, (Eina_Compare_Cb)_sort_cb);
     }

   return;
out:
   E_FREE_LIST(themes, free);   
   E_FREE_LIST(sthemes, free);   
}

static Eina_Bool
_eio_filter_cb(void *data __UNUSED__, Eio_File *handler __UNUSED__, const char *file)
{
   return eina_str_has_extension(file, ".edj");
}

static void
_e_mod_menu_theme_del(void *d __UNUSED__)
{
   cur_theme = NULL;
}

static void
_e_mod_menu_theme_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;
   E_Config_Theme *ct;
   char *file;
   Eina_List *l;

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Theme"));
   e_util_menu_item_theme_icon_set(mi, "preferences-desktop-theme");
   e_menu_item_callback_set(mi, _e_mod_run_theme_cb, NULL);
   ct = e_theme_config_get("theme");
   if (!ct) return;

   cur_theme = ct->file;   
   m = e_menu_new();
   e_object_del_attach_func_set(E_OBJECT(m), _e_mod_menu_theme_del);
   e_menu_title_set(m, "Themes");
   e_menu_item_submenu_set(mi, m);
   e_object_unref(E_OBJECT(m));

   EINA_LIST_FOREACH(themes, l, file)
     _item_new(file, m);
   if (themes && sthemes)
     e_menu_item_separator_set(e_menu_item_new(m), 1);
   EINA_LIST_FOREACH(sthemes, l, file)
     _item_new(file, m);
}

static Eina_Bool
_monitor_theme_rescan(void *d __UNUSED__, int type __UNUSED__, Eio_Monitor_Event *ev)
{
   char buf[PATH_MAX];

   if (eio_mon[0] == ev->monitor)
     {
        if (eio_ls[0]) return ECORE_CALLBACK_RENEW;
        E_FREE_LIST(themes, free);
        e_user_dir_concat_static(buf, "themes");
        eio_ls[0] = eio_file_ls(buf, _eio_filter_cb, _init_main_cb, _init_done_cb, _init_error_cb, NULL);
     }
   else
     {
        if (eio_ls[1]) return ECORE_CALLBACK_RENEW;
        E_FREE_LIST(sthemes, free);
        e_prefix_data_concat_static(buf, "data/themes");
        eio_ls[1] = eio_file_ls(buf, _eio_filter_cb, _init_main_cb, _init_done_cb, _init_error_cb, NULL);
     }

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_monitor_error(void *d __UNUSED__, int type __UNUSED__, Eio_Monitor_Event *ev)
{
   if (eio_mon[0] == ev->monitor)
     eio_mon[0] = NULL;
   else
     eio_mon[1] = NULL;
   return ECORE_CALLBACK_RENEW;
}

EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[PATH_MAX];

   e_configure_registry_category_add("internal", -1, _("Internal"),
                                     NULL, "enlightenment/internal");
   e_configure_registry_item_add("internal/wallpaper_desk", -1, _("Wallpaper"),
                                 NULL, "preferences-system-windows", e_int_config_wallpaper_desk);
   e_configure_registry_item_add("internal/borders_border", -1, _("Border"),
                                 NULL, "preferences-system-windows", e_int_config_borders_border);

   e_configure_registry_category_add("appearance", 10, _("Look"), NULL,
                                     "preferences-look");
   e_configure_registry_item_add("appearance/wallpaper", 10, _("Wallpaper"), NULL,
                                 "preferences-desktop-wallpaper",
                                 e_int_config_wallpaper);
   e_configure_registry_item_add("appearance/theme", 20, _("Theme"), NULL,
                                 "preferences-desktop-theme",
                                 e_int_config_theme);
   e_configure_registry_item_add("appearance/xsettings", 20, _("GTK Application Theme"), NULL,
                                 "preferences-desktop-theme",
                                 e_int_config_xsettings);
   e_configure_registry_item_add("appearance/colors", 30, _("Colors"), NULL,
                                 "preferences-desktop-color",
                                 e_int_config_color_classes);
   e_configure_registry_item_add("appearance/fonts", 40, _("Fonts"), NULL,
                                 "preferences-desktop-font",
                                 e_int_config_fonts);
   e_configure_registry_item_add("appearance/borders", 50, _("Borders"), NULL,
                                 "preferences-system-windows",
                                 e_int_config_borders);
   e_configure_registry_item_add("appearance/transitions", 60, _("Transitions"), NULL,
                                 "preferences-transitions",
                                 e_int_config_transitions);
   e_configure_registry_item_add("appearance/scale", 70, _("Scaling"), NULL,
                                 "preferences-scale",
                                 e_int_config_scale);
   e_configure_registry_item_add("appearance/startup", 80, _("Startup"), NULL,
                                 "preferences-startup",
                                 e_int_config_startup);

   maug[0] =
     e_int_menus_menu_augmentation_add_sorted("config/1", _("Wallpaper"),
                                              _e_mod_menu_wallpaper_add, NULL, NULL, NULL);
   maug[1] =
     e_int_menus_menu_augmentation_add_sorted("config/1", _("Theme"),
                                              _e_mod_menu_theme_add, NULL, NULL, NULL);

   conf_module = m;
   e_module_delayed_set(m, 1);

   e_user_dir_concat_static(buf, "themes");
   eio_ls[0] = eio_file_ls(buf, _eio_filter_cb, _init_main_cb, _init_done_cb, _init_error_cb, m);
   eio_mon[0] = eio_monitor_add(buf);
   e_prefix_data_concat_static(buf, "data/themes");
   eio_ls[1] = eio_file_ls(buf, _eio_filter_cb, _init_main_cb, _init_done_cb, _init_error_cb, m);
   eio_mon[1] = eio_monitor_add(buf);

   E_LIST_HANDLER_APPEND(handlers, EIO_MONITOR_SELF_DELETED, _monitor_error, NULL);
   E_LIST_HANDLER_APPEND(handlers, EIO_MONITOR_FILE_CREATED, _monitor_theme_rescan, NULL);
   E_LIST_HANDLER_APPEND(handlers, EIO_MONITOR_FILE_DELETED, _monitor_theme_rescan, NULL);
   E_LIST_HANDLER_APPEND(handlers, EIO_MONITOR_ERROR, _monitor_error, NULL);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_Config_Dialog *cfd;

   /* remove module-supplied menu additions */
   if (maug[0])
     {
        e_int_menus_menu_augmentation_del("config/1", maug[0]);
        maug[0] = NULL;
     }
   if (maug[1])
     {
        e_int_menus_menu_augmentation_del("config/1", maug[1]);
        maug[1] = NULL;
     }

   if (eio_ls[0])
     eio_file_cancel(eio_ls[0]);
   else
     E_FREE_LIST(themes, free);

   if (eio_ls[1])
     eio_file_cancel(eio_ls[1]);
   else
     E_FREE_LIST(sthemes, free);
   if (eio_mon[0]) eio_monitor_del(eio_mon[0]);
   if (eio_mon[1]) eio_monitor_del(eio_mon[1]);
   E_FREE_LIST(handlers, ecore_event_handler_del);
   eio_ls[0] = eio_ls[1] = NULL;
   eio_mon[0] = eio_mon[1] = NULL;
   cur_theme = NULL;

   while ((cfd = e_config_dialog_get("E", "appearance/startup")))
     e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "appearance/scale")))
     e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "appearance/transitions")))
     e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "appearance/borders")))
     e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "appearance/fonts")))
     e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "appearance/colors")))
     e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "apppearance/theme")))
     e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "appearance/wallpaper")))
     e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "appearance/xsettings")))
     e_object_del(E_OBJECT(cfd));

   e_configure_registry_item_del("appearance/startup");
   e_configure_registry_item_del("appearance/scale");
   e_configure_registry_item_del("appearance/transitions");
   e_configure_registry_item_del("appearance/borders");
   e_configure_registry_item_del("appearance/fonts");
   e_configure_registry_item_del("appearance/colors");
   e_configure_registry_item_del("appearance/theme");
   e_configure_registry_item_del("appearance/wallpaper");
   e_configure_registry_item_del("appearance/xsettings");
   e_configure_registry_category_del("appearance");

   while ((cfd = e_config_dialog_get("E", "internal/borders_border")))
     e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "appearance/wallpaper")))
     e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("appearance/borders");
   e_configure_registry_item_del("internal/wallpaper_desk");
   e_configure_registry_category_del("internal");

   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
