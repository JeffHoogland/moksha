#include "e.h"

typedef struct _About_Data About_Data;

struct _About_Data
{
   E_Menu *menu;
   E_Menu *modules;
};

/* local subsystem functions */
static void _e_int_menus_about_end     (void *data, E_Menu *m);
static void _e_int_menus_about_about   (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_about_restart (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_about_exit    (void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_int_menus_apps_scan     (E_Menu *m);
static void _e_int_menus_apps_start    (void *data, E_Menu *m);
static void _e_int_menus_apps_end      (void *data, E_Menu *m);
static void _e_int_menus_apps_free_hook(void *obj);
static void _e_int_menus_apps_run      (void *data, E_Menu *m, E_Menu_Item *mi);

/* externally accessible functions */
E_Menu *
e_int_menus_about_new(void)
{
   E_Menu *m, *subm;
   E_Menu_Item *mi;
   About_Data *dat;
   
   dat = calloc(1, sizeof(About_Data));
   m = e_menu_new();
   dat->menu = m;
   
   e_menu_post_deactivate_callback_set(m, _e_int_menus_about_end, dat);
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, "About Enlightenment...");   
   e_menu_item_icon_file_set(mi,
			     e_path_find(path_images, "e.png"));
   e_menu_item_callback_set(mi, _e_int_menus_about_about, NULL);
   
   subm = e_module_menu_new();
   dat->modules = subm;
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, "Modules");
   e_menu_item_submenu_set(mi, subm);
   
   mi = e_menu_item_new(m);
   e_menu_item_separator_set(mi, 1);
   
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, "Restart Enlightement");
   e_menu_item_callback_set(mi, _e_int_menus_about_restart, NULL);

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, "Exit Enlightement");
   e_menu_item_callback_set(mi, _e_int_menus_about_exit, NULL);
   return m;
}

E_Menu *
e_int_menus_apps_new(char *dir, int top)
{
   E_Menu *m;
   E_Menu_Item *mi;
   E_App *a;
   
   m = e_menu_new();
   a = e_app_new(dir, 0);
   e_object_data_set(E_OBJECT(m), a);
   e_menu_pre_activate_callback_set(m, _e_int_menus_apps_start, NULL);
   if (top)
     {
	e_menu_post_deactivate_callback_set(m, _e_int_menus_apps_end, NULL);
	e_object_free_attach_func_set(E_OBJECT(m), _e_int_menus_apps_free_hook);
     }
   return m;
}

/* local subsystem functions */
static void
_e_int_menus_about_end(void *data, E_Menu *m)
{
   About_Data *dat;
   
   dat = data;
   e_object_unref(E_OBJECT(dat->modules));
   e_object_unref(E_OBJECT(m));
   free(dat);
}

static void
_e_int_menus_about_about(void *data, E_Menu *m, E_Menu_Item *mi)
{
   e_error_dialog_show("About Enlightenment",
		       "This is Enlightenment "VERSION".\n"
		       "Copyright © 1999-2004, by the Enlightenment Dev Team.\n"
		       "\n"
		       "We hope you enjoy using this software as much as we enjoyed writing it.\n"
		       "Please think of the aardvarks. They need some love too."
		       );
}

static void
_e_int_menus_about_restart(void *data, E_Menu *m, E_Menu_Item *mi)
{
   printf("RESTART ON!\n");
   restart = 1;
   ecore_main_loop_quit();
}

static void
_e_int_menus_about_exit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   ecore_main_loop_quit();
}

static void
_e_int_menus_apps_scan(E_Menu *m)
{
   E_Menu_Item *mi;
   E_App *a;
   Evas_List *l;
   
   a = e_object_data_get(E_OBJECT(m));
   e_app_subdir_scan(a, 0);
   for (l = a->subapps; l; l = l->next)
     {
	a = l->data;
	
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, a->name);
	if (a->exe)
	  {
	     e_menu_item_icon_edje_set(mi, a->path, "icon");
	     e_menu_item_callback_set(mi, _e_int_menus_apps_run, a);
	  }
	else
	  {
	     char buf[4096];
	     
	     snprintf(buf, sizeof(buf), "%s/.directory.eet", a->path);
	     e_menu_item_icon_edje_set(mi, buf, "icon");
	     e_menu_item_submenu_set(mi, e_int_menus_apps_new(a->path, 0));
	  }
     }
}

static void
_e_int_menus_apps_start(void *data, E_Menu *m)
{
   _e_int_menus_apps_scan(m);
   e_menu_pre_activate_callback_set(m, NULL, NULL);
}

static void
_e_int_menus_apps_end(void *data, E_Menu *m)
{
   Evas_List *l;
   
   for (l = m->items; l; l = l->next)
     {
	E_Menu_Item *mi;
	
	mi = l->data;
	if (mi->submenu)
	  _e_int_menus_apps_end(NULL, mi->submenu);
     }
   e_object_unref(E_OBJECT(m));
}

static void
_e_int_menus_apps_free_hook(void *obj)
{
   E_Menu *m;
   E_App *a;
   
   m = obj;
   a = e_object_data_get(E_OBJECT(m));
   if (a) e_object_unref(E_OBJECT(a));
}

static void
_e_int_menus_apps_run(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_App *a;
   
   a = data;
   e_app_exec(a);
}
