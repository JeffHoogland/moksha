#include "e.h"
#include "evry_api.h"

/* "System" plugin: shutdown, reboot and log off.
 *
 * Every item is an action (subtype EVRY_TYPE_ACTION), so pressing Enter
 * runs it directly.  The items call the same Moksha actions ("halt",
 * "reboot", "logout") as the main menu does, so the usual confirmation
 * dialog is shown (unless the user disabled confirmation dialogs).
 */

typedef struct _Plugin    Plugin;
typedef struct _Sys_Def   Sys_Def;

struct _Plugin
{
   Evry_Plugin base;
   Eina_List  *all;
};

struct _Sys_Def
{
   const char   *label;
   const char   *detail;
   const char   *icon;
   const char   *action;   /* name of the registered e_action */
   E_Sys_Action  sys;      /* used to check if the action is possible */
};

static const Sys_Def _defs[] =
{
   { N_("Shutdown"), N_("Power off the computer"),  "system-shutdown",
     "halt",   E_SYS_HALT },
   { N_("Reboot"),   N_("Restart the computer"),    "system-reboot",
     "reboot", E_SYS_REBOOT },
   { N_("Logoff"),   N_("Log out of the session"),  "system-log-out",
     "logout", E_SYS_LOGOUT },
   { N_("Lock"),     N_("Lock the screen"),         "system-lock-screen",
     "desk_lock", E_SYS_NONE },
   { N_("Suspend"),  N_("Suspend to RAM"),          "system-suspend",
     "suspend", E_SYS_SUSPEND },
   { N_("Hibernate"), N_("Suspend to disk"),        "system-hibernate",
     "hibernate", E_SYS_HIBERNATE }
};

static const Evry_API *evry = NULL;
static Evry_Module *evry_module = NULL;
static Evry_Plugin *_plug = NULL;
static Evry_Action *_act = NULL;
static Evry_Type E_SYSTEM;
static char _module_icon[] = "system-shutdown";

static int
_sys_action(Evry_Action *act)
{
   E_Action *a;
   const char *name = NULL;

   if (act->it1.item) name = act->it1.item->data;

   if (!name) return EVRY_ACTION_FINISHED;

   a = e_action_find(name);
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);

   return EVRY_ACTION_FINISHED;
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *item __UNUSED__)
{
   Plugin *p;

   EVRY_PLUGIN_INSTANCE(p, plugin);

   return EVRY_PLUGIN(p);
}

static void
_finish(Evry_Plugin *plugin)
{
   Evry_Item *it;
   GET_PLUGIN(p, plugin);

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   EINA_LIST_FREE (p->all, it)
     EVRY_ITEM_FREE(it);

   E_FREE(p);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   GET_PLUGIN(p, plugin);

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   if (!p->all)
     {
        Evry_Item *it;
        unsigned int i;

        for (i = 0; i < sizeof(_defs) / sizeof(_defs[0]); i++)
          {
             const Sys_Def *d = &_defs[i];

             /* E_SYS_NONE: no capability check (e.g. screen lock) */
             if ((d->sys != E_SYS_NONE) &&
                 (!e_sys_action_possible_get(d->sys)))
               continue;

             it = EVRY_ITEM_NEW(Evry_Item, p, _(d->label), NULL, NULL);
             it->data = (void *)d->action;
             it->id = eina_stringshare_add(d->action);
             EVRY_ITEM_ICON_SET(it, d->icon);
             EVRY_ITEM_DETAIL_SET(it, _(d->detail));

             p->all = eina_list_append(p->all, it);
          }
     }

   return EVRY_PLUGIN_ITEMS_ADD(p, p->all, input, 1, 1);
}

static int
_plugins_init(const Evry_API *api)
{
   evry = api;

   if (!evry->api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   E_SYSTEM = evry->type_register("E_SYSTEM");

   _plug = EVRY_PLUGIN_BASE("System", _module_icon, E_SYSTEM,
                            _begin, _finish, _fetch);
   evry->plugin_register(_plug, EVRY_PLUGIN_SUBJECT, 12);

   /* items are plain E_SYSTEM items; this action (the default one for
    * them) performs the system operation */
   _act = EVRY_ACTION_NEW("Execute", E_SYSTEM, 0, "system-run",
                          _sys_action, NULL);
   evry->action_register(_act, 0);

   return EINA_TRUE;
}

static void
_plugins_shutdown(void)
{
   EVRY_PLUGIN_FREE(_plug);
   _plug = NULL;

   EVRY_ACTION_FREE(_act);
   _act = NULL;
}

/***************************************************************************/

Eina_Bool
evry_plug_system_init(E_Module *m)
{
   EVRY_MODULE_NEW(evry_module, evry, _plugins_init, _plugins_shutdown);

   e_module_delayed_set(m, 1);

   return EINA_TRUE;
}

void
evry_plug_system_shutdown(void)
{
   EVRY_MODULE_FREE(evry_module);
}

void
evry_plug_system_save(void){}
