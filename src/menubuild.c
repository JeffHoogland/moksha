#include "debug.h"
#include "menu.h"
#include "menubuild.h"
#include "exec.h"
#include "util.h"
#include "file.h"
#include "border.h"
#include "observer.h"

#ifdef USE_FERITE
# include "e_ferite.h"
#endif

Evas_List build_menus = NULL;

static void e_build_menu_cb_exec(E_Menu *m, E_Menu_Item *mi, void *data);
static void e_build_menu_cb_exec(E_Menu *m, E_Menu_Item *mi, void *data);

static void e_build_menu_unbuild(E_Build_Menu *bm);

static void e_build_menu_db_poll(int val, void *data);
static E_Menu *e_build_menu_db_build_number(E_Build_Menu *bm, E_DB_File *db, int num);
static void e_build_menu_db_build(E_Build_Menu *bm);

static void e_build_menu_gnome_apps_poll(int val, void *data);
static void e_build_menu_gnome_apps_build(E_Build_Menu *bm);

static E_Menu *e_build_menu_iconified_borders_build(E_Build_Menu *bm);
static void e_build_menu_iconified_borders_rebuild(E_Observer *observer, E_Observee *observee);


/* ------------ various callbacks ---------------------- */
static void 
e_build_menu_cb_exec(E_Menu *m, E_Menu_Item *mi, void *data)
{
   char *exe;

   D_ENTER;
   
   exe = data;
   e_exec_run(exe);

   D_RETURN;
   UN(m);
   UN(mi);
}

static void 
e_build_menu_cb_uniconify(E_Menu *m, E_Menu_Item *mi, void *data)
{
   E_Border *b;

   D_ENTER;
   
   b = data;
   e_border_uniconify(b);
   
   D_RETURN;
   UN(m);
   UN(mi);
}

static void 
e_build_menu_cb_script(E_Menu *m, E_Menu_Item *mi, void *data)
{
   char *script;

   D_ENTER;
   
#ifdef USE_FERITE
   script = data;
   e_ferite_run(script);
#else
   D("No cookies for you. You will have to install ferite.\n");
#endif
   
   D_RETURN;
   UN(m);
   UN(mi);
   UN(script);
   UN(data);
}

/*--------------------------------------------------------*/

static void
e_build_menu_unbuild(E_Build_Menu *bm)
{
   Evas_List l;
   
   D_ENTER;
   
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

	     e_object_unref(E_OBJECT(m));
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

   D_RETURN;
}


/* BUILDING from DB's */


static void
e_build_menu_db_poll(int val, void *data)
{
   time_t mod;
   E_Build_Menu *bm;
   
   D_ENTER;
   
   bm = data;
   mod = e_file_mod_time(bm->file);
   if (mod <= bm->mod_time) 
     {
	ecore_add_event_timer(bm->file, 1.0, e_build_menu_db_poll, 0, data);
	D_RETURN;
     }
   bm->mod_time = mod;
   
   e_build_menu_unbuild(bm);
   e_build_menu_db_build(bm);
   if (!bm->menu) bm->mod_time = 0;
   
   ecore_add_event_timer(bm->file, 1.0, e_build_menu_db_poll, 0, data);

   D_RETURN;
   UN(val);
}

static void
e_build_menu_gnome_apps_poll(int val, void *data)
{
   time_t mod;
   E_Build_Menu *bm;
   
   D_ENTER;
   
   bm = data;
   mod = e_file_mod_time(bm->file);
   if (mod <= bm->mod_time) 
     {
	ecore_add_event_timer(bm->file, 1.0, e_build_menu_gnome_apps_poll, 0, data);
	D_RETURN;
     }
   bm->mod_time = mod;
   
   e_build_menu_unbuild(bm);
   e_build_menu_gnome_apps_build(bm);
   if (!bm->menu) bm->mod_time = 0;
   
   ecore_add_event_timer(bm->file, 1.0, e_build_menu_gnome_apps_poll, 0, data);

   D_RETURN;
   UN(val);
}

static E_Menu *
e_build_menu_db_build_number(E_Build_Menu *bm, E_DB_File *db, int num)
{
   E_Menu *menu;
   char    buf[PATH_MAX];
   int     num2, i2;
   
   D_ENTER;
   
   sprintf(buf, "/menu/%i/count", num);
   if (!e_db_int_get(db, buf, &num2)) D_RETURN_(NULL);
   menu = e_menu_new();
   e_menu_set_padding_icon(menu, 2);
   e_menu_set_padding_state(menu, 2);
   for (i2 = 0; i2 < num2; i2++)
     {
	E_Menu_Item *menuitem;
	char        *text, *icon, *exe, *script;
	int          ok, sub, sep;
	
	sprintf(buf, "/menu/%i/%i/text", num, i2);
	text = e_db_str_get(db, buf);
	sprintf(buf, "/menu/%i/%i/icon", num, i2);
	icon = e_db_str_get(db, buf);
	sprintf(buf, "/menu/%i/%i/command", num, i2);
	exe = e_db_str_get(db, buf);
	sprintf(buf, "/menu/%i/%i/script", num, i2);
	script = e_db_str_get(db, buf);
	sprintf(buf, "/menu/%i/%i/submenu", num, i2);
	ok = e_db_int_get(db, buf, &sub);
	sep = 0;
	sprintf(buf, "/menu/%i/%i/separator", num, i2);
	e_db_int_get(db, buf, &sep);
	menuitem = e_menu_item_new(text);
	e_menu_item_set_icon(menuitem, icon);
	if ((icon) && (text)) e_menu_item_set_scale_icon(menuitem, 1);
	IF_FREE(text);
	IF_FREE(icon);
	if (sep) e_menu_item_set_separator(menuitem, 1);
	else
	  {
	     if (ok)
	       {
		  E_Menu *menu2;
		  
		  menu2 = e_build_menu_db_build_number(bm, db, sub);
		  e_menu_item_set_submenu(menuitem, menu2);
	       }
	  }
	if (exe)
	  {
	     e_menu_item_set_callback(menuitem, e_build_menu_cb_exec, exe);
	     bm->commands = evas_list_prepend(bm->commands, exe);
	  }
		if( script )
		{
		   e_menu_item_set_callback(menuitem, e_build_menu_cb_script, script);
		   bm->commands = evas_list_prepend(bm->commands, script);		   
		}
	e_menu_add_item(menu, menuitem);
     }
   bm->menus = evas_list_prepend(bm->menus, menu);

   D_RETURN_(menu);
}

static void
e_build_menu_db_build(E_Build_Menu *bm)
{
   E_DB_File *db;
   int num;
   
   D_ENTER;
   
   e_db_flush();
   db = e_db_open_read(bm->file);
   if (!db) D_RETURN;
   
   if (!e_db_int_get(db, "/menu/count", &num)) goto error;
   if (num > 0) bm->menu = e_build_menu_db_build_number(bm, db, 0);
   error:
   e_db_close(db);

   D_RETURN;
}


/* BUILD from GNOME APPS directory structure */


static E_Menu *
e_build_menu_gnome_apps_build_dir(E_Build_Menu *bm, char *dir)
{
   E_Menu *menu = NULL;
   Evas_List l, entries = NULL;
   
   D_ENTER;
   
   menu = e_menu_new();
   e_menu_set_padding_icon(menu, 2);
   e_menu_set_padding_state(menu, 2);
   /* build the order of things to scan ...*/
     {
	FILE *f;
	char buf[PATH_MAX];
	Evas_List dirlist = NULL;
	
	/* read .order file */
	sprintf(buf, "%s/.order", dir);
	f = fopen(buf, "rb");
	if (f)
	  {
	     while (fgets(buf, PATH_MAX, f))
	       {
		  int buf_len;
		  
		  buf_len = strlen(buf);
		  if (buf_len > 0)
		    {
		       if (buf[buf_len - 1] == '\n')
			   buf[buf_len - 1] = 0;
		       entries = evas_list_append(entries, strdup(buf));
		    }
	       }
	     fclose(f);
	  }
	/* read dir listing in alphabetical order and use that to suppliment */
	dirlist = e_file_ls(dir);
	for (l = dirlist; l; l = l->next)
	  {
	     char *s;
	     
	     s = l->data;
	     /* if it isnt a "dot" file or dir */
	     if (s[0] != '.')
	       {
		  Evas_List ll;
		  int have_it;
		  
		  have_it = 0;
		  for (ll = entries; ll; ll = ll->next)
		    {
		       if (!strcmp(ll->data, s))
			 {
			    have_it = 1;
			    break;
			 }
		    }
		  if (!have_it)
		    entries = evas_list_append(entries, strdup(s));
	       }
	     free(s);
	  }
	if (dirlist) evas_list_free(dirlist);
     }
   /* now go thru list... */
   for (l = entries; l; l = l->next)
     {
	char *s;
	char buf[PATH_MAX];
	E_Menu_Item *menuitem;
	char *icon, *name, *exe;
	E_Menu *sub;
	FILE *f;
	
	f = NULL;
	icon = NULL;
	exe = NULL;
	name = NULL;
	sub = NULL;
	s = l->data;
	sprintf(buf, "%s/%s", dir, s);
	/* if its a subdir... */
	if (e_file_is_dir(buf))
	  {
	     sub = e_build_menu_gnome_apps_build_dir(bm, buf);
	     sprintf(buf, "%s/%s/.directory", dir, s);
	     
	     f = fopen(buf, "rb");
	  }
	/* regular file */
	else if (e_file_exists(buf))
	  {
	     sprintf(buf, "%s/%s", dir, s);
	     
	     f = fopen(buf, "rb");
	  }
	/* doesnt exist at all? next item */
	else continue;
	if (f)
	  {
	     while (fgets(buf, PATH_MAX, f))
	       {
		  int buf_len;
		  
		  buf_len = strlen(buf);
		  if (buf_len > 0)
		    {
		       if (buf[buf_len - 1] == '\n')
			 buf[buf_len - 1] = 0;
		       /* look for Name= */
		       if ((!name) &&
			   (((e_util_glob_matches(buf, "Name[en]=*")) ||
			     (e_util_glob_matches(buf, "Name=*")))))
			 {
			    char *eq;
			    
			    eq = strchr(buf, '=');
			    if (eq)
			      name = strdup(eq + 1);
			 }
		       /* look for Icon= */
		       else if ((!icon) &&
				((e_util_glob_matches(buf, "Icon=*"))))
			 {
			    char *eq;
			    
			    eq = strchr(buf, '=');
			    if (eq)
			      {
				 char buf2[PATH_MAX];
				 
				 sprintf(buf2, "/usr/share/pixmaps/%s", eq +1);
				 icon = strdup(buf2);
			      }
			 }
		       /* look for Icon= */
		       else if ((!exe) &&
				((e_util_glob_matches(buf, "Exec=*"))))
			 {
			    char *eq;
			    
			    eq = strchr(buf, '=');
			    if (eq)
			      exe = strdup(eq + 1);
			 }
		    }		       
	       }
	     fclose(f);
	  }
	
	if (!name) name = strdup(s);
	menuitem = e_menu_item_new(name);
	if (icon) e_menu_item_set_icon(menuitem, icon);
        if ((icon) && (name)) e_menu_item_set_scale_icon(menuitem, 1);
	if (exe)
	  {
	     e_menu_item_set_callback(menuitem, e_build_menu_cb_exec, exe);
		  bm->commands = evas_list_prepend(bm->commands, exe);
	  }
	if (sub) e_menu_item_set_submenu(menuitem, sub);	     
	e_menu_add_item(menu, menuitem);
	
	IF_FREE(name);
	IF_FREE(icon);
	free(s);
     }
   if (entries) evas_list_free(entries);
   bm->menus = evas_list_prepend(bm->menus, menu);	

   D_RETURN_(menu);
}

static void
e_build_menu_gnome_apps_build(E_Build_Menu *bm)
{
   E_Menu *menu;
   
   D_ENTER;
   
   menu = e_build_menu_gnome_apps_build_dir(bm, bm->file);
   bm->menu = menu;

   D_RETURN;
}

static void
e_build_menu_cleanup(E_Build_Menu *bm)
{
   D_ENTER;
   
   ecore_del_event_timer(bm->file);
   e_build_menu_unbuild(bm);
   IF_FREE(bm->file);
   build_menus = evas_list_remove(build_menus, bm);   

   /* Call the destructor of the base class */
   e_object_cleanup(E_OBJECT(bm));

   D_RETURN;
}

E_Build_Menu *
e_build_menu_new_from_db(char *file)
{
   E_Build_Menu *bm;
   
   D_ENTER;
   
   if (!file) D_RETURN_(NULL);
   bm = NEW(E_Build_Menu, 1);
   ZERO(bm, E_Build_Menu, 1);

   e_object_init(E_OBJECT(bm), (E_Cleanup_Func) e_build_menu_cleanup);
   
   bm->file = strdup(file);
   
   build_menus = evas_list_prepend(build_menus, bm);   
   e_build_menu_db_poll(0, bm);   

   D_RETURN_(bm);
}

E_Build_Menu *
e_build_menu_new_from_gnome_apps(char *dir)
{
   E_Build_Menu *bm;
   
   D_ENTER;
   
   if (!dir) D_RETURN_(NULL);
   bm = NEW(E_Build_Menu, 1);
   ZERO(bm, E_Build_Menu, 1);

   e_object_init(E_OBJECT(bm), (E_Cleanup_Func) e_build_menu_cleanup);

   bm->file = strdup(dir);
   
   build_menus = evas_list_prepend(build_menus, bm);
   e_build_menu_gnome_apps_poll(0, bm);

   D_RETURN_(bm);
}

/*------------------------- iconified borders menu ----------------*/

E_Build_Menu *
e_build_menu_new_from_iconified_borders()
{
   E_Build_Menu *bm;
   Evas_List l;

   D_ENTER;

   bm = NEW(E_Build_Menu, 1);
   ZERO(bm, E_Build_Menu, 1);

   e_observer_init(E_OBSERVER(bm), E_EVENT_WINDOW_ICONIFY, e_build_menu_iconified_borders_rebuild, (E_Cleanup_Func) e_build_menu_cleanup);

   for (l = e_border_get_borders_list(); l; l = l->next)
   {
      E_Border *b = l->data;
      e_observer_register_observee(E_OBSERVER(bm), E_OBSERVEE(b));
   }
   bm->menu = e_build_menu_iconified_borders_build(bm);
   
   build_menus = evas_list_prepend(build_menus, bm);

   D_RETURN_(bm);
}

static void
e_build_menu_iconified_borders_rebuild(E_Observer *observer, E_Observee *observee)
{
   E_Build_Menu *bm;
   
   D_ENTER;
   D("catch iconify, rebuild menu");
   bm = (E_Build_Menu *)observer;
   
   e_build_menu_unbuild(bm);
   bm->menu = e_build_menu_iconified_borders_build(bm);
   D_RETURN;
}

static E_Menu *
e_build_menu_iconified_borders_build(E_Build_Menu *bm)
{
   E_Menu *menu = NULL;
   Evas_List l, entries = NULL;
   
   D_ENTER;
   
   menu = e_menu_new();
   e_menu_set_padding_icon(menu, 2);
   e_menu_set_padding_state(menu, 2);
  
   for (l = e_border_get_borders_list(); l; l = l->next)
   {
     E_Border *b;
     char *name = NULL;
     E_Menu_Item *menuitem;

     b = l->data;

     if (b->client.iconified)
     {
       e_strdup(name, b->client.title);
       D("adding menu item: %s\n", name);
       menuitem = e_menu_item_new(name);
       e_menu_item_set_callback(menuitem, e_build_menu_cb_uniconify, b);
       e_menu_add_item(menu, menuitem);

       IF_FREE(name);
     }  
   }
   bm->menus = evas_list_prepend(bm->menus, menu);	

   D_RETURN_(menu);
}
