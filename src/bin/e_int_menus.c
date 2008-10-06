/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _Main_Data Main_Data;

struct _Main_Data
{
   E_Menu *menu;
   E_Menu *apps;
   E_Menu *all_apps;
   E_Menu *desktops;
   E_Menu *clients;
   E_Menu *enlightenment;
   E_Menu *config;
   E_Menu *lost_clients;
   E_Menu *sys;
};

/* local subsystem functions */
static void _e_int_menus_main_del_hook       (void *obj);
static void _e_int_menus_main_about          (void *data, E_Menu *m, E_Menu_Item *mi);
//static void _e_int_menus_fwin_favorites_item_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static int  _e_int_menus_main_lock_defer_cb  (void *data);
static void _e_int_menus_main_lock           (void *data, E_Menu *m, E_Menu_Item*mi);
static void _e_int_menus_main_restart        (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_main_logout         (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_main_exit           (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_main_halt           (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_main_reboot         (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_main_suspend        (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_main_hibernate      (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_apps_scan           (E_Menu *m, Efreet_Menu *menu);
static void _e_int_menus_apps_start          (void *data, E_Menu *m);
static void _e_int_menus_apps_free_hook      (void *obj);
static void _e_int_menus_apps_free_hook2     (void *obj);
static void _e_int_menus_apps_run            (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_apps_drag           (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_config_pre_cb       (void *data, E_Menu *m);
static void _e_int_menus_config_free_hook    (void *obj);
static void _e_int_menus_clients_pre_cb      (void *data, E_Menu *m);
static void _e_int_menus_clients_item_create (E_Border *bd, E_Menu *m);
static void _e_int_menus_clients_free_hook   (void *obj);
static void _e_int_menus_clients_item_cb     (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_clients_icon_cb     (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_clients_cleanup_cb  (void *data, E_Menu *m, E_Menu_Item *mi);
static int  _e_int_menus_clients_group_desk_cb    (void *d1, void *d2);
static int  _e_int_menus_clients_group_class_cb   (void *d1, void *d2);
static int  _e_int_menus_clients_sort_alpha_cb    (void *d1, void *d2);
static int  _e_int_menus_clients_sort_z_order_cb  (void *d1, void *d2);
static void _e_int_menus_clients_add_by_class   (Evas_List *borders, E_Menu *m);
static void _e_int_menus_clients_add_by_desk    (E_Desk *curr_desk, Evas_List *borders, E_Menu *m);
static void _e_int_menus_clients_add_by_none    (Evas_List *borders, E_Menu *m);
static void _e_int_menus_clients_menu_add_iconified  (Evas_List *borders, E_Menu *m);
static const char *_e_int_menus_clients_title_abbrv (const char *title);
static void _e_int_menus_virtuals_pre_cb     (void *data, E_Menu *m);
static void _e_int_menus_virtuals_item_cb    (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_themes_about        (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_lost_clients_pre_cb    (void *data, E_Menu *m);
static void _e_int_menus_lost_clients_free_hook (void *obj);
static void _e_int_menus_lost_clients_item_cb   (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_sys_pre_cb          (void *data, E_Menu *m);
static void _e_int_menus_sys_free_hook       (void *obj);
static void _e_int_menus_augmentation_add    (E_Menu *m, Evas_List *augmentation);
static void _e_int_menus_augmentation_del    (E_Menu *m, Evas_List *augmentation);
static void _e_int_menus_shelves_pre_cb      (void *data, E_Menu *m);
static void _e_int_menus_shelves_item_cb     (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_shelves_add_cb      (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_shelves_del_cb      (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_main_showhide       (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_desk_item_cb        (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_items_del_hook      (void *obj);
static void _e_int_menus_item_label_set      (Efreet_Menu *entry, E_Menu_Item *mi);

/* local subsystem globals */
static Evas_Hash *_e_int_menus_augmentation = NULL;

/* externally accessible functions */
EAPI E_Menu *
e_int_menus_main_new(void)
{
   E_Menu *m, *subm;
   E_Menu_Item *mi;
   Main_Data *dat;
   Evas_List *l = NULL;

   dat = calloc(1, sizeof(Main_Data));
   m = e_menu_new();
   e_menu_title_set(m, _("Main"));
   dat->menu = m;
   e_object_data_set(E_OBJECT(m), dat);   
   e_object_del_attach_func_set(E_OBJECT(m), _e_int_menus_main_del_hook);

   e_menu_category_set(m, "main");

   l = evas_hash_find(_e_int_menus_augmentation, "main/0");
   if (l) _e_int_menus_augmentation_add(m, l);

   if (e_config->menu_favorites_show) 
     {
	subm = e_int_menus_favorite_apps_new();
	if (subm)
	  {
	     dat->apps = subm; 
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Favorite Applications"));
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/favorites");
	     e_menu_item_submenu_set(mi, subm);
	  }
     }

   if (e_config->menu_apps_show) 
     {
	subm = e_int_menus_all_apps_new();
	dat->all_apps = subm;
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Applications"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/applications");
	e_menu_item_submenu_set(mi, subm);
     }

   l = evas_hash_find(_e_int_menus_augmentation, "main/1");
   if (l) _e_int_menus_augmentation_add(m, l);

   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);

   l = evas_hash_find(_e_int_menus_augmentation, "main/2");
   if (l) _e_int_menus_augmentation_add(m, l);

   subm = e_int_menus_desktops_new();
   dat->desktops = subm;
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Desktop"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/desktops");
   e_menu_item_submenu_set(mi, subm);

   subm = e_int_menus_clients_new();
   e_object_data_set(E_OBJECT(subm), dat);   
   dat->clients = subm;
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Windows"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/windows");
   e_menu_item_submenu_set(mi, subm);
/*  
   subm = e_int_menus_lost_clients_new();
   e_object_data_set(E_OBJECT(subm), dat);   
   dat->lost_clients = subm;
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Lost Windows"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/lost_windows");
   e_menu_item_submenu_set(mi, subm);
 */

   l = evas_hash_find(_e_int_menus_augmentation, "main/3");
   if (l) _e_int_menus_augmentation_add(m, l);

   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);

   l = evas_hash_find(_e_int_menus_augmentation, "main/4");
   if (l) _e_int_menus_augmentation_add(m, l);

   subm = e_menu_new();
   dat->enlightenment = subm;
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Enlightenment"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/e");
   e_object_free_attach_func_set(E_OBJECT(subm), _e_int_menus_items_del_hook);
   e_menu_item_submenu_set(mi, subm);

   mi = e_menu_item_new(subm);
   e_menu_item_label_set(mi, _("About"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/e");
   e_menu_item_callback_set(mi, _e_int_menus_main_about, NULL);

   mi = e_menu_item_new(subm);
   e_menu_item_label_set(mi, _("Theme"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/themes");
   e_menu_item_callback_set(mi, _e_int_menus_themes_about, NULL);

   l = evas_hash_find(_e_int_menus_augmentation, "main/5");
   if (l) _e_int_menus_augmentation_add(m, l);

   mi = e_menu_item_new(subm);
   e_menu_item_separator_set(mi, 1);

   l = evas_hash_find(_e_int_menus_augmentation, "main/6");
   if (l) _e_int_menus_augmentation_add(m, l);

   mi = e_menu_item_new(subm);
   e_menu_item_label_set(mi, _("Restart"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/reset");
   e_menu_item_callback_set(mi, _e_int_menus_main_restart, NULL);

   mi = e_menu_item_new(subm);
   e_menu_item_label_set(mi, _("Exit"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/exit");
   e_menu_item_callback_set(mi, _e_int_menus_main_exit, NULL);

   l = evas_hash_find(_e_int_menus_augmentation, "main/7");
   if (l) _e_int_menus_augmentation_add(m, l);

   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);

   l = evas_hash_find(_e_int_menus_augmentation, "main/8");
   if (l) _e_int_menus_augmentation_add(m, l);

   subm = e_int_menus_config_new();
   dat->config = subm;
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Configuration"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/configuration");
   e_menu_item_submenu_set(mi, subm);

   l = evas_hash_find(_e_int_menus_augmentation, "main/9");
   if (l) _e_int_menus_augmentation_add(m, l);

   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);

   l = evas_hash_find(_e_int_menus_augmentation, "main/10");
   if (l) _e_int_menus_augmentation_add(m, l);

   subm = e_int_menus_sys_new();
   dat->sys = subm;
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("System"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/system");
   e_menu_item_submenu_set(mi, subm);

   l = evas_hash_find(_e_int_menus_augmentation, "main/11");
   if (l) _e_int_menus_augmentation_add(m, l);

   return m;
}

EAPI E_Menu *
e_int_menus_apps_new(const char *dir)
{
   E_Menu *m;

   m = e_menu_new();
   if (dir) e_object_data_set(E_OBJECT(m), strdup(dir));
   e_menu_pre_activate_callback_set(m, _e_int_menus_apps_start, NULL);
   e_object_del_attach_func_set(E_OBJECT(m), _e_int_menus_items_del_hook);
   e_object_free_attach_func_set(E_OBJECT(m), _e_int_menus_apps_free_hook);
   return m;
}

EAPI E_Menu *
e_int_menus_desktops_new(void)
{
   E_Menu *m, *subm;
   E_Menu_Item *mi;

   m = e_menu_new();

   subm = e_menu_new();
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Virtual"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/desktops");
   e_menu_pre_activate_callback_set(subm, _e_int_menus_virtuals_pre_cb, NULL);
   e_object_free_attach_func_set(E_OBJECT(subm), _e_int_menus_items_del_hook);
   e_menu_item_submenu_set(mi, subm);

   subm = e_menu_new();
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Shelves"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf");
   e_menu_pre_activate_callback_set(subm, _e_int_menus_shelves_pre_cb, NULL);
   e_object_free_attach_func_set(E_OBJECT(subm), _e_int_menus_items_del_hook);
   e_menu_item_submenu_set(mi, subm);

   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Show/Hide All Windows"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/showhide");
   e_menu_item_callback_set(mi, _e_int_menus_main_showhide, NULL);

   e_object_free_attach_func_set(E_OBJECT(m), _e_int_menus_items_del_hook);
   return m;
}

EAPI E_Menu *
e_int_menus_favorite_apps_new(void)
{
   E_Menu *m = NULL;
   char buf[PATH_MAX];
   const char *homedir;

   homedir = e_user_homedir_get();
   snprintf(buf, sizeof(buf), 
            "%s/.e/e/applications/menu/favorite.menu", homedir);

   if (ecore_file_exists(buf)) m = e_int_menus_apps_new(buf);
   return m;
}

EAPI E_Menu *
e_int_menus_all_apps_new(void)
{
   E_Menu *m;

   m = e_int_menus_apps_new(NULL);
   return m;
}

EAPI E_Menu *
e_int_menus_config_new(void)
{
   E_Menu *m;

   m = e_menu_new();
   e_menu_pre_activate_callback_set(m, _e_int_menus_config_pre_cb, NULL);
   return m;
}

EAPI E_Menu *
e_int_menus_clients_new(void)
{
   E_Menu *m;

   m = e_menu_new();
   e_menu_pre_activate_callback_set(m, _e_int_menus_clients_pre_cb, NULL);
   return m;
}

EAPI E_Menu *
e_int_menus_lost_clients_new(void)
{
   E_Menu *m;

   m = e_menu_new();
   e_menu_pre_activate_callback_set(m, _e_int_menus_lost_clients_pre_cb, NULL);
   return m;
}

EAPI E_Menu *
e_int_menus_sys_new(void)
{
   E_Menu *m;

   m = e_menu_new();
   e_menu_pre_activate_callback_set(m, _e_int_menus_sys_pre_cb, NULL);
   return m;
}

EAPI E_Int_Menu_Augmentation *
e_int_menus_menu_augmentation_add(const char *menu,
				  void (*func_add) (void *data, E_Menu *m),
				  void *data_add,
				  void (*func_del) (void *data, E_Menu *m),
				  void *data_del)
{
   E_Int_Menu_Augmentation *maug;
   Evas_List *l = NULL;

   maug = E_NEW(E_Int_Menu_Augmentation, 1);
   if (!maug) return NULL;

   maug->add.func = func_add;
   maug->add.data = data_add;

   maug->del.func = func_del;
   maug->del.data = data_del;

   l = evas_hash_find(_e_int_menus_augmentation, menu);
   if (l) 
     {
        _e_int_menus_augmentation = 
          evas_hash_del(_e_int_menus_augmentation, menu, l);
     }

   l = evas_list_append(l, maug);
   _e_int_menus_augmentation = 
     evas_hash_add(_e_int_menus_augmentation, menu, l);

   return maug;
}

EAPI void
e_int_menus_menu_augmentation_del(const char *menu, E_Int_Menu_Augmentation *maug)
{
   Evas_List *l = NULL;

   l = evas_hash_find(_e_int_menus_augmentation, menu);
   if (l)
     {
	/*
	 * We should always add the list to the hash, in case the list
	 * becomes empty, or the first element is removed.
	 */
	_e_int_menus_augmentation = 
          evas_hash_del(_e_int_menus_augmentation, menu, l);

	l = evas_list_remove(l, maug);
	if (l) 
          {
             _e_int_menus_augmentation = 
               evas_hash_add(_e_int_menus_augmentation, menu, l);
          }
     }
   free(maug);
}

/* local subsystem functions */
static void
_e_int_menus_main_del_hook(void *obj)
{
   Main_Data *dat;
   E_Menu *m;

   m = obj;
   dat = e_object_data_get(E_OBJECT(obj));
   if (dat)
     {
	if (dat->apps) e_object_del(E_OBJECT(dat->apps));
	if (dat->all_apps) e_object_del(E_OBJECT(dat->all_apps));
	e_object_del(E_OBJECT(dat->desktops));
	e_object_del(E_OBJECT(dat->clients));
	e_object_del(E_OBJECT(dat->enlightenment));
	e_object_del(E_OBJECT(dat->config));
	if (dat->lost_clients) e_object_del(E_OBJECT(dat->lost_clients));
	e_object_del(E_OBJECT(dat->sys));
	free(dat);
     }
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "main/0"));
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "main/1"));
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "main/2"));
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "main/3"));
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "main/4"));
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "main/5"));
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "main/6"));
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "main/7"));
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "main/8"));
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "main/9"));
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "main/10"));
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "main/11"));
}

static void
_e_int_menus_main_about(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_About *about;

   about = e_about_new(e_container_current_get(e_manager_current_get()));
   if (about) e_about_show(about);
}

static void
_e_int_menus_themes_about(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Theme_About *about;

   about = e_theme_about_new(e_container_current_get(e_manager_current_get()));
   if (about) e_theme_about_show(about);
}

/*
static void
_e_int_menus_fwin_favorites_item_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   e_fwin_new(m->zone->container, "favorites", "/");
}
*/

/* FIXME: this is a workaround for menus' haveing a key grab AND exebuf
 * wanting one too
 */
static int
_e_int_menus_main_lock_defer_cb(void *data)
{
   e_desklock_show();
   return 0;
}

static void
_e_int_menus_main_lock(void *data, E_Menu *m, E_Menu_Item *mi)
{
   /* this is correct - should be after other idle enteres have run - i.e.
    * after e_menu's idler_enterer has been run */
   ecore_idle_enterer_add(_e_int_menus_main_lock_defer_cb, m->zone);
}

static void
_e_int_menus_main_showhide(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Action *act;

   act = e_action_find("desk_deskshow_toggle");
   if (act) act->func.go(E_OBJECT(m->zone), NULL);
}

static void
_e_int_menus_main_restart(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Action *a;

   a = e_action_find("restart");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}

static void
_e_int_menus_main_logout(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Action *a;

   a = e_action_find("logout");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}

static void
_e_int_menus_main_exit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Action *a;

   a = e_action_find("exit");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}

static void
_e_int_menus_main_halt(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Action *a;

   a = e_action_find("halt");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}

static void
_e_int_menus_main_reboot(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Action *a;

   a = e_action_find("reboot");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}

static void
_e_int_menus_main_suspend(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Action *a;

   a = e_action_find("suspend");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}

static void
_e_int_menus_main_hibernate(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Action *a;

   a = e_action_find("hibernate");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}

static void
_e_int_menus_apps_scan(E_Menu *m, Efreet_Menu *menu)
{
   E_Menu_Item *mi;

   if (menu->entries)
     {
	Efreet_Menu *entry;

        ecore_list_first_goto(menu->entries);
	while ((entry = ecore_list_next(menu->entries)))
	  {
	     mi = e_menu_item_new(m);

	     _e_int_menus_item_label_set(entry, mi);

	     if (entry->icon)
	       {
		  if (entry->icon[0] == '/')
		    e_menu_item_icon_file_set(mi, entry->icon);
		  else
		    {
		       char *file;

		       file = efreet_icon_path_find(e_config->icon_theme, 
                                                    entry->icon, 24);
		       e_menu_item_icon_file_set(mi, file);
		       E_FREE(file);
		    }
	       }
	     if (entry->type == EFREET_MENU_ENTRY_SEPARATOR)
	       e_menu_item_separator_set(mi, 1);
	     else if (entry->type == EFREET_MENU_ENTRY_DESKTOP)
	       {
		  e_menu_item_callback_set(mi, _e_int_menus_apps_run, 
                                           entry->desktop);
		  e_menu_item_drag_callback_set(mi, _e_int_menus_apps_drag, 
                                                entry->desktop);
	       }
	     else if (entry->type == EFREET_MENU_ENTRY_MENU)
	       {
		  E_Menu *subm;

		  subm = e_menu_new();
		  e_menu_pre_activate_callback_set(subm, 
                                                   _e_int_menus_apps_start, 
                                                   entry);
		  e_object_del_attach_func_set(E_OBJECT(subm), 
                                               _e_int_menus_items_del_hook);
		  e_menu_item_submenu_set(mi, subm);
	       }
	     /* TODO: Highlight header
	     else if (entry->type == EFREET_MENU_ENTRY_HEADER)
	     */
	  }
     }
   else
     {
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("(No Applications)"));
     }
}

static void
_e_int_menus_apps_start(void *data, E_Menu *m)
{
   Efreet_Menu *menu;

   menu = data;
   if (!menu)
     {
	char *dir = NULL;

	dir = e_object_data_get(E_OBJECT(m));
	if (dir)
	  {
	     menu = efreet_menu_parse(dir);
	     free(dir);
	  }
	else menu = efreet_menu_get();
	e_object_data_set(E_OBJECT(m), menu);
	e_object_free_attach_func_set(E_OBJECT(m), 
                                      _e_int_menus_apps_free_hook2);
     }
   if (menu) _e_int_menus_apps_scan(m, menu);
   e_menu_pre_activate_callback_set(m, NULL, NULL);
}

static void
_e_int_menus_items_del_hook(void *obj)
{
   E_Menu *m;
   Evas_List *l = NULL;

   m = obj;
   for (l = m->items; l; l = l->next)
     {
	E_Menu_Item *mi;

	mi = l->data;
	if (mi->submenu) e_object_del(E_OBJECT(mi->submenu));
     }
}

static void
_e_int_menus_apps_free_hook(void *obj)
{
   E_Menu *m;
   char *dir;

   m = obj;
   dir = e_object_data_get(E_OBJECT(m));
   E_FREE(dir);
}

static void
_e_int_menus_apps_free_hook2(void *obj)
{
   E_Menu *m;
   Efreet_Menu *menu;

   m = obj;
   menu = e_object_data_get(E_OBJECT(m));
   if (menu) efreet_menu_free(menu);
}

static void
_e_int_menus_apps_run(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Efreet_Desktop *desktop;

   desktop = data;
   e_exec(m->zone, desktop, NULL, NULL, "menu/apps");
}

static void
_e_int_menus_apps_drag(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Efreet_Desktop *desktop;

   desktop = data;

   /* start drag! */
   if (mi->icon_object)
      {
	 E_Drag *drag;
	 Evas_Object *o = NULL;
	 Evas_Coord x, y, w, h;
	 unsigned int size;
	 const char *drag_types[] = { "enlightenment/desktop" };

	 evas_object_geometry_get(mi->icon_object, &x, &y, &w, &h);
	 drag = e_drag_new(m->zone->container, x, y, drag_types, 1, desktop, -1,
			   NULL, NULL);

	 size = MIN(w, h);
	  o = e_util_desktop_icon_add(desktop, size, e_drag_evas_get(drag));
	 e_drag_object_set(drag, o);
	  e_drag_resize(drag, w, h);
	 e_drag_start(drag, mi->drag.x + w, mi->drag.y + h);
      }
}

static void
_e_int_menus_virtuals_pre_cb(void *data, E_Menu *m)
{
   E_Menu_Item *mi;
   E_Menu *root;

   e_menu_pre_activate_callback_set(m, NULL, NULL);

   root = e_menu_root_get(m);
   if ((root) && (root->zone))
     {
	E_Zone *zone;
	int i;

	zone = root->zone;
	for (i = 0; i < zone->desk_x_count * zone->desk_y_count; i++)
	  {
	     E_Desk *desk;

             desk = zone->desks[i];
	     mi = e_menu_item_new(m);
	     e_menu_item_radio_group_set(mi, 1);
	     e_menu_item_radio_set(mi, 1);
	     e_menu_item_label_set(mi, desk->name);
	     if (desk == e_desk_current_get(zone)) 
	       e_menu_item_toggle_set(mi, 1); 
	     e_menu_item_callback_set(mi, _e_int_menus_virtuals_item_cb, desk);
	  }
     }

   if (e_configure_registry_exists("screen/virtual_desktops"))
     {
	mi = e_menu_item_new(m);
	e_menu_item_separator_set(mi, 1);

	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Configure Virtual Desktops"));
	e_menu_item_callback_set(mi, _e_int_menus_desk_item_cb, NULL);
     }
}

static void
_e_int_menus_desk_item_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   e_configure_registry_call("screen/virtual_desktops", m->zone->container, NULL);
}

static void
_e_int_menus_virtuals_item_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Desk *desk = data;

   e_desk_show(desk);
}

static void
_e_int_menus_module_item_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   e_configure_registry_call("extensions/modules", m->zone->container, NULL);
}

static void
_e_int_menus_config_pre_cb(void *data, E_Menu *m)
{
   E_Menu_Item *mi;
   Evas_List *l = NULL;

   e_menu_pre_activate_callback_set(m, NULL, NULL);

   l = evas_hash_find(_e_int_menus_augmentation, "config/0");
   if (l)
     {
	_e_int_menus_augmentation_add(m, l);
	mi = e_menu_item_new(m);
	e_menu_item_separator_set(mi, 1);
     }
   
   if (e_configure_registry_exists("extensions/modules"))
     {
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Modules"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/modules");
	e_menu_item_callback_set(mi, _e_int_menus_module_item_cb, NULL);
     }

   l = evas_hash_find(_e_int_menus_augmentation, "config/1");
   if (l) _e_int_menus_augmentation_add(m, l);

   l = evas_hash_find(_e_int_menus_augmentation, "config/2");
   if (l)
     {
	mi = e_menu_item_new(m);
	e_menu_item_separator_set(mi, 1);

	_e_int_menus_augmentation_add(m, l);
     }

   e_object_free_attach_func_set(E_OBJECT(m), _e_int_menus_config_free_hook);
}

static void
_e_int_menus_config_free_hook(void *obj)
{
   E_Menu *m;

   m = obj;
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "config/0"));
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "config/1"));
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "config/2"));
}

static void
_e_int_menus_sys_pre_cb(void *data, E_Menu *m)
{
   E_Menu_Item *mi;
   Evas_List *l = NULL;

   e_menu_pre_activate_callback_set(m, NULL, NULL);

   l = evas_hash_find(_e_int_menus_augmentation, "sys/0");
   if (l)
     {
	_e_int_menus_augmentation_add(m, l);
	
	mi = e_menu_item_new(m);
	e_menu_item_separator_set(mi, 1);
     }

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Lock Screen"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/desklock_menu");
   e_menu_item_callback_set(mi, _e_int_menus_main_lock, NULL);

   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);

   if (e_sys_action_possible_get(E_SYS_HALT) ||
       e_sys_action_possible_get(E_SYS_REBOOT) ||
       e_sys_action_possible_get(E_SYS_SUSPEND) ||
       e_sys_action_possible_get(E_SYS_HIBERNATE))
     {
	if (e_sys_action_possible_get(E_SYS_SUSPEND))
	  {
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Suspend"));
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/suspend");
	     e_menu_item_callback_set(mi, _e_int_menus_main_suspend, NULL);
	  }
	if (e_sys_action_possible_get(E_SYS_HIBERNATE))
	  {
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Hibernate"));
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/hibernate");
	     e_menu_item_callback_set(mi, _e_int_menus_main_hibernate, NULL);
	  }
	if (e_sys_action_possible_get(E_SYS_REBOOT))
	  {
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Reboot"));
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/reboot");
	     e_menu_item_callback_set(mi, _e_int_menus_main_reboot, NULL);
	  }
	if (e_sys_action_possible_get(E_SYS_HALT))
	  {
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Shut Down"));
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/halt");
	     e_menu_item_callback_set(mi, _e_int_menus_main_halt, NULL);
	  }
	mi = e_menu_item_new(m);
	e_menu_item_separator_set(mi, 1);
     }

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Logout"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/logout");
   e_menu_item_callback_set(mi, _e_int_menus_main_logout, NULL);

   l = evas_hash_find(_e_int_menus_augmentation, "sys/1");
   if (l)
     {
	_e_int_menus_augmentation_add(m, l);
	
	mi = e_menu_item_new(m);
	e_menu_item_separator_set(mi, 1);
     }

   e_object_free_attach_func_set(E_OBJECT(m), _e_int_menus_sys_free_hook);
}

static void
_e_int_menus_sys_free_hook(void *obj)
{
   E_Menu *m;

   m = obj;
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "sys/0"));
   _e_int_menus_augmentation_del(m, evas_hash_find(_e_int_menus_augmentation, "sys/1"));
}

static int
_e_int_menus_clients_group_desk_cb(void *d1, void *d2)
{
   E_Border *bd1;
   E_Border *bd2;
   int j,k;

   if (!d1) return 1;
   if (!d2) return -1;

   bd1 = d1;
   bd2 = d2;

   j = bd1->desk->y * 12 + bd1->desk->x;
   k = bd2->desk->y * 12 + bd2->desk->x;

   if (j > k) return 1;
   if (j < k) return -1;
   return -1;   /* Returning '-1' on equal is intentional */
}

static int
_e_int_menus_clients_group_class_cb(void *d1, void *d2)
{
   E_Border *bd1;
   E_Border *bd2;

   if (!d1) return 1;
   if (!d2) return -1;

   bd1 = d1;
   bd2 = d2;
   
   if (strcmp((const char*)bd1->client.icccm.class, 
	      (const char*)bd2->client.icccm.class) > 0) 
     return 1;

   if (strcmp((const char*)bd1->client.icccm.class, 
	      (const char*)bd2->client.icccm.class) < 0) 
     return -1;

   return -1;   /* Returning '-1' on equal is intentional */
}

static int
_e_int_menus_clients_sort_alpha_cb(void *d1, void *d2)
{
   E_Border *bd1;
   E_Border *bd2;
   const char *name1;
   const char *name2;

   if (!d1) return 1;
   if (!d2) return -1;

   bd1 = d1;
   bd2 = d2;
   name1 = e_border_name_get(bd1);
   name2 = e_border_name_get(bd2);

   if (strcasecmp(name1, name2) > 0) return 1;
   if (strcasecmp(name1, name2) < 0) return -1;
   return 0;
}

static int
_e_int_menus_clients_sort_z_order_cb(void *d1, void *d2)
{
   E_Border *bd1, *bd2;

   if (!d1) return 1;
   if (!d2) return -1;

   bd1 = d1;
   bd2 = d2;

   if (bd1->layer < bd2->layer) return 1;
   if (bd1->layer > bd2->layer) return -1;
   return 0;   
}

static void
_e_int_menus_clients_menu_add_iconified(Evas_List *borders, E_Menu *m)
{
   Evas_List *l = NULL;
   E_Menu_Item *mi;

   if (evas_list_count(borders) > 0)
     { 
	mi = e_menu_item_new(m); 
	e_menu_item_separator_set(mi, 1); 

	for (l = borders; l; l = l->next)
	  { 
	     E_Border *bd; 

	     bd = l->data;
	     _e_int_menus_clients_item_create(bd, m);
	  }
     }
}

static void
_e_int_menus_clients_add_by_class(Evas_List *borders, E_Menu *m)
{
   Evas_List *l = NULL, *ico = NULL;
   E_Menu *subm = NULL;
   E_Menu_Item *mi;
   char *class = NULL;

   class = strdup("");
   for (l = borders; l; l = l->next) 
     {
	E_Border *bd; 

	bd = l->data; 
	if ((bd->iconic) && 
	    (e_config->clientlist_separate_iconified_apps == E_CLIENTLIST_GROUPICONS_SEP))
	  { 
	     ico = evas_list_append(ico, bd); 
	     continue;
	  }

	if (((strcmp(class, bd->client.icccm.class) != 0) && 
	    e_config->clientlist_separate_with != E_CLIENTLIST_GROUP_SEP_NONE))
	  { 
	     if (e_config->clientlist_separate_with == E_CLIENTLIST_GROUP_SEP_MENU) 
	       { 
		  if ((subm) && (mi)) e_menu_item_submenu_set(mi, subm); 
		  mi = e_menu_item_new(m); 
		  e_menu_item_label_set(mi, bd->client.icccm.class); 
		  e_util_menu_item_edje_icon_set(mi, "enlightenment/windows"); 
		  subm = e_menu_new(); 
	       } 
	     else 
	       { 
		  mi = e_menu_item_new(m); 
		  e_menu_item_separator_set(mi, 1); 
	       } 
	     class = strdup(bd->client.icccm.class); 
	  } 
	if (e_config->clientlist_separate_with == E_CLIENTLIST_GROUP_SEP_MENU) 
	  _e_int_menus_clients_item_create(bd, subm); 
	else  
	  _e_int_menus_clients_item_create(bd, m); 
     }

   if ((e_config->clientlist_separate_with == E_CLIENTLIST_GROUP_SEP_MENU) 
       && (subm) && (mi)) 
     e_menu_item_submenu_set(mi, subm);

   _e_int_menus_clients_menu_add_iconified(ico, m);
}

static void
_e_int_menus_clients_add_by_desk(E_Desk *curr_desk, Evas_List *borders, E_Menu *m)
{
   E_Desk *desk = NULL;
   Evas_List *l = NULL, *alt = NULL, *ico = NULL;
   E_Menu *subm;
   E_Menu_Item *mi;

   /* Deal with present desk first */
   for (l = borders; l; l = l->next)
     {
	E_Border *bd;

	bd = l->data; 
	if (bd->iconic && e_config->clientlist_separate_iconified_apps && E_CLIENTLIST_GROUPICONS_SEP) 
	  { 
	     ico = evas_list_append(ico, bd); 
	     continue; 
	  }

	if (bd->desk != curr_desk)
	  {
	     if ((!bd->iconic) || 
		 (bd->iconic && e_config->clientlist_separate_iconified_apps == 
		  E_CLIENTLIST_GROUPICONS_OWNER))
	       {
		  alt = evas_list_append(alt, bd); 
		  continue;
	       }
	  }
	else
	  _e_int_menus_clients_item_create(bd, m);
     }

   desk = NULL;
   subm = NULL;
   if (evas_list_count(alt) > 0) 
     {
	if (e_config->clientlist_separate_with == E_CLIENTLIST_GROUP_SEP_MENU)
	  {
	     mi = e_menu_item_new(m);
	     e_menu_item_separator_set(mi, 1);
	  }

	for (l = alt; l; l = l->next)
	  { 
	     E_Border *bd;

	     bd = l->data; 
	     if ((bd->desk != desk) && 
		 (e_config->clientlist_separate_with != E_CLIENTLIST_GROUP_SEP_NONE))
	       {
		  if (e_config->clientlist_separate_with == E_CLIENTLIST_GROUP_SEP_MENU)
		    { 
		       if (subm && mi) e_menu_item_submenu_set(mi, subm); 
		       mi = e_menu_item_new(m); 
		       e_menu_item_label_set(mi, bd->desk->name); 
		       e_util_menu_item_edje_icon_set(mi, "enlightenment/desktops");
		       subm = e_menu_new(); 
		    }
		  else
		    { 
		       mi = e_menu_item_new(m); 
		       e_menu_item_separator_set(mi, 1);
		    }
		  desk = bd->desk;
	       } 
	     if (e_config->clientlist_separate_with == E_CLIENTLIST_GROUP_SEP_MENU) 
	       _e_int_menus_clients_item_create(bd, subm); 
	     else  
	       _e_int_menus_clients_item_create(bd, m);
	  }
	if (e_config->clientlist_separate_with == E_CLIENTLIST_GROUP_SEP_MENU 
	    && (subm) && (mi)) 
	  e_menu_item_submenu_set(mi, subm);
     }

   _e_int_menus_clients_menu_add_iconified(ico, m);
}

static void
_e_int_menus_clients_add_by_none(Evas_List *borders, E_Menu *m)
{
   Evas_List *l = NULL, *ico = NULL;

   for (l = borders; l; l = l->next)
     {
	E_Border *bd;

	bd = l->data;
	if ((bd->iconic) && (e_config->clientlist_separate_iconified_apps) && 
            (E_CLIENTLIST_GROUPICONS_SEP)) 
	  { 
	     ico = evas_list_append(ico, bd); 
	     continue; 
	  }
	_e_int_menus_clients_item_create(bd, m);
     }

   _e_int_menus_clients_menu_add_iconified(ico, m);
}

static void
_e_int_menus_clients_pre_cb(void *data, E_Menu *m)
{
   E_Menu *subm;
   E_Menu_Item *mi;
   Evas_List *l = NULL, *borders = NULL;
   E_Zone *zone = NULL;
   E_Desk *desk = NULL;
   Main_Data *dat;

   e_menu_pre_activate_callback_set(m, NULL, NULL);
   /* get the current clients */
   zone = e_zone_current_get(e_container_current_get(e_manager_current_get()));
   desk = e_desk_current_get(zone); 

   if (e_config->clientlist_sort_by == E_CLIENTLIST_SORT_MOST_RECENT) 
     l = e_border_focus_stack_get(); 
   else
     l = e_border_client_list();
   for (; l; l = l->next)
     {
	E_Border *border;

	border = l->data;
	if (border->client.netwm.state.skip_taskbar) continue;
	if (border->user_skip_winlist) continue;
	if ((border->zone == zone) || (border->iconic) ||
	    (border->zone != zone && e_config->clientlist_include_all_zones))
	  borders = evas_list_append(borders, border);
     }

   dat = (Main_Data *)e_object_data_get(E_OBJECT(m));
   if (!dat) e_menu_title_set(m, _("Windows"));

   if (!borders)
     { 
	/* FIXME here we want nothing, but that crashes!!! */
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("(No Windows)"));
     }

   if (borders) 
     {
	/* Sort the borders */
	if (e_config->clientlist_sort_by == E_CLIENTLIST_SORT_ALPHA)
          borders = evas_list_sort(borders, evas_list_count(borders), 
                                   _e_int_menus_clients_sort_alpha_cb);

	if (e_config->clientlist_sort_by == E_CLIENTLIST_SORT_ZORDER)
	  borders = evas_list_sort(borders, evas_list_count(borders),
                                   _e_int_menus_clients_sort_z_order_cb);

	/* Group the borders */
        if (e_config->clientlist_group_by == E_CLIENTLIST_GROUP_DESK)
	  { 
             borders = evas_list_sort(borders, evas_list_count(borders), 
                                      _e_int_menus_clients_group_desk_cb);
             _e_int_menus_clients_add_by_desk(desk, borders, m);
	  }
        if (e_config->clientlist_group_by == E_CLIENTLIST_GROUP_CLASS) 
	  { 
	     borders = evas_list_sort(borders, evas_list_count(borders), 
                                      _e_int_menus_clients_group_class_cb); 
	     _e_int_menus_clients_add_by_class(borders, m);
	  }
	if (e_config->clientlist_group_by == E_CLIENTLIST_GROUP_NONE)
          _e_int_menus_clients_add_by_none(borders, m);
     }

   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Cleanup Windows"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/windows");
   e_menu_item_callback_set(mi, _e_int_menus_clients_cleanup_cb, zone);

   if (dat)
     {
	mi = e_menu_item_new(m);
	e_menu_item_separator_set(mi, 1);

	subm = e_int_menus_lost_clients_new();
	e_object_data_set(E_OBJECT(subm), e_object_data_get(E_OBJECT(m)));   
	dat->lost_clients = subm;
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Lost Windows"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/lost_windows");
	e_menu_item_submenu_set(mi, subm);
     }

   e_object_free_attach_func_set(E_OBJECT(m), _e_int_menus_clients_free_hook);
   e_object_data_set(E_OBJECT(m), borders);
}

static const char *
_e_int_menus_clients_title_abbrv(const char *title)
{
   int max_len;

   max_len = e_config->clientlist_max_caption_len;
   if ((e_config->clientlist_limit_caption_len) && (strlen(title) > max_len))
     {
	char *abbv;
	const char *left, *right;

	abbv = calloc(E_CLIENTLIST_MAX_CAPTION_LEN + 4, sizeof(char));
	left = title;
	right = title + (strlen(title) - (max_len / 2));

	strncpy(abbv, left, max_len / 2);
	strncat(abbv, "...", 3);
	strncat(abbv, right, max_len / 2);

	return abbv;
     }
   else
     return title;
}

static void
_e_int_menus_clients_item_create(E_Border *bd, E_Menu *m)
{
   E_Menu_Item *mi;
   const char *title;

   title = _e_int_menus_clients_title_abbrv(e_border_name_get(bd));
   mi = e_menu_item_new(m);
   e_menu_item_check_set(mi, 1);
   if ((title) && (title[0]))
     e_menu_item_label_set(mi, title);
   else
     e_menu_item_label_set(mi, _("No name!!"));
   /* ref the border as we implicitly unref it in the callback */
   e_object_ref(E_OBJECT(bd));
/*   e_object_breadcrumb_add(E_OBJECT(bd), "clients_menu");*/
   e_menu_item_callback_set(mi, _e_int_menus_clients_item_cb, bd);
   e_menu_item_realize_callback_set(mi, _e_int_menus_clients_icon_cb, bd);
   if (!bd->iconic) e_menu_item_toggle_set(mi, 1);
}

static void
_e_int_menus_clients_free_hook(void *obj)
{
   E_Menu *m;
   Evas_List *borders = NULL;

   m = obj;
   borders = e_object_data_get(E_OBJECT(m));
   while (borders)
     {
	E_Border *bd;

	bd = borders->data;
	borders = evas_list_remove_list(borders, borders);
	e_object_unref(E_OBJECT(bd));
     }
}

static void 
_e_int_menus_clients_item_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;

   bd = data;
   E_OBJECT_CHECK(bd);

   if (bd->iconic)
     {
        if (e_config->clientlist_warp_to_iconified_desktop == 1)
          e_desk_show(bd->desk);

	if (!bd->lock_user_iconify)
	  e_border_uniconify(bd);
     }

   if (!bd->iconic) e_desk_show(bd->desk);
   if (!bd->lock_user_stacking) e_border_raise(bd);
   if (!bd->lock_focus_out)
     {
	if (e_config->focus_policy != E_FOCUS_CLICK)
	  ecore_x_pointer_warp(bd->zone->container->win,
			       bd->x + (bd->w / 2), bd->y + (bd->h / 2));
	e_border_focus_set(bd, 1, 1);
     }
}

static void 
_e_int_menus_clients_icon_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd;
   Evas_Object *o;

   bd = data;
   E_OBJECT_CHECK(bd);

   o = e_icon_add(m->evas);
   e_icon_object_set(o, e_border_icon_add(bd, m->evas));
   mi->icon_object = o;
}

static void 
_e_int_menus_clients_cleanup_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Action *act;

   act = e_action_find("cleanup_windows");
   if (act) act->func.go(E_OBJECT(m->zone), NULL);
}

static void
_e_int_menus_lost_clients_pre_cb(void *data, E_Menu *m)
{
   E_Menu_Item *mi;
   Evas_List *l, *borders = NULL;
   E_Menu *root;
   E_Zone *zone = NULL;

   e_menu_pre_activate_callback_set(m, NULL, NULL);
   root = e_menu_root_get(m);
   /* get the current clients */
   if (root) zone = root->zone;
   borders = e_border_lost_windows_get(zone);

   if (!borders)
     { 
	/* FIXME here we want nothing, but that crashes!!! */
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("(No Windows)"));
	return;
     }
   for (l = borders; l; l = l->next)
     {
	E_Border *bd; 
	const char *title = "";

        bd = l->data;
	title = e_border_name_get(bd);
	mi = e_menu_item_new(m);
	if ((title) && (title[0]))
	  e_menu_item_label_set(mi, title);
	else
	  e_menu_item_label_set(mi, _("No name!!"));
	/* ref the border as we implicitly unref it in the callback */
	e_object_ref(E_OBJECT(bd));
	e_menu_item_callback_set(mi, _e_int_menus_lost_clients_item_cb, bd);
	if (bd->desktop) 
          e_util_desktop_menu_item_icon_add(bd->desktop, 24, mi);
     }
   e_object_free_attach_func_set(E_OBJECT(m), 
				 _e_int_menus_lost_clients_free_hook);
   e_object_data_set(E_OBJECT(m), borders);
}

static void
_e_int_menus_lost_clients_free_hook(void *obj)
{
   E_Menu *m;
   Evas_List *borders = NULL;

   m = obj;
   borders = e_object_data_get(E_OBJECT(m));
   while (borders)
     {
	E_Border *bd;

	bd = borders->data;
	borders = evas_list_remove_list(borders, borders);
	e_object_unref(E_OBJECT(bd));
     }
}

static void 
_e_int_menus_lost_clients_item_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Border *bd = data;

   E_OBJECT_CHECK(bd);
   if (bd->iconic) e_border_uniconify(bd);
   if (bd->desk) e_desk_show(bd->desk);
   e_border_move(bd, bd->zone->x + ((bd->zone->w - bd->w) / 2), 
		 bd->zone->y + ((bd->zone->h - bd->h) / 2));
   e_border_raise(bd);
   if (!bd->lock_focus_out)
     e_border_focus_set(bd, 1, 1);
}

static void
_e_int_menus_augmentation_add(E_Menu *m, Evas_List *augmentation)
{
   Evas_List *l = NULL;

   for (l = augmentation; l; l = l->next)
     {
	E_Int_Menu_Augmentation *aug;

	aug = l->data;
	if (aug->add.func) aug->add.func(aug->add.data, m);
     }
}

static void
_e_int_menus_augmentation_del(E_Menu *m, Evas_List *augmentation)
{
   Evas_List *l = NULL;

   for (l = augmentation; l; l = l->next)
     {
	E_Int_Menu_Augmentation *aug;

	aug = l->data;
	if (aug->del.func) aug->del.func(aug->del.data, m);
     }
}

static void 
_e_int_menus_shelves_pre_cb(void *data, E_Menu *m) 
{
   E_Menu_Item *mi;
   Evas_List *l, *shelves = NULL;
   E_Container *con;
   E_Zone *zone;

   e_menu_pre_activate_callback_set(m, NULL, NULL);
   con = e_container_current_get(e_manager_current_get());
   zone = e_zone_current_get(con);

   /* get the current clients */
   shelves = e_shelf_list();

   if (!shelves)
     { 
	/* FIXME here we want nothing, but that crashes!!! */
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("(No Shelves)"));
     }
   for (l = shelves; l; l = l->next)
     {
	E_Shelf *s;
	const char *name;
	char buf[4096];

	if (!(s = l->data)) continue;
	if (s->zone->num != zone->num) continue;
	if (s->cfg->container != con->num) continue;

	name = s->name;
	if (!name) name = _("Shelf #");	
	snprintf(buf, sizeof(buf), "%s %i", name, s->id);

	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, buf);
	e_menu_item_callback_set(mi, _e_int_menus_shelves_item_cb, s);
	switch (s->cfg->orient) 
	  {
	   case E_GADCON_ORIENT_LEFT:
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf_position_left");
	     break;
	   case E_GADCON_ORIENT_RIGHT:
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf_position_right");
	     break;
	   case E_GADCON_ORIENT_TOP:
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf_position_top");
	     break;
	   case E_GADCON_ORIENT_BOTTOM:
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf_position_bottom");
	     break;
	   case E_GADCON_ORIENT_CORNER_TL:
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf_position_top_left");
	     break;
	   case E_GADCON_ORIENT_CORNER_TR:
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf_position_top_right");
	     break;
	   case E_GADCON_ORIENT_CORNER_BL:
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf_position_bottom_left");
	     break;
	   case E_GADCON_ORIENT_CORNER_BR:
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf_position_bottom_right");
	     break;
	   case E_GADCON_ORIENT_CORNER_LT:
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf_position_left_top");
	     break;
	   case E_GADCON_ORIENT_CORNER_RT:
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf_position_right_top");
	     break;
	   case E_GADCON_ORIENT_CORNER_LB:
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf_position_left_bottom");
	     break;
	   case E_GADCON_ORIENT_CORNER_RB:
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf_position_right_bottom");
	     break;
	   default:
	     e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf");	     
	     break;
	  }
     }
   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Add A Shelf"));
   e_menu_item_callback_set(mi, _e_int_menus_shelves_add_cb, NULL);

   if (shelves)
     { 
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Delete A Shelf"));
	e_menu_item_callback_set(mi, _e_int_menus_shelves_del_cb, NULL);
     }	
}

static void 
_e_int_menus_shelves_item_cb(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   E_Shelf *s = data;

   E_OBJECT_CHECK(s);
   e_int_shelf_config(s);
}

static void 
_e_int_menus_shelves_add_cb(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   E_Container *con;
   E_Zone *zone;
   E_Config_Shelf *cs;

   con = e_container_current_get(e_manager_current_get());
   zone = e_zone_current_get(con);
   cs = E_NEW(E_Config_Shelf, 1);
   cs->name = evas_stringshare_add("shelf");
   cs->container = con->num;
   cs->zone = zone->num;
   cs->popup = 1;
   cs->layer = 200;
   cs->orient = E_GADCON_ORIENT_CORNER_BR;
   cs->fit_along = 1;
   cs->fit_size = 0;
   cs->style = evas_stringshare_add("default");
   cs->size = 40;
   cs->overlap = 0;
   e_config->shelves = evas_list_append(e_config->shelves, cs);
   e_config_save_queue();

   e_shelf_config_init();
}

static void 
_e_int_menus_shelves_del_cb(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   e_configure_registry_call("extensions/shelves", m->zone->container, NULL);
}

static void 
_e_int_menus_item_label_set(Efreet_Menu *entry, E_Menu_Item *mi) 
{
   Efreet_Desktop *desktop;
   char label[4096];
   int opt = 0;

   if ((!entry) || (!mi)) return;

   desktop = entry->desktop;
   if ((e_config->menu_eap_name_show) && (entry->name)) opt |= 0x4;
   if (desktop) 
     {
	if ((e_config->menu_eap_generic_show) && (desktop->generic_name) &&
	    (desktop->generic_name[0] != 0))
	  opt |= 0x2;
	if ((e_config->menu_eap_comment_show) && (desktop->comment) &&
	    (desktop->comment[0] != 0))
	  opt |= 0x1;
     }

   if (opt == 0x7) 
     snprintf(label, sizeof(label), "%s (%s) [%s]", entry->name, 
              desktop->generic_name, desktop->comment);
   else if (opt == 0x6) 
     snprintf(label, sizeof(label), "%s (%s)", entry->name, 
              desktop->generic_name);
   else if (opt == 0x5) 
     snprintf(label, sizeof(label), "%s [%s]", entry->name, desktop->comment);
   else if (opt == 0x4) 
     snprintf(label, sizeof(label), "%s", entry->name);
   else if (opt == 0x3) 
     snprintf(label, sizeof(label), "%s [%s]", desktop->generic_name, 
              desktop->comment);
   else if (opt == 0x2) 
     snprintf(label, sizeof(label), "%s", desktop->generic_name);
   else if (opt == 0x1) 
     snprintf(label, sizeof(label), "%s", desktop->comment);
   else
     snprintf(label, sizeof(label), "%s", entry->name);

   e_menu_item_label_set(mi, label);
}
