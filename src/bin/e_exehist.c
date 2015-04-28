#include "e.h"
#include <libgen.h>

EAPI int E_EVENT_EXEHIST_UPDATE = 0;

/* local subsystem functions */
typedef struct _E_Exehist      E_Exehist;
typedef struct _E_Exehist_Item E_Exehist_Item;

struct _E_Exehist
{
   Eina_List *history;
   Eina_List *mimes;
};

struct _E_Exehist_Item
{
   const char  *exe;
   const char  *normalized_exe;
   const char  *launch_method;
   double       exetime;
   unsigned int count;
};

static void        _e_exehist_unload_queue(void);
static void        _e_exehist_load(void);
static void        _e_exehist_clear(void);
static void        _e_exehist_unload(void);
static void        _e_exehist_limit(void);
static const char *_e_exehist_normalize_exe(const char *exe);
static void        _e_exehist_cb_unload(void *data);
static int         _e_exehist_sort_exe_cb(const void *d1, const void *d2);
static int         _e_exehist_sort_pop_cb(const void *d1, const void *d2);

/* local subsystem globals */
static E_Config_DD *_e_exehist_config_edd = NULL;
static E_Config_DD *_e_exehist_config_item_edd = NULL;
static E_Exehist *_e_exehist = NULL;
static E_Powersave_Deferred_Action *_e_exehist_unload_defer = NULL;
static int _e_exehist_changes = 0;

/* externally accessible functions */
EINTERN int
e_exehist_init(void)
{
   _e_exehist_config_item_edd = E_CONFIG_DD_NEW("E_Exehist_Item", E_Exehist_Item);
#undef T
#undef D
#define T E_Exehist_Item
#define D _e_exehist_config_item_edd
   E_CONFIG_VAL(D, T, exe, STR);
   E_CONFIG_VAL(D, T, normalized_exe, STR);
   E_CONFIG_VAL(D, T, launch_method, STR);
   E_CONFIG_VAL(D, T, exetime, DOUBLE);

   _e_exehist_config_edd = E_CONFIG_DD_NEW("E_Exehist", E_Exehist);
#undef T
#undef D
#define T E_Exehist
#define D _e_exehist_config_edd
   E_CONFIG_LIST(D, T, history, _e_exehist_config_item_edd);
   E_CONFIG_LIST(D, T, mimes, _e_exehist_config_item_edd);

   E_EVENT_EXEHIST_UPDATE = ecore_event_type_new();

   return 1;
}

EINTERN int
e_exehist_shutdown(void)
{
   if (_e_exehist_unload_defer)
     {
        e_powersave_deferred_action_del(_e_exehist_unload_defer);
        _e_exehist_unload_defer = NULL;
     }
   _e_exehist_cb_unload(NULL);
   E_CONFIG_DD_FREE(_e_exehist_config_item_edd);
   E_CONFIG_DD_FREE(_e_exehist_config_edd);
   return 1;
}

EAPI void
e_exehist_add(const char *launch_method, const char *exe)
{
   E_Exehist_Item *ei;

   _e_exehist_load();
   if (!_e_exehist) return;
   ei = E_NEW(E_Exehist_Item, 1);
   if (!ei)
     {
        _e_exehist_unload_queue();
        return;
     }
   ei->launch_method = eina_stringshare_add(launch_method);
   ei->exe = eina_stringshare_add(exe);
   ei->normalized_exe = _e_exehist_normalize_exe(exe);
   ei->exetime = ecore_time_unix_get();
   _e_exehist->history = eina_list_append(_e_exehist->history, ei);
   _e_exehist_limit();
   _e_exehist_changes++;
   ecore_event_add(E_EVENT_EXEHIST_UPDATE, NULL, NULL, NULL);
   _e_exehist_unload_queue();
}

EAPI void
e_exehist_del(const char *exe)
{
   E_Exehist_Item *ei;
   Eina_List *l;
   Eina_Bool ok = EINA_FALSE;

   _e_exehist_load();
   if (!_e_exehist) return;
   EINA_LIST_FOREACH(_e_exehist->history, l, ei)
     {
        if ((ei->exe) && (!strcmp(exe, ei->exe)))
          {
             eina_stringshare_del(ei->exe);
             eina_stringshare_del(ei->normalized_exe);
             eina_stringshare_del(ei->launch_method);
             free(ei);
             _e_exehist->history = eina_list_remove_list(_e_exehist->history,
                                                         l);
             _e_exehist_changes++;
             _e_exehist_unload_queue();
             ok = EINA_TRUE;
          }
     }
   if (ok)
     ecore_event_add(E_EVENT_EXEHIST_UPDATE, NULL, NULL, NULL);
}

EAPI void
e_exehist_clear(void)
{
   _e_exehist_load();
   if (!_e_exehist) return;
   _e_exehist_clear();
   _e_exehist_changes++;
   ecore_event_add(E_EVENT_EXEHIST_UPDATE, NULL, NULL, NULL);
   _e_exehist_unload_queue();
}

EAPI int
e_exehist_popularity_get(const char *exe)
{
   Eina_List *l;
   E_Exehist_Item *ei;
   const char *normal;
   int count = 0;

   _e_exehist_load();
   if (!_e_exehist) return 0;
   normal = _e_exehist_normalize_exe(exe);
   if (!normal) return 0;
   EINA_LIST_FOREACH(_e_exehist->history, l, ei)
     {
        if ((ei->normalized_exe) && (!strcmp(normal, ei->normalized_exe)))
          count++;
     }
   eina_stringshare_del(normal);
   _e_exehist_unload_queue();
   return count;
}

EAPI double
e_exehist_newest_run_get(const char *exe)
{
   Eina_List *l;
   E_Exehist_Item *ei;
   const char *normal;

   _e_exehist_load();
   if (!_e_exehist) return 0.0;
   normal = _e_exehist_normalize_exe(exe);
   if (!normal) return 0.0;
   EINA_LIST_REVERSE_FOREACH(_e_exehist->history, l, ei)
     {
        if ((ei->normalized_exe) && (!strcmp(normal, ei->normalized_exe)))
          {
             eina_stringshare_del(normal);
             _e_exehist_unload_queue();
             return ei->exetime;
          }
     }
   eina_stringshare_del(normal);
   _e_exehist_unload_queue();
   return 0.0;
}

EAPI Eina_List *
e_exehist_list_get(void)
{
   return e_exehist_sorted_list_get(E_EXEHIST_SORT_BY_DATE, 0);
}

EAPI Eina_List *
e_exehist_sorted_list_get(E_Exehist_Sort sort_type, int max)
{
   Eina_List *list = NULL, *pop = NULL, *l = NULL, *m;
   Eina_Iterator *iter;
   E_Exehist_Item *ei;
   int count = 1;
   E_Exehist_Item *prev = NULL;

   if (!max) max = 20;
   _e_exehist_load();
   switch (sort_type)
     {
      case E_EXEHIST_SORT_BY_EXE:
      case E_EXEHIST_SORT_BY_POPULARITY:
        l = eina_list_clone(_e_exehist->history);
        l = eina_list_sort(l, 0, _e_exehist_sort_exe_cb);
        iter = eina_list_iterator_new(l);
        break;

      default:
        iter = eina_list_iterator_reversed_new(_e_exehist->history);
        break;
     }
   EINA_ITERATOR_FOREACH(iter, ei)
     {
        int bad = 0;

        if (!(ei->normalized_exe)) continue;
        if (sort_type == E_EXEHIST_SORT_BY_POPULARITY)
          {
             if (!prev || (strcmp(prev->normalized_exe, ei->normalized_exe)))
               {
                  prev = ei;
                  pop = eina_list_append(pop, ei);
               }
             prev->count++;
          }
        else
          {
             const char *exe;

             EINA_LIST_FOREACH(list, m, exe)
               {
                  if (!exe) continue;
                  if (!strcmp(exe, ei->exe))
                    {
                       bad = 1;
                       break;
                    }
               }
             if (!(bad))
               {
                  list = eina_list_append(list, ei->exe);
                  count++;
               }
          }
        if (count > max) break;
     }
   if (sort_type == E_EXEHIST_SORT_BY_POPULARITY)
     {
        count = 1;
        pop = eina_list_sort(pop, 0, _e_exehist_sort_pop_cb);
        EINA_LIST_FOREACH(pop, l, prev)
          {
             list = eina_list_append(list, prev->exe);
             count++;
             if (count > max) break;
          }
        eina_list_free(pop);
     }
   eina_list_free(l);
   eina_iterator_free(iter);
   _e_exehist_unload_queue();
   return list;
}

EAPI void
e_exehist_mime_desktop_add(const char *mime, Efreet_Desktop *desktop)
{
   const char *f;
   E_Exehist_Item *ei;
   Eina_List *l;
   char buf[PATH_MAX];
   Efreet_Ini *ini;

   if ((!mime) || (!desktop)) return;
   if (!desktop->orig_path) return;
   _e_exehist_load();
   if (!_e_exehist) return;

   f = efreet_util_path_to_file_id(desktop->orig_path);
   if (!f) return;

   snprintf(buf, sizeof(buf), "%s/applications/defaults.list",
            efreet_data_home_get());
   ini = efreet_ini_new(buf);
   //fprintf(stderr, "try open %s = %p\n", buf, ini);
   if (ini)
     {
        //fprintf(stderr, "SAVE mime %s with %s\n", mime, desktop->orig_path);
        if (!efreet_ini_section_set(ini, "Default Applications"))
          {
             efreet_ini_section_add(ini, "Default Applications");
             efreet_ini_section_set(ini, "Default Applications");
          }
        if (desktop->orig_path)
          efreet_ini_string_set(ini, mime, ecore_file_file_get(desktop->orig_path));
        efreet_ini_save(ini, buf);
        efreet_ini_free(ini);
     }

   EINA_LIST_FOREACH(_e_exehist->mimes, l, ei)
     {
        if ((ei->launch_method) && (!strcmp(mime, ei->launch_method)))
          {
             if ((ei->exe) && (!strcmp(f, ei->exe)))
               {
                  _e_exehist_unload_queue();
                  return;
               }
             if (ei->exe) eina_stringshare_del(ei->exe);
             if (ei->launch_method) eina_stringshare_del(ei->launch_method);
             free(ei);
             _e_exehist->mimes = eina_list_remove_list(_e_exehist->mimes, l);
             _e_exehist_changes++;
             break;
          }
     }
   ei = E_NEW(E_Exehist_Item, 1);
   if (!ei)
     {
        _e_exehist_unload_queue();
        return;
     }
   ei->launch_method = eina_stringshare_add(mime);
   ei->exe = eina_stringshare_add(f);
   ei->exetime = ecore_time_unix_get();
   _e_exehist->mimes = eina_list_append(_e_exehist->mimes, ei);
   _e_exehist_limit();
   _e_exehist_changes++;
   _e_exehist_unload_queue();
}

EAPI Efreet_Desktop *
e_exehist_mime_desktop_get(const char *mime)
{
   Efreet_Desktop *desktop;
   E_Exehist_Item *ei;
   Eina_List *l;

   //fprintf(stderr, "e_exehist_mime_desktop_get(%s)\n", mime);
   if (!mime) return NULL;
   _e_exehist_load();
   //fprintf(stderr, "x\n");
   if (!_e_exehist) return NULL;
   EINA_LIST_FOREACH(_e_exehist->mimes, l, ei)
     {
        //fprintf(stderr, "look for %s == %s\n", mime, ei->launch_method);
        if ((ei->launch_method) && (!strcmp(mime, ei->launch_method)))
          {
             desktop = NULL;
             if (ei->exe) desktop = efreet_util_desktop_file_id_find(ei->exe);
             //fprintf(stderr, "  desk = %p\n", desktop);
             if (desktop)
               {
                  _e_exehist_unload_queue();
                  return desktop;
               }
          }
     }
   _e_exehist_unload_queue();
   return NULL;
}

/* local subsystem functions */
static void
_e_exehist_unload_queue(void)
{
   if (_e_exehist_unload_defer)
     e_powersave_deferred_action_del(_e_exehist_unload_defer);
   _e_exehist_unload_defer =
     e_powersave_deferred_action_add(_e_exehist_cb_unload, NULL);
}

static void
_e_exehist_load(void)
{
   if (!_e_exehist)
     _e_exehist = e_config_domain_load("exehist", _e_exehist_config_edd);
   if (!_e_exehist)
     _e_exehist = E_NEW(E_Exehist, 1);
}

static void
_e_exehist_clear(void)
{
   if (_e_exehist)
     {
        E_Exehist_Item *ei;
        EINA_LIST_FREE(_e_exehist->history, ei)
          {
             eina_stringshare_del(ei->exe);
             eina_stringshare_del(ei->normalized_exe);
             eina_stringshare_del(ei->launch_method);
             free(ei);
          }
        EINA_LIST_FREE(_e_exehist->mimes, ei)
          {
             eina_stringshare_del(ei->exe);
             eina_stringshare_del(ei->launch_method);
             free(ei);
          }
     }
}

static void
_e_exehist_unload(void)
{
   _e_exehist_clear();
   E_FREE(_e_exehist);
}

static void
_e_exehist_limit(void)
{
   /* go from first item in hist on and either delete all items before a
    * specific timestamp, or if the list count > limit then delete items
    *
    * for now - limit to 500
    */
   if (_e_exehist)
     {
        while (eina_list_count(_e_exehist->history) > 500)
          {
             E_Exehist_Item *ei;

             ei = eina_list_data_get(_e_exehist->history);
             eina_stringshare_del(ei->exe);
             eina_stringshare_del(ei->normalized_exe);
             eina_stringshare_del(ei->launch_method);
             free(ei);
             _e_exehist->history = eina_list_remove_list(_e_exehist->history, _e_exehist->history);
          }
        while (eina_list_count(_e_exehist->mimes) > 500)
          {
             E_Exehist_Item *ei;

             ei = eina_list_data_get(_e_exehist->mimes);
             eina_stringshare_del(ei->exe);
             eina_stringshare_del(ei->launch_method);
             free(ei);
             _e_exehist->mimes = eina_list_remove_list(_e_exehist->mimes, _e_exehist->mimes);
          }
     }
}

static const char *
_e_exehist_normalize_exe(const char *exe)
{
   char *base, *buf, *cp, *space = NULL;
   const char *ret;
   Eina_Bool flag = EINA_FALSE;

   buf = strdup(exe);
   base = basename(buf);
   if ((base[0] == '.') && (base[1] == '\0'))
     {
        free(buf);
        return NULL;
     }

   cp = base;
   while (*cp)
     {
        if (isspace(*cp))
          {
             if (!space) space = cp;
             if (flag) flag = EINA_FALSE;
          }
        else if (!flag)
          {
             /* usually a variable in the desktop exe field */
             if (space && *cp == '%')
               flag = EINA_TRUE;
             else
               {
                  char lower = tolower(*cp);

                  space = NULL;
                  if (lower != *cp) *cp = lower;
               }
          }
        cp++;
     }

   if (space) *space = '\0';

   ret = eina_stringshare_add(base);
   free(buf);

   return ret;
}

static void
_e_exehist_cb_unload(void *data __UNUSED__)
{
   if (_e_exehist_changes)
     {
        e_config_domain_save("exehist", _e_exehist_config_edd, _e_exehist);
        _e_exehist_changes = 0;
     }
   _e_exehist_unload();
   _e_exehist_unload_defer = NULL;
}

static int
_e_exehist_sort_exe_cb(const void *d1, const void *d2)
{
   const E_Exehist_Item *ei1, *ei2;

   ei1 = d1;
   ei2 = d2;

   if ((!ei1) || (!ei1->normalized_exe)) return 1;
   if ((!ei2) || (!ei2->normalized_exe)) return -1;

   return strcmp(ei1->normalized_exe, ei2->normalized_exe);
}

static int
_e_exehist_sort_pop_cb(const void *d1, const void *d2)
{
   const E_Exehist_Item *ei1, *ei2;

   if (!(ei1 = d1)) return 1;
   if (!(ei2 = d2)) return -1;

   return ei2->count - ei1->count;
}

