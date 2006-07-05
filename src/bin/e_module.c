/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* TODO List:
 * 
 * * add module types/classes
 * * add list of exclusions that a module cant work withApi
 * 
 */

typedef struct _Module_Menu_Data Module_Menu_Data;

struct _Module_Menu_Data
{
   Evas_List *submenus;
};

/* local subsystem functions */
static void _e_module_free(E_Module *m);
static E_Menu *_e_module_control_menu_new(E_Module *mod);
static void _e_module_menu_free(void *obj);
static void _e_module_control_menu_about(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_module_control_menu_configuration(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_module_control_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi);
static void _e_module_dialog_disable_show(const char *title, const char *body, E_Module *m);
static void _e_module_cb_dialog_disable(void *data, E_Dialog *dia);
static void _e_module_dialog_icon_set(E_Dialog *dia, const char *icon);

/* local subsystem globals */
static Evas_List *_e_modules = NULL;

/* externally accessible functions */
EAPI int
e_module_init(void)
{
   Evas_List *pl = NULL, *l;
   
   for (l = e_config->modules; l;)
     {
	E_Config_Module *em;
	E_Module *m;
	
	em = l->data;
	pl = l;
	l = l->next;
	m = NULL;
	if (em->name) m = e_module_new(em->name);
	if (m)
	  {
	     if (em->enabled) e_module_enable(m);
	  }
	else
	  {
	     if (em->name) evas_stringshare_del(em->name);
	     E_FREE(em);
	     e_config->modules = evas_list_remove_list(e_config->modules, pl);
	     e_config_save_queue();
	  }
     }
   
   return 1;
}

EAPI int
e_module_shutdown(void)
{
   Evas_List *l, *tmp;

#ifdef HAVE_VALGRIND
   /* do a leak check now before we dlclose() all those plugins, cause
    * that means we won't get a decent backtrace to leaks in there
    */
   VALGRIND_DO_LEAK_CHECK
#endif

   for (l = _e_modules; l;)
     {
	tmp = l;
	l = l->next;
	e_object_del(E_OBJECT(tmp->data));
     }
   return 1;
}

EAPI E_Module *
e_module_new(const char *name)
{
   E_Module *m;
   char buf[4096];
   char body[4096], title[1024];
   const char *modpath;
   char *s;
   Evas_List *l;
   int in_list = 0;

   if (!name) return NULL;
   m = E_OBJECT_ALLOC(E_Module, E_MODULE_TYPE, _e_module_free);
   if (name[0] != '/')
     {
	snprintf(buf, sizeof(buf), "%s/%s/module.so", name, MODULE_ARCH);
	modpath = e_path_find(path_modules, buf);
     }
   else
     modpath = evas_stringshare_add(name);
   if (!modpath)
     {
	snprintf(body, sizeof(body), _("There was an error loading module named: %s<br>"
				       "No module named %s could be found in the<br>"
				       "module search directories.<br>"),
				     name, buf);
	_e_module_dialog_disable_show(_("Error loading Module"), body, m);
	m->error = 1;
	goto init_done;
     }
   m->handle = dlopen(modpath, RTLD_NOW | RTLD_GLOBAL);
   if (!m->handle)
     {
	snprintf(body, sizeof(body), _("There was an error loading module named: %s<br>"
				       "The full path to this module is:<br>"
				       "%s<br>"
				       "The error reported was:<br>"
				       "%s<br>"),
				     name, buf, dlerror());
	_e_module_dialog_disable_show(_("Error loading Module"), body, m);
	m->error = 1;
	goto init_done;
     }
   m->api = dlsym(m->handle, "e_modapi");
   m->func.init = dlsym(m->handle, "e_modapi_init");
   m->func.shutdown = dlsym(m->handle, "e_modapi_shutdown");
   m->func.save = dlsym(m->handle, "e_modapi_save");
   m->func.about = dlsym(m->handle, "e_modapi_about");
   m->func.config = dlsym(m->handle, "e_modapi_config");

   if ((!m->func.init) ||
       (!m->func.shutdown) ||
       (!m->func.save) ||
       (!m->func.about) ||
       (!m->api) ||
       
       // this is to more forcibly catch old/bad modules. will go - eventually,
       // but for now is a good check to have
       (dlsym(m->handle, "e_modapi_info"))
       
       )
     {
	snprintf(body, sizeof(body), _("There was an error loading module named: %s<br>"
				       "The full path to this module is:<br>"
				       "%s<br>"
				       "The error reported was:<br>"
				       "%s<br>"),
				     name, buf, _("Module does not contain all needed functions"));
	_e_module_dialog_disable_show(_("Error loading Module"), body, m);
	m->api = NULL;
	m->func.init = NULL;
	m->func.shutdown = NULL;
	m->func.save = NULL;
	m->func.about = NULL;
	m->func.config = NULL;

	dlclose(m->handle);
	m->handle = NULL;
	m->error = 1;
	goto init_done;
     }
   if (m->api->version < E_MODULE_API_VERSION)
     {
	snprintf(body, sizeof(body), _("Module API Error<br>Error initializing Module: %s<br>"
				       "It requires a minimum module API version of: %i.<br>"
				       "The module API advertized by Enlightenment is: %i.<br>"), 
				     _(m->api->name), m->api->version, E_MODULE_API_VERSION);

	snprintf(title, sizeof(title), _("Enlightenment %s Module"), _(m->api->name));

	_e_module_dialog_disable_show(title, body, m);
	m->api = NULL;
	m->func.init = NULL;
	m->func.shutdown = NULL;
	m->func.save = NULL;
	m->func.about = NULL;
	m->func.config = NULL;
	dlclose(m->handle);
	m->handle = NULL;
	m->error = 1;
	goto init_done;
     }

init_done:

   _e_modules = evas_list_append(_e_modules, m);
   m->name = evas_stringshare_add(name);
   if (modpath)
     {
	s =  ecore_file_get_dir(modpath);
	if (s)
	  {
	     char *s2;
	     
	     s2 = ecore_file_get_dir(s);
	     free(s);
	     if (s2)
	       {
		  m->dir = evas_stringshare_add(s2);
		  free(s2);
	       }
	  }
     }
   for (l = e_config->modules; l; l = l->next)
     {
	E_Config_Module *em;
	
	em = l->data;
	if (!strcmp(em->name, m->name))
	  {
	     in_list = 1;
	     break;
	  }
     }
   if (!in_list)
     {
	E_Config_Module *em;
	
	em = E_NEW(E_Config_Module, 1);
	em->name = evas_stringshare_add(m->name);
	em->enabled = 0;
	e_config->modules = evas_list_append(e_config->modules, em);
	e_config_save_queue();
     }
   if (modpath) evas_stringshare_del(modpath);
   return m;
}

EAPI int
e_module_save(E_Module *m)
{
   E_OBJECT_CHECK_RETURN(m, 0);
   E_OBJECT_TYPE_CHECK_RETURN(m, E_MODULE_TYPE, 0);
   if ((!m->enabled) || (m->error)) return 0;
   return m->func.save(m);
}

EAPI const char *
e_module_dir_get(E_Module *m)
{
   E_OBJECT_CHECK_RETURN(m, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(m, E_MODULE_TYPE, 0);
   return m->dir;
}

EAPI int
e_module_enable(E_Module *m)
{
   Evas_List *l;
   
   E_OBJECT_CHECK_RETURN(m, 0);
   E_OBJECT_TYPE_CHECK_RETURN(m, E_MODULE_TYPE, 0);
   if ((m->enabled) || (m->error)) return 0;
   m->data = m->func.init(m);
   if (m->data)
     {
	m->enabled = 1;
	for (l = e_config->modules; l; l = l->next)
	  {
	     E_Config_Module *em;
	     
	     em = l->data;
	     if (!e_util_strcmp(em->name, m->name))
	       {
		  em->enabled = 1;
		  e_config_save_queue();
		  break;
	       }
	  }
	return 1;
     }
   return 0;
}

EAPI int
e_module_disable(E_Module *m)
{
   Evas_List *l;
   int ret;
   
   E_OBJECT_CHECK_RETURN(m, 0);
   E_OBJECT_TYPE_CHECK_RETURN(m, E_MODULE_TYPE, 0);
   if ((!m->enabled) || (m->error)) return 0;
   ret = m->func.shutdown(m);
   m->data = NULL;
   m->enabled = 0;
   for (l = e_config->modules; l; l = l->next)
     {
	E_Config_Module *em;
	
	em = l->data;
	if (!e_util_strcmp(em->name, m->name))
	  {
	     em->enabled = 0;
	     e_config_save_queue();
	     break;
	  }
     }
   return ret;
}

EAPI int
e_module_enabled_get(E_Module *m)
{
   E_OBJECT_CHECK_RETURN(m, 0);
   E_OBJECT_TYPE_CHECK_RETURN(m, E_MODULE_TYPE, 0);
   return m->enabled;
}

EAPI int
e_module_save_all(void)
{
   Evas_List *l;
   int ret = 1;
   
   for (l = _e_modules; l; l = l->next) e_object_ref(E_OBJECT(l->data));
   for (l = _e_modules; l; l = l->next)
     {
	E_Module *m;
	
	m = l->data;
	if ((m->enabled) && (!m->error))
	  {
	     if (!m->func.save(m)) ret = 0;
	  }
     }
   for (l = _e_modules; l; l = l->next) e_object_unref(E_OBJECT(l->data));
   return ret;
}

EAPI E_Module *
e_module_find(const char *name)
{
   Evas_List *l;
   
   if (!name) return NULL;
   for (l = _e_modules; l; l = l->next)
     {
	E_Module *m;
	
	m = l->data;
	if (!strcmp(name, m->name)) return m;
     }
   return NULL;
}

EAPI Evas_List *
e_module_list(void)
{
   return _e_modules;
}

EAPI void
e_module_dialog_show(E_Module *m, const char *title, const char *body)
{
   E_Dialog *dia;
   E_Border *bd;
   char eap[4096];

   dia = e_dialog_new(e_container_current_get(e_manager_current_get()));
   if (!dia) return;

   e_dialog_title_set(dia, title);
   if (m) 
     {
	snprintf(eap, sizeof(eap), "%s/module.eap", e_module_dir_get(m));     
	_e_module_dialog_icon_set(dia, eap);
     }
   else
     e_dialog_icon_set(dia, "enlightenment/modules", 64);
   
   e_dialog_text_set(dia, body);
   e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
   e_dialog_button_focus_num(dia, 0);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   if (!m) return;
   bd = dia->win->border;
   if (!bd) return;
   bd->internal_icon = evas_stringshare_add(eap);
}

/* local subsystem functions */

static void
_e_module_free(E_Module *m)
{
   Evas_List *l;
   
   for (l = e_config->modules; l; l = l->next)
     {
	E_Config_Module *em;
	
	em = l->data;
	if (!strcmp(em->name, m->name))
	  {
	     e_config->modules = evas_list_remove(e_config->modules, em);
	     if (em->name) evas_stringshare_del(em->name);
	     E_FREE(em);
	     /* FIXME
	      * This is crap, a job is added, but doesn't run because
	      * main loop has quit!
	     e_config_save_queue();
	     */
	     break;
	  }
     }
   
   if ((m->enabled) && (!m->error))
     {
	m->func.save(m);
	m->func.shutdown(m);
     }
   if (m->name) evas_stringshare_del(m->name);
   if (m->dir) evas_stringshare_del(m->dir);
   if (m->handle) dlclose(m->handle);
   _e_modules = evas_list_remove(_e_modules, m);
   free(m);
}

static E_Menu *
_e_module_control_menu_new(E_Module *mod)
{
   E_Menu *m;
   E_Menu_Item *mi;

   if (mod->error) return NULL;
   
   m = e_menu_new();
   
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("About..."));
   e_menu_item_callback_set(mi, _e_module_control_menu_about, mod);
   
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Enabled"));
   e_menu_item_check_set(mi, 1);
   if (mod->enabled) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _e_module_control_menu_enabled, mod);

   if (mod->func.config)
     {
	mi = e_menu_item_new(m);
	e_menu_item_separator_set(mi, 1);
	
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Configuration"));
	e_menu_item_callback_set(mi, _e_module_control_menu_configuration, mod);
     }
   return m;
}

static void
_e_module_menu_free(void *obj)
{
   Module_Menu_Data *dat;
   
   dat = e_object_data_get(E_OBJECT(obj));
   while (dat->submenus)
     {
	E_Menu *subm;
	
	subm = dat->submenus->data;
	dat->submenus = evas_list_remove_list(dat->submenus, dat->submenus);
	e_object_del(E_OBJECT(subm));
     }
   free(dat);
}

static void
_e_module_control_menu_about(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Module *mod;
   
   mod = data;
   mod->func.about(mod);
}

static void
_e_module_control_menu_configuration(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Module *mod;
   
   mod = data;
   mod->func.config(mod);
}

static void
_e_module_control_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Module *mod;
   int enabled;
   
   mod = data;
   enabled = e_menu_item_toggle_get(mi);
   if ((mod->enabled) && (!enabled))
     {
	e_module_save(mod);
	e_module_disable(mod);
     }
   else if ((!mod->enabled) && (enabled))
     e_module_enable(mod);
   
   e_menu_item_toggle_set(mi, e_module_enabled_get(mod));
}

static void
_e_module_dialog_disable_show(const char *title, const char *body, E_Module *m)
{
   E_Dialog *dia;
   char buf[4096];

   dia = e_dialog_new(e_container_current_get(e_manager_current_get()));
   if (!dia) return;

   snprintf(buf, sizeof(buf), "%s<br>%s", body,
					  _("Would you like to unload this module?<br>"));

   e_dialog_title_set(dia, title);
   e_dialog_icon_set(dia, "enlightenment/e", 64);
   e_dialog_text_set(dia, buf);
   e_dialog_button_add(dia, _("Yes"), NULL, _e_module_cb_dialog_disable, m);
   e_dialog_button_add(dia, _("No"), NULL, NULL, NULL);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
}

static void
_e_module_cb_dialog_disable(void *data, E_Dialog *dia)
{
   E_Module *m;

   m = data;
   e_module_disable(m);
   e_object_del(E_OBJECT(m));
   e_object_del(E_OBJECT(dia));
   e_config_save_queue();
}

static void 
_e_module_dialog_icon_set(E_Dialog *dia, const char *icon) 
{
   /* These should never happen, but just in case */
   if (!dia) return;
   if (!icon) return;
   
   dia->icon_object = edje_object_add(e_win_evas_get(dia->win));
   edje_object_file_set(dia->icon_object, icon, "icon");
   edje_extern_object_min_size_set(dia->icon_object, 64, 64);
   edje_object_part_swallow(dia->bg_object, "icon_swallow", dia->icon_object);
   evas_object_show(dia->icon_object);
}
