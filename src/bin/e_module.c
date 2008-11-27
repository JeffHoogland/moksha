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

/* local subsystem functions */
static void _e_module_free(E_Module *m);
static void _e_module_dialog_disable_show(const char *title, const char *body, E_Module *m);
static void _e_module_cb_dialog_disable(void *data, E_Dialog *dia);
static void _e_module_event_update_free(void *data, void *event);
static int _e_module_cb_idler(void *data);
static int _e_module_sort_priority(const void *d1, const void *d2);

/* local subsystem globals */
static Eina_List *_e_modules = NULL;
static Ecore_Idler *_e_module_idler = NULL;
static Eina_List *_e_modules_delayed = NULL;

EAPI int E_EVENT_MODULE_UPDATE = 0;

/* externally accessible functions */
EAPI int
e_module_init(void)
{
   E_EVENT_MODULE_UPDATE = ecore_event_type_new();
   return 1;
}

EAPI int
e_module_shutdown(void)
{
   Eina_List *l;

#ifdef HAVE_VALGRIND
   /* do a leak check now before we dlclose() all those plugins, cause
    * that means we won't get a decent backtrace to leaks in there
    */
   VALGRIND_DO_LEAK_CHECK
#endif

   _e_modules = eina_list_reverse(_e_modules);
   for (l = _e_modules; l; l = l->next)
     {
	E_Module *m;
	
	m = l->data;
	if ((m->enabled) && (!m->error))
	  {
	     m->func.save(m);
	     m->func.shutdown(m);
	     m->enabled = 0;
	  }
     }
   l = _e_modules;
   _e_modules = NULL;
   while (l)
     {
	e_object_del(E_OBJECT(l->data));
	l = eina_list_remove_list(l, l);
     }
   return 1;
}

EAPI void
e_module_all_load(void)
{
   Eina_List *l;
   e_config->modules = eina_list_sort(e_config->modules,
                                      eina_list_count(e_config->modules),
                                      _e_module_sort_priority);
   for (l = e_config->modules; l; l = l->next)
     {
	E_Config_Module *em;
	E_Module *m;
	
	em = l->data;
	if (!em) continue;
	if ((em->delayed) && (em->enabled))
	  {
	     if (!_e_module_idler)
	       _e_module_idler = ecore_idler_add(_e_module_cb_idler, NULL);
	     _e_modules_delayed = 
	       eina_list_append(_e_modules_delayed,
				eina_stringshare_add(em->name));
	  }
	else if (em->enabled)
	  {
	     m = NULL;
	     if (em->name) m = e_module_new(em->name);
	     if (m) e_module_enable(m);
	  }
     }
}

EAPI E_Module *
e_module_new(const char *name)
{
   E_Module *m;
   char buf[4096];
   char body[4096], title[1024];
   const char *modpath;
   char *s;
   Eina_List *l;
   int in_list = 0;

   if (!name) return NULL;
   m = E_OBJECT_ALLOC(E_Module, E_MODULE_TYPE, _e_module_free);
   if (name[0] != '/')
     {
	snprintf(buf, sizeof(buf), "%s/%s/module.so", name, MODULE_ARCH);
	modpath = e_path_find(path_modules, buf);
     }
   else
     modpath = eina_stringshare_add(name);
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

   if ((!m->func.init) ||
       (!m->func.shutdown) ||
       (!m->func.save) ||
       (!m->api)
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
	dlclose(m->handle);
	m->handle = NULL;
	m->error = 1;
	goto init_done;
     }

init_done:

   _e_modules = eina_list_append(_e_modules, m);
   m->name = eina_stringshare_add(name);
   if (modpath)
     {
	s =  ecore_file_dir_get(modpath);
	if (s)
	  {
	     char *s2;
	     
	     s2 = ecore_file_dir_get(s);
	     free(s);
	     if (s2)
	       {
		  m->dir = eina_stringshare_add(s2);
		  free(s2);
	       }
	  }
     }
   for (l = e_config->modules; l; l = l->next)
     {
	E_Config_Module *em;
	
	em = l->data;
	if (!em) continue;
	if (!e_util_strcmp(em->name, m->name))
	  {
	     in_list = 1;
	     break;
	  }
     }
   if (!in_list)
     {
	E_Config_Module *em;
	
	em = E_NEW(E_Config_Module, 1);
	em->name = eina_stringshare_add(m->name);
	em->enabled = 0;
	e_config->modules = eina_list_append(e_config->modules, em);
	e_config_save_queue();
     }
   if (modpath) eina_stringshare_del(modpath);
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
   Eina_List *l;
   E_Event_Module_Update *ev;
   
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
	     if (!em) continue;
	     if (!e_util_strcmp(em->name, m->name))
	       {
		  em->enabled = 1;
		  e_config_save_queue();
		  
		  ev = E_NEW(E_Event_Module_Update, 1);
		  ev->name = strdup(em->name);
		  ev->enabled = 1;
		  ecore_event_add(E_EVENT_MODULE_UPDATE, ev, 
				  _e_module_event_update_free, NULL);
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
   E_Event_Module_Update *ev;
   Eina_List *l;
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
	if (!em) continue;
	if (!e_util_strcmp(em->name, m->name))
	  {
	     em->enabled = 0;
	     e_config_save_queue();
	     
	     ev = E_NEW(E_Event_Module_Update, 1);
	     ev->name = strdup(em->name);
	     ev->enabled = 0;
	     ecore_event_add(E_EVENT_MODULE_UPDATE, ev, 
			     _e_module_event_update_free, NULL);
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
   Eina_List *l;
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
   Eina_List *l;
   
   if (!name) return NULL;
   for (l = _e_modules; l; l = l->next)
     {
	E_Module *m;
	
	m = l->data;
	if (!e_util_strcmp(name, m->name)) return m;
     }
   return NULL;
}

EAPI Eina_List *
e_module_list(void)
{
   return _e_modules;
}

EAPI void
e_module_dialog_show(E_Module *m, const char *title, const char *body)
{
   E_Dialog *dia;
   E_Border *bd;
   char buf[PATH_MAX];
   char *icon = NULL;

   dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_module_dialog");
   if (!dia) return;

   e_dialog_title_set(dia, title);
   if (m)
     {
	Efreet_Desktop *desktop;

	snprintf(buf, sizeof(buf), "%s/module.desktop", e_module_dir_get(m));

	desktop = efreet_desktop_get(buf);
	if ((desktop) && (desktop->icon))
	  {
	     icon = efreet_icon_path_find(e_config->icon_theme, desktop->icon, 64);
	     if (!icon)
	       {
		  snprintf(buf, sizeof(buf), "%s/%s.edj",
			   e_module_dir_get(m), desktop->icon);
		  icon = strdup(buf);
	       }
	     dia->icon_object = e_util_icon_add(icon, e_win_evas_get(dia->win));
	     edje_extern_object_min_size_set(dia->icon_object, 64, 64);
	     edje_object_part_swallow(dia->bg_object, "e.swallow.icon", dia->icon_object);
	     evas_object_show(dia->icon_object);
	  }
	if (desktop) efreet_desktop_free(desktop);
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
   bd->internal_icon = eina_stringshare_add(icon);
   free(icon);
}

EAPI void
e_module_delayed_set(E_Module *m, int delayed)
{
   Eina_List *l;
   
   for (l = e_config->modules; l; l = l->next)
     {
        E_Config_Module *em;
	
	em = l->data;
	if (!em) continue;
	if (!e_util_strcmp(m->name, em->name))
	  {
	     if (em->delayed != delayed)
	       {
		  em->delayed = delayed;
		  e_config_save_queue();
	       }
	     break;
	  }
     }
}

EAPI void
e_module_priority_set(E_Module *m, int priority)
{
   /* Set the loading order for a module.
      More priority means load earlier */
   Eina_List *l;
   
   for (l = e_config->modules; l; l = l->next)
     {
	E_Config_Module *em;
	
	em = l->data;
	if (!em) continue;
	if (!e_util_strcmp(m->name, em->name))
	  {
	     if (em->priority != priority)
	       {
		  em->priority = priority;
		  e_config_save_queue();
	       }
	     break;
	  }
     }
}

/* local subsystem functions */

static void
_e_module_free(E_Module *m)
{
   Eina_List *l;
   
   for (l = e_config->modules; l; l = l->next)
     {
	E_Config_Module *em;
	
	em = l->data;
	if (!em) continue;
	if (!e_util_strcmp(em->name, m->name))
	  {
	     e_config->modules = eina_list_remove(e_config->modules, em);
	     if (em->name) eina_stringshare_del(em->name);
	     E_FREE(em);
	     break;
	  }
     }
   
   if ((m->enabled) && (!m->error))
     {
	m->func.save(m);
	m->func.shutdown(m);
     }
   if (m->name) eina_stringshare_del(m->name);
   if (m->dir) eina_stringshare_del(m->dir);
   if (m->handle) dlclose(m->handle);
   _e_modules = eina_list_remove(_e_modules, m);
   free(m);
}

static void
_e_module_dialog_disable_show(const char *title, const char *body, E_Module *m)
{
   E_Dialog *dia;
   char buf[4096];

   printf("MODULE ERR:\n%s\n", body);
   dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_module_unload_dialog");
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
_e_module_event_update_free(void *data, void *event) 
{
   E_Event_Module_Update *ev;
   
   ev = event;
   if (!ev) return;
   E_FREE(ev->name);
   E_FREE(ev);
}

static int
_e_module_cb_idler(void *data)
{
   if (_e_modules_delayed)
     {
	const char *name;
	E_Module *m;
	
	name = _e_modules_delayed->data;
	_e_modules_delayed = eina_list_remove_list(_e_modules_delayed, _e_modules_delayed);
	m = NULL;
	if (name) m = e_module_new(name);
	if (m) e_module_enable(m);
	eina_stringshare_del(name);
     }
   if (_e_modules_delayed) return 1;
   _e_module_idler = NULL;
   return 0;
}

static int
_e_module_sort_priority(const void *d1, const void *d2)
{
   const E_Config_Module *m1, *m2;

   m1 = d1;
   m2 = d2;
   return (m2->priority - m1->priority);
}
