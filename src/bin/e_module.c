#include "e.h"

/* TODO List:
 *
 * * add module types/classes
 * * add list of exclusions that a module can't work withApi
 *
 */

/* local subsystem functions */
static void      _e_module_free(E_Module *m);
static void      _e_module_dialog_disable_show(const char *title, const char *body, E_Module *m);
static void      _e_module_cb_dialog_disable(void *data, E_Dialog *dia);
static void      _e_module_event_update_free(void *data, void *event);
static Eina_Bool _e_module_cb_idler(void *data);
static int       _e_module_sort_priority(const void *d1, const void *d2);
static void      _e_module_whitelist_check(void);

/* local subsystem globals */
static Eina_List *_e_modules = NULL;
static Ecore_Idle_Enterer *_e_module_idler = NULL;
static Eina_List *_e_modules_delayed = NULL;
static Eina_Bool _e_modules_initting = EINA_FALSE;

EAPI int E_EVENT_MODULE_UPDATE = 0;
EAPI int E_EVENT_MODULE_INIT_END = 0;

/* externally accessible functions */
EINTERN int
e_module_init(void)
{
   E_EVENT_MODULE_UPDATE = ecore_event_type_new();
   E_EVENT_MODULE_INIT_END = ecore_event_type_new();
   return 1;
}

EINTERN int
e_module_shutdown(void)
{
   E_Module *m;

#ifdef HAVE_VALGRIND
   /* do a leak check now before we dlclose() all those plugins, cause
    * that means we won't get a decent backtrace to leaks in there
    */
   VALGRIND_DO_LEAK_CHECK
#endif

   /* do not use EINA_LIST_FREE! e_object_del modifies list */
   if (x_fatal)
     e_module_save_all();
   else
     {
        while (_e_modules)
          {
             m = _e_modules->data;
             if ((m) && (m->enabled) && !(m->error))
               {
                  m->func.save(m);
                  m->func.shutdown(m);
                  m->enabled = 0;
               }
             e_object_del(E_OBJECT(m));
          }
     }

   return 1;
}

EAPI void
e_module_all_load(void)
{
   Eina_List *l;
   E_Config_Module *em;
   char buf[128];

   _e_modules_initting = EINA_TRUE;
   
   e_config->modules =
     eina_list_sort(e_config->modules, 0, _e_module_sort_priority);

   EINA_LIST_FOREACH(e_config->modules, l, em)
     {
        if (!em) continue;
        if ((em->delayed) && (em->enabled) && (!e_config->no_module_delay))
          {
             if (!_e_module_idler)
               _e_module_idler = ecore_idle_enterer_add(_e_module_cb_idler, NULL);
             _e_modules_delayed =
               eina_list_append(_e_modules_delayed,
                                eina_stringshare_add(em->name));
          }
        else if (em->enabled)
          {
             E_Module *m;

             if (!em->name) continue;

             e_util_env_set("E_MODULE_LOAD", em->name);
             snprintf(buf, sizeof(buf), _("Loading Module: %s"), em->name);
             e_init_status_set(buf);

             m = e_module_new(em->name);
             if (m) e_module_enable(m);
          }
     }

   if (!_e_modules_delayed)
     {
        ecore_event_add(E_EVENT_MODULE_INIT_END, NULL, NULL, NULL);
        _e_modules_initting = EINA_FALSE;
        _e_module_whitelist_check();
     }

   unsetenv("E_MODULE_LOAD");
}

EAPI Eina_Bool
e_module_loading_get(void)
{
   return _e_modules_initting;
}

EAPI E_Module *
e_module_new(const char *name)
{
   E_Module *m;
   char buf[PATH_MAX];
   char body[4096], title[1024];
   const char *modpath = NULL;
   char *s;
   Eina_List *l;
   E_Config_Module *em;
   int in_list = 0;

   if (!name) return NULL;
   m = E_OBJECT_ALLOC(E_Module, E_MODULE_TYPE, _e_module_free);
   if (name[0] != '/')
     {
        snprintf(buf, sizeof(buf), "%s/%s/module.so", name, MODULE_ARCH);
        modpath = e_path_find(path_modules, buf);
     }
   else if (eina_str_has_extension(name, ".so"))
     modpath = eina_stringshare_add(name);
   if (!modpath)
     {
        snprintf(body, sizeof(body),
                 _("There was an error loading the module named: %s<br>"
                   "No module named %s could be found in the<br>"
                   "module search directories.<br>"), name, buf);
        _e_module_dialog_disable_show(_("Error loading Module"), body, m);
        m->error = 1;
        goto init_done;
     }
   m->handle = dlopen(modpath, (RTLD_NOW | RTLD_GLOBAL));
   if (!m->handle)
     {
        snprintf(body, sizeof(body),
                 _("There was an error loading the module named: %s<br>"
                   "The full path to this module is:<br>"
                   "%s<br>"
                   "The error reported was:<br>"
                   "%s<br>"), name, buf, dlerror());
        _e_module_dialog_disable_show(_("Error loading Module"), body, m);
        m->error = 1;
        goto init_done;
     }
   m->api = dlsym(m->handle, "e_modapi");
   m->func.init = dlsym(m->handle, "e_modapi_init");
   m->func.shutdown = dlsym(m->handle, "e_modapi_shutdown");
   m->func.save = dlsym(m->handle, "e_modapi_save");

   if ((!m->func.init) || (!m->func.shutdown) || (!m->func.save) || (!m->api))
     {
        snprintf(body, sizeof(body),
                 _("There was an error loading the module named: %s<br>"
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
        snprintf(body, sizeof(body),
                 _("Module API Error<br>Error initializing Module: %s<br>"
                   "It requires a minimum module API version of: %i.<br>"
                   "The module API advertized by Enlightenment is: %i.<br>"),
                 _(m->api->name), m->api->version, E_MODULE_API_VERSION);

        snprintf(title, sizeof(title), _("Enlightenment %s Module"),
                 _(m->api->name));

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
        s = ecore_file_dir_get(modpath);
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
   EINA_LIST_FOREACH(e_config->modules, l, em)
     {
        if (!em) continue;
        if (em->name == m->name)
          {
             in_list = 1;
             break;
          }
     }
   if (!in_list)
     {
        E_Config_Module *module;

        module = E_NEW(E_Config_Module, 1);
        module->name = eina_stringshare_add(m->name);
        module->enabled = 0;
        e_config->modules = eina_list_append(e_config->modules, module);
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
   E_Config_Module *em;

   E_OBJECT_CHECK_RETURN(m, 0);
   E_OBJECT_TYPE_CHECK_RETURN(m, E_MODULE_TYPE, 0);
   if ((m->enabled) || (m->error)) return 0;
   m->data = m->func.init(m);
   if (m->data)
     {
        m->enabled = 1;
        EINA_LIST_FOREACH(e_config->modules, l, em)
          {
             if (!em) continue;
             if (!e_util_strcmp(em->name, m->name))
               {
                  em->enabled = 1;
                  e_config_save_queue();

                  ev = E_NEW(E_Event_Module_Update, 1);
                  ev->name = eina_stringshare_ref(em->name);
                  ev->enabled = 1;
                  ecore_event_add(E_EVENT_MODULE_UPDATE, ev,
                                  _e_module_event_update_free, NULL);
                  break;
               }
          }
        if (!_e_modules_initting)
          _e_module_whitelist_check();
        return 1;
     }
   return 0;
}

EAPI int
e_module_disable(E_Module *m)
{
   E_Event_Module_Update *ev;
   Eina_List *l;
   E_Config_Module *em;
   int ret;

   E_OBJECT_CHECK_RETURN(m, 0);
   E_OBJECT_TYPE_CHECK_RETURN(m, E_MODULE_TYPE, 0);
   if ((!m->enabled) || (m->error)) return 0;
   ret = m->func.shutdown(m);
   m->data = NULL;
   m->enabled = 0;
   EINA_LIST_FOREACH(e_config->modules, l, em)
     {
        if (!em) continue;
        if (!e_util_strcmp(em->name, m->name))
          {
             em->enabled = 0;
             e_config_save_queue();

             ev = E_NEW(E_Event_Module_Update, 1);
             ev->name = eina_stringshare_ref(em->name);
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
   E_Module *m;
   int ret = 1;

   EINA_LIST_FOREACH(_e_modules, l, m)
     e_object_ref(E_OBJECT(m));
   EINA_LIST_FOREACH(_e_modules, l, m)
     if ((m->enabled) && (!m->error))
       {
          if (!m->func.save(m)) ret = 0;
       }
   EINA_LIST_FOREACH(_e_modules, l, m)
     e_object_unref(E_OBJECT(m));
   return ret;
}

EAPI E_Module *
e_module_find(const char *name)
{
   Eina_List *l;
   E_Module *m;

   if (!name) return NULL;
   EINA_LIST_FOREACH(_e_modules, l, m)
     if (!e_util_strcmp(name, m->name)) return m;
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
   const char *icon = NULL;

   dia = e_dialog_new(e_container_current_get(e_manager_current_get()),
                      "E", "_module_dialog");
   if (!dia) return;

   e_dialog_title_set(dia, title);
   if (m)
     {
        Efreet_Desktop *desktop;

        snprintf(buf, sizeof(buf), "%s/module.desktop", e_module_dir_get(m));

        desktop = efreet_desktop_new(buf);
        if ((desktop) && (desktop->icon))
          {
             icon = efreet_icon_path_find(e_config->icon_theme, desktop->icon, 64);
             if (!icon)
               {
                  snprintf(buf, sizeof(buf), "%s/%s.edj",
                           e_module_dir_get(m), desktop->icon);
                  dia->icon_object = e_util_icon_add(buf, e_win_evas_get(dia->win));
               }
             else
               dia->icon_object = e_util_icon_add(icon, e_win_evas_get(dia->win));
             evas_object_size_hint_min_set(dia->icon_object, 64, 64);
             edje_object_part_swallow(dia->bg_object, "e.swallow.icon", dia->icon_object);
             evas_object_show(dia->icon_object);
          }
        if (desktop) efreet_desktop_free(desktop);
     }
   else
     e_dialog_icon_set(dia, "preferences-plugin", 64);

   e_dialog_text_set(dia, body);
   e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
   e_dialog_button_focus_num(dia, 0);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   if (!m) return;
   bd = dia->win->border;
   if (!bd) return;
   bd->internal_icon = eina_stringshare_add(icon);
}

EAPI void
e_module_delayed_set(E_Module *m, int delayed)
{
   Eina_List *l;
   E_Config_Module *em;

   EINA_LIST_FOREACH(e_config->modules, l, em)
     {
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
   E_Config_Module *em;

   EINA_LIST_FOREACH(e_config->modules, l, em)
     {
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
   E_Config_Module *em;
   Eina_List *l;

   EINA_LIST_FOREACH(e_config->modules, l, em)
     {
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
//   if (m->handle) dlclose(m->handle); DONT dlclose! causes problems with deferred callbacks for free etc. - when their code goes away!
   _e_modules = eina_list_remove(_e_modules, m);
   free(m);
}

static void
_e_module_dialog_disable_show(const char *title, const char *body, E_Module *m)
{
   E_Dialog *dia;
   char buf[4096];

   printf("MODULE ERR:\n%s\n", body);
   dia = e_dialog_new(NULL, "E", "_module_unload_dialog");
   EINA_SAFETY_ON_NULL_RETURN(dia);

   snprintf(buf, sizeof(buf), "%s<br>%s", body,
            _("What action should be taken with this module?<br>"));

   e_dialog_title_set(dia, title);
   e_dialog_icon_set(dia, "enlightenment", 64);
   e_dialog_text_set(dia, buf);
   e_dialog_button_add(dia, _("Unload"), NULL, _e_module_cb_dialog_disable, m);
   e_dialog_button_add(dia, _("Keep"), NULL, NULL, NULL);
   e_win_centered_set(dia->win, 1);
   dia->win->state.no_remember = 1;
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

static Eina_Bool
_e_module_cb_idler(void *data __UNUSED__)
{
   if (_e_modules_delayed)
     {
        const char *name;
        E_Module *m;

        name = eina_list_data_get(_e_modules_delayed);
        _e_modules_delayed =
          eina_list_remove_list(_e_modules_delayed, _e_modules_delayed);
        m = NULL;
        if (name) m = e_module_new(name);
        if (m)
          {
#ifndef E17_RELEASE_BUILD
             char buf[1024];
             snprintf(buf, sizeof(buf), "DELAYED MODULE LOAD: %s", name);
             e_main_ts(buf);
#endif
             e_module_enable(m);
          }
        eina_stringshare_del(name);
     }
   if (_e_modules_delayed)
     {
        e_util_wakeup();
        return ECORE_CALLBACK_RENEW;
     }

   ecore_event_add(E_EVENT_MODULE_INIT_END, NULL, NULL, NULL);
   
   _e_modules_initting = EINA_FALSE;
   _e_module_whitelist_check();

   _e_module_idler = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static int
_e_module_sort_priority(const void *d1, const void *d2)
{
   const E_Config_Module *m1, *m2;

   m1 = d1;
   m2 = d2;
   return m2->priority - m1->priority;
}

static void
_e_module_event_update_free(void *data __UNUSED__, void *event)
{
   E_Event_Module_Update *ev;

   if (!(ev = event)) return;
   eina_stringshare_del(ev->name);
   E_FREE(ev);
}

static void
_cleanup_cb(void *data, E_Dialog *dialog)
{
   Eina_List *badl = data;
   const char *s;

   e_object_del(E_OBJECT(dialog));
   EINA_LIST_FREE(badl, s)
     eina_stringshare_del(s);
}

static void
_ignore_cb(void *data, E_Dialog *dialog)
{
   const char *s;

   e_object_del(E_OBJECT(dialog));

   EINA_LIST_FREE(e_config->bad_modules, s)
     eina_stringshare_del(s);
   e_config->bad_modules = data;
   e_config_save_queue();
}

static void
_e_module_whitelist_check(void)
{
   Eina_List *l, *badl = NULL;
   E_Module *m;
   unsigned int known = 0;
   int i;
   const char *s;
   const char *goodmods[] = 
     {
        "access",
        "backlight",
        "battery",
        "bluez",
        "clock",
        "comp",
        "conf",
        "conf_applications",
        "conf_dialogs",
        "conf_display",
        "conf_edgebindings",
        "conf_interaction",
        "conf_intl",
        "conf_keybindings",
        "conf_menus",
        "conf_paths",
        "conf_performance",
        "conf_randr",
        "conf_shelves",
        "conf_theme",
        "conf_wallpaper2",
        "conf_window_manipulation",
        "conf_window_remembers",
        "connman",
        "cpufreq",
        "dropshadow",
        "everything",
        "fileman",
        "fileman_opinfo",
        "gadman",
        "ibar",
        "ibox",
        "illume2",
        "illume-bluetooth",
        "illume-home",
        "illume-home-toggle",
        "illume-indicator",
        "illume-kbd-toggle",
        "illume-keyboard",
        "illume-mode-toggle",
        "illume-softkey",
        "layout",
        "mixer",
        "msgbus",
        "notification",
        "ofono",
        "pager",
        "physics",
        "quickaccess",
        "shot",
        "start",
        "syscon",
        "systray",
        "tasks",
        "temperature",
        "tiling",
        "winlist",
        "wizard",
        "wl_drm",
        "wl_screenshot",
        "wl_shell",
        "xkbswitch",
        
        NULL // end marker
     };

   EINA_LIST_FOREACH(_e_modules, l, m)
     {
        Eina_Bool ok;
        
        if (!m->name) continue;
        ok = EINA_FALSE;
        for (i = 0; goodmods[i]; i++)
          {
             if (!strcmp(m->name, goodmods[i]))
               {
                  ok = EINA_TRUE;
                  break;
               }
          }
        if (!ok) badl = eina_list_append(badl, eina_stringshare_add(m->name));
     }

   EINA_LIST_FOREACH(badl, l, s)
     {
        const char *tmp;
        Eina_List *ll;
        Eina_Bool found = EINA_FALSE;

        EINA_LIST_FOREACH(e_config->bad_modules, ll, tmp)
          {
             if (!strcmp(s, tmp))
               {
                  found = EINA_TRUE;
                  break;
               }
          }

        if (!found) break;
        known++;
     }

   {
      Ecore_X_Atom _x_tainted;
      char *state;
      unsigned int _e_tainted;

      state = badl ? "YES" : "NO";
      _e_tainted = badl ? 1 : 0;

      _x_tainted = ecore_x_atom_get("_E_TAINTED");
      ecore_x_window_prop_card32_set(ecore_x_window_root_first_get(),
                                     _x_tainted, &_e_tainted, 1);

      e_env_set("E17_TAINTED", state);
   }

   if (eina_list_count(badl) != known)
     {
        E_Dialog *dia;
        Eina_Strbuf *sbuf;
        
        dia = e_dialog_new(e_container_current_get(e_manager_current_get()),
                           "E", "_module_whitelist_dialog");
        if (!dia)
          {
             eina_list_free(badl);
             return;
          }

        sbuf = eina_strbuf_new();
        if (!sbuf)
          {
             eina_list_free(badl);
             e_object_del(E_OBJECT(dia));
             return;
          }
        eina_strbuf_append
          (sbuf, _("The following modules are not standard ones for<br>"
                   "Enlightenment and may cause bugs and crashes.<br>"
                   "Please remove them before reporting any bugs.<br>"
                   "<br>"
                   "The module list is as follows:<br>"
                   "<br>"));
        EINA_LIST_FOREACH(badl, l, s)
          {
             eina_strbuf_append(sbuf, s);
             eina_strbuf_append(sbuf, "<br>");
          }

        e_dialog_title_set(dia, _("Unstable module tainting"));
        e_dialog_icon_set(dia, "enlightenment", 64);
        e_dialog_text_set(dia, eina_strbuf_string_get(sbuf));
        e_dialog_button_add(dia, _("OK"), NULL, _cleanup_cb, badl);
        e_dialog_button_add(dia, _("I know"), NULL, _ignore_cb, badl);
        e_win_centered_set(dia->win, 1);
        e_dialog_show(dia);
        eina_strbuf_free(sbuf);
     }
   else
     {
        EINA_LIST_FREE(badl, s)
          eina_stringshare_del(s);
     }
}
