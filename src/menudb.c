#include "e.h"

Evas_List build_menus = NULL;

static void e_build_menu_poll(int val, void *data);

static void
e_build_menu_poll(int val, void *data)
{
   time_t mod;
   E_Build_Menu *bm;
   
   bm = data;
   mod = e_file_modified_time(bm->file);
   if (mod <= bm->mod_time) 
     {
	e_add_event_timer(bm->file, 1.0, e_build_menu_poll, 0, data);
	return;
     }
   bm->mod_time = mod;
   
   e_build_menu_unbuild(bm);
   e_build_menu_build(bm);
   if (!bm->menu) bm->mod_time = 0;
   
   e_add_event_timer(bm->file, 1.0, e_build_menu_poll, 0, data);
   UN(val);
}

static void 
e_build_menu_cb_exec(E_Menu *m, E_Menu_Item *mi, void *data)
{
   char *exe;
   
   exe = data;
   e_exec_run(exe);
   UN(m);
   UN(mi);
}

void
e_build_menu_unbuild(E_Build_Menu *bm)
{
   Evas_List l;
   
   bm->menu = NULL;
   if (bm->menus)
     {
	for (l = bm->menus; l; l = l->next)
	  {
	     E_Menu *m;
	     
	     m = l->data;
	     e_menu_hide(m);
	     e_menu_update_shows(m);
	     e_menu_update_hides(m);
	     e_menu_update_finish(m);
	     OBJ_DO_FREE(m);
	  }
	bm->menus = evas_list_free(bm->menus);
     }
   if (bm->commands)
     {
	for (l = bm->commands; l; l = l->next)
	  {
	     IF_FREE(l->data);
	  }
	bm->commands = evas_list_free(bm->commands);
     }
}

E_Menu *
e_build_menu_build_number(E_Build_Menu *bm, E_DB_File *db, int num)
{
   E_Menu *menu;
   char    buf[4096];
   int     num2, i2;
   
   sprintf(buf, "/menu/%i/count", num);
   if (!e_db_int_get(db, buf, &num2)) return NULL;
   menu = e_menu_new();
   menu->pad.icon = 2;
   menu->pad.state = 2;
   for (i2 = 0; i2 < num2; i2++)
     {
	E_Menu_Item *menuitem;
	char        *text, *icon, *exe;
	int          ok, sub, sep;
	
	sprintf(buf, "/menu/%i/%i/text", num, i2);
	text = e_db_str_get(db, buf);
	sprintf(buf, "/menu/%i/%i/icon", num, i2);
	icon = e_db_str_get(db, buf);
	sprintf(buf, "/menu/%i/%i/command", num, i2);
	exe = e_db_str_get(db, buf);
	sprintf(buf, "/menu/%i/%i/submenu", num, i2);
	ok = e_db_int_get(db, buf, &sub);
	sep = 0;
	sprintf(buf, "/menu/%i/%i/separator", num, i2);
	e_db_int_get(db, buf, &sep);
	menuitem = e_menu_item_new(text);
	menuitem->icon = icon;
	if ((icon) && (text)) menuitem->scale_icon = 1;
	if (sep)
	  menuitem->separator = 1;
	else
	  {
	     if (ok)
	       {
		  E_Menu *menu2;
		  
		  menu2 = e_build_menu_build_number(bm, db, sub);
		  menuitem->submenu = menu2;
	       }
	  }
	if (exe)
	  {
	     e_menu_item_set_callback(menuitem, e_build_menu_cb_exec, exe);
	     bm->commands = evas_list_prepend(bm->commands, exe);
	  }
	e_menu_add_item(menu, menuitem);
     }
   bm->menus = evas_list_prepend(bm->menus, menu);
   return menu;
}

void
e_build_menu_build(E_Build_Menu *bm)
{
   E_DB_File *db;
   int num;
   
   e_db_flush();
   db = e_db_open_read(bm->file);
   if (!db) return;
   
   if (!e_db_int_get(db, "/menu/count", &num)) goto error;
   if (num > 0) bm->menu = e_build_menu_build_number(bm, db, 0);
   error:
   e_db_close(db);
}

void
e_build_menu_free(E_Build_Menu *bm)
{
   e_del_event_timer(bm->file);
   e_build_menu_unbuild(bm);
   IF_FREE(bm->file);
   build_menus = evas_list_remove(build_menus, bm);   
   FREE(bm);
}

E_Build_Menu *
e_build_menu_new_from_db(char *file)
{
   E_Build_Menu *bm;
   
   if (!file) return NULL;
   bm = NEW(E_Build_Menu, 1);
   ZERO(bm, E_Build_Menu, 1);
   OBJ_INIT(bm, e_build_menu_free);
   
   bm->file = strdup(file);
   
   build_menus = evas_list_prepend(build_menus, bm);   
   e_build_menu_poll(0, bm);   
   return bm;
}
