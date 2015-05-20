#include "e.h"

#define REMEMBER_HIERARCHY 1
#define REMEMBER_SIMPLE    0

EAPI int E_EVENT_REMEMBER_UPDATE = -1;

typedef struct _E_Remember_List E_Remember_List;

struct _E_Remember_List
{
   Eina_List *list;
};

/* local subsystem functions */
static void        _e_remember_free(E_Remember *rem);
static void        _e_remember_update(E_Border *bd, E_Remember *rem);
static E_Remember *_e_remember_find(E_Border *bd, int check_usable);
static void        _e_remember_cb_hook_pre_post_fetch(void *data, void *bd);
static void        _e_remember_cb_hook_eval_post_new_border(void *data, void *bd);
static void        _e_remember_init_edd(void);
static Eina_Bool   _e_remember_restore_cb(void *data, int type, void *event);

/* local subsystem globals */
static Eina_List *hooks = NULL;
static E_Config_DD *e_remember_edd = NULL;
static E_Config_DD *e_remember_list_edd = NULL;
static E_Remember_List *remembers = NULL;
static Eina_List *handlers = NULL;
static Ecore_Idler *remember_idler = NULL;
static Eina_List *remember_idler_list = NULL;

/* static Eina_List *e_remember_restart_list = NULL; */

/* externally accessible functions */
EINTERN int
e_remember_init(E_Startup_Mode mode)
{
   Eina_List *l = NULL;
   E_Remember *rem;
   E_Border_Hook *h;

   if (mode == E_STARTUP_START)
     {
        EINA_LIST_FOREACH(e_config->remembers, l, rem)
          {
             if ((rem->apply & E_REMEMBER_APPLY_RUN) && (rem->prop.command))
               e_util_head_exec(rem->prop.head, rem->prop.command);
          }
     }
   E_EVENT_REMEMBER_UPDATE = ecore_event_type_new();

   h = e_border_hook_add(E_BORDER_HOOK_EVAL_PRE_POST_FETCH,
                         _e_remember_cb_hook_pre_post_fetch, NULL);
   if (h) hooks = eina_list_append(hooks, h);
   h = e_border_hook_add(E_BORDER_HOOK_EVAL_POST_NEW_BORDER,
                         _e_remember_cb_hook_eval_post_new_border, NULL);
   if (h) hooks = eina_list_append(hooks, h);

   _e_remember_init_edd();
   remembers = e_config_domain_load("e_remember_restart", e_remember_list_edd);

   if (remembers)
     {
        handlers = eina_list_append
            (handlers, ecore_event_handler_add
              (E_EVENT_MODULE_INIT_END, _e_remember_restore_cb, NULL));
     }

   return 1;
}

EINTERN int
e_remember_shutdown(void)
{
   E_FREE_LIST(hooks, e_border_hook_del);

   E_CONFIG_DD_FREE(e_remember_edd);
   E_CONFIG_DD_FREE(e_remember_list_edd);

   E_FREE_LIST(handlers, ecore_event_handler_del);
   if (remember_idler) ecore_idler_del(remember_idler);
   remember_idler = NULL;
   remember_idler_list = eina_list_free(remember_idler_list);

   return 1;
}

EAPI void
e_remember_internal_save(void)
{
   Eina_List *l;
   E_Border *bd;
   E_Remember *rem;

   //printf("internal save %d\n", restart);
   if (!remembers)
     remembers = E_NEW(E_Remember_List, 1);
   else
     {
        EINA_LIST_FREE(remembers->list, rem)
          _e_remember_free(rem);
     }

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if (!bd->internal) continue;

        rem = E_NEW(E_Remember, 1);
        if (!rem) break;

        e_remember_default_match_set(rem, bd);
        rem->apply = (E_REMEMBER_APPLY_POS | E_REMEMBER_APPLY_SIZE |
                      E_REMEMBER_APPLY_BORDER | E_REMEMBER_APPLY_LAYER |
                      E_REMEMBER_APPLY_SHADE | E_REMEMBER_APPLY_ZONE |
                      E_REMEMBER_APPLY_DESKTOP | E_REMEMBER_APPLY_LOCKS |
                      E_REMEMBER_APPLY_SKIP_WINLIST |
                      E_REMEMBER_APPLY_SKIP_PAGER |
                      E_REMEMBER_APPLY_SKIP_TASKBAR |
                      E_REMEMBER_APPLY_OFFER_RESISTANCE);
        _e_remember_update(bd, rem);

        remembers->list = eina_list_append(remembers->list, rem);
     }

   e_config_domain_save("e_remember_restart", e_remember_list_edd, remembers);
}

static Eina_Bool
_e_remember_restore_idler_cb(void *d __UNUSED__)
{
   E_Remember *rem;
   E_Action *act_fm = NULL, *act;
   E_Container *con;
   Eina_Bool done = EINA_FALSE;

   con = e_container_current_get(e_manager_current_get());

   EINA_LIST_FREE(remember_idler_list, rem)
     {
        if (!rem->class) continue;
        if (rem->no_reopen) continue;
        if (done) break;

        if (!strncmp(rem->class, "e_fwin::", 8))
          {
             if (!act_fm)
               act_fm = e_action_find("fileman");
             if (!act_fm) continue;
             /* at least '/' */
             if (!rem->class[9]) continue;

             act_fm->func.go(NULL, rem->class + 8);
          }
        else if (!strncmp(rem->class, "_config::", 9))
          {
             char *param = NULL;
             char path[1024];
             const char *p;

             p = rem->class + 9;
             if ((param = strstr(p, "::")))
               {
                  snprintf(path, (param - p) + sizeof(char), "%s", p);
                  param = param + 2;
               }
             else
               snprintf(path, sizeof(path), "%s", p);

             if (e_configure_registry_exists(path))
               {
                  e_configure_registry_call(path, con, param);
               }
          }
        else if (!strcmp(rem->class, "_configure"))
          {
             /* TODO this is just for settings panel. it could also
                use e_configure_registry_item_add */
             /* ..or make a general path for window that are started
                by actions */

             act = e_action_find("configuration");
             if (act)
               act->func.go(NULL, NULL);
          }
        done = EINA_TRUE;
     }
   if (!done) remember_idler = NULL;

   return done;
}

static Eina_Bool
_e_remember_restore_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   handlers = eina_list_free(handlers);
   if (!remembers->list) return ECORE_CALLBACK_PASS_ON;
   remember_idler_list = eina_list_clone(remembers->list);
   remember_idler = ecore_idler_add(_e_remember_restore_idler_cb, NULL);
   return ECORE_CALLBACK_PASS_ON;
}

EAPI E_Remember *
e_remember_new(void)
{
   E_Remember *rem;

   rem = E_NEW(E_Remember, 1);
   if (!rem) return NULL;
   e_config->remembers = eina_list_prepend(e_config->remembers, rem);
   return rem;
}

EAPI int
e_remember_usable_get(E_Remember *rem)
{
   if ((rem->apply_first_only) && (rem->used_count > 0)) return 0;
   return 1;
}

EAPI void
e_remember_use(E_Remember *rem)
{
   rem->used_count++;
}

EAPI void
e_remember_unuse(E_Remember *rem)
{
   rem->used_count--;
}

EAPI void
e_remember_del(E_Remember *rem)
{
   Eina_List *l = NULL;
   E_Border *bd;

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if (bd->remember != rem) continue;

        bd->remember = NULL;
        e_remember_unuse(rem);
     }

   _e_remember_free(rem);
}

EAPI E_Remember *
e_remember_find_usable(E_Border *bd)
{
   E_Remember *rem;

   rem = _e_remember_find(bd, 1);
   return rem;
}

EAPI E_Remember *
e_remember_find(E_Border *bd)
{
   E_Remember *rem;

   rem = _e_remember_find(bd, 0);
   return rem;
}

EAPI void
e_remember_match_update(E_Remember *rem)
{
   int max_count = 0;

   if (rem->match & E_REMEMBER_MATCH_NAME) max_count += 2;
   if (rem->match & E_REMEMBER_MATCH_CLASS) max_count += 2;
   if (rem->match & E_REMEMBER_MATCH_TITLE) max_count += 2;
   if (rem->match & E_REMEMBER_MATCH_ROLE) max_count += 2;
   if (rem->match & E_REMEMBER_MATCH_TYPE) max_count += 2;
   if (rem->match & E_REMEMBER_MATCH_TRANSIENT) max_count += 2;
   if (rem->apply_first_only) max_count++;

   if (max_count != rem->max_score)
     {
        /* The number of matches for this remember has changed so we
         * need to remove from list and insert back into the appropriate
         * loction. */
        Eina_List *l = NULL;
        E_Remember *r;

        rem->max_score = max_count;
        e_config->remembers = eina_list_remove(e_config->remembers, rem);

        EINA_LIST_FOREACH(e_config->remembers, l, r)
          {
             if (r->max_score <= rem->max_score) break;
          }

        if (l)
          e_config->remembers = eina_list_prepend_relative_list(e_config->remembers, rem, l);
        else
          e_config->remembers = eina_list_append(e_config->remembers, rem);
     }
}

EAPI int
e_remember_default_match_set(E_Remember *rem, E_Border *bd)
{
   const char *title, *clasz, *name, *role;
   int match;

   if (rem->name) eina_stringshare_del(rem->name);
   if (rem->class) eina_stringshare_del(rem->class);
   if (rem->title) eina_stringshare_del(rem->title);
   if (rem->role) eina_stringshare_del(rem->role);
   rem->name = NULL;
   rem->class = NULL;
   rem->title = NULL;
   rem->role = NULL;

   name = bd->client.icccm.name;
   if (!name || name[0] == 0) name = NULL;
   clasz = bd->client.icccm.class;
   if (!clasz || clasz[0] == 0) clasz = NULL;
   role = bd->client.icccm.window_role;
   if (!role || role[0] == 0) role = NULL;

   match = E_REMEMBER_MATCH_TRANSIENT;
   if (bd->client.icccm.transient_for != 0)
     rem->transient = 1;
   else
     rem->transient = 0;

   if (name && clasz)
     {
        match |= E_REMEMBER_MATCH_NAME | E_REMEMBER_MATCH_CLASS;
        rem->name = eina_stringshare_add(name);
        rem->class = eina_stringshare_add(clasz);
     }
   else if ((title = e_border_name_get(bd)) && title[0])
     {
        match |= E_REMEMBER_MATCH_TITLE;
        rem->title = eina_stringshare_add(title);
     }
   if (role)
     {
        match |= E_REMEMBER_MATCH_ROLE;
        rem->role = eina_stringshare_add(role);
     }
   if (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_UNKNOWN)
     {
        match |= E_REMEMBER_MATCH_TYPE;
        rem->type = bd->client.netwm.type;
     }

   rem->match = match;

   return match;
}

EAPI void
e_remember_update(E_Border *bd)
{
   if (bd->new_client) return;
   if (!bd->remember) return;
   if (bd->remember->keep_settings) return;
   _e_remember_update(bd, bd->remember);
   e_config_save_queue();
}

static void
_e_remember_update(E_Border *bd, E_Remember *rem)
{
   if (rem->apply & E_REMEMBER_APPLY_POS ||
       rem->apply & E_REMEMBER_APPLY_SIZE)
     {
        if (bd->fullscreen || bd->maximized)
          {
             rem->prop.pos_x = bd->saved.x;
             rem->prop.pos_y = bd->saved.y;
             rem->prop.pos_w = bd->saved.w;
             rem->prop.pos_h = bd->saved.h;
          }
        else
          {
             rem->prop.pos_x = bd->x - bd->zone->x;
             rem->prop.pos_y = bd->y - bd->zone->y;
             rem->prop.res_x = bd->zone->w;
             rem->prop.res_y = bd->zone->h;
             rem->prop.pos_w = bd->client.w;
             rem->prop.pos_h = bd->client.h;
             rem->prop.w = bd->client.w;
             rem->prop.h = bd->client.h;
          }
        if (bd->internal)
          rem->prop.maximize = bd->maximized & E_MAXIMIZE_DIRECTION;
        else
          rem->prop.maximize = 0;
     }
   if (rem->apply & E_REMEMBER_APPLY_LAYER)
     {
        if (bd->fullscreen)
          rem->prop.layer = bd->saved.layer;
        else
          rem->prop.layer = bd->layer;
     }
   if (rem->apply & E_REMEMBER_APPLY_LOCKS)
     {
        rem->prop.lock_user_location = bd->lock_user_location;
        rem->prop.lock_client_location = bd->lock_client_location;
        rem->prop.lock_user_size = bd->lock_user_size;
        rem->prop.lock_client_size = bd->lock_client_size;
        rem->prop.lock_user_stacking = bd->lock_user_stacking;
        rem->prop.lock_client_stacking = bd->lock_client_stacking;
        rem->prop.lock_user_iconify = bd->lock_user_iconify;
        rem->prop.lock_client_iconify = bd->lock_client_iconify;
        rem->prop.lock_user_desk = bd->lock_user_desk;
        rem->prop.lock_client_desk = bd->lock_client_desk;
        rem->prop.lock_user_sticky = bd->lock_user_sticky;
        rem->prop.lock_client_sticky = bd->lock_client_sticky;
        rem->prop.lock_user_shade = bd->lock_user_shade;
        rem->prop.lock_client_shade = bd->lock_client_shade;
        rem->prop.lock_user_maximize = bd->lock_user_maximize;
        rem->prop.lock_client_maximize = bd->lock_client_maximize;
        rem->prop.lock_user_fullscreen = bd->lock_user_fullscreen;
        rem->prop.lock_client_fullscreen = bd->lock_client_fullscreen;
        rem->prop.lock_border = bd->lock_border;
        rem->prop.lock_close = bd->lock_close;
        rem->prop.lock_focus_in = bd->lock_focus_in;
        rem->prop.lock_focus_out = bd->lock_focus_out;
        rem->prop.lock_life = bd->lock_life;
     }
   if (rem->apply & E_REMEMBER_APPLY_SHADE)
     {
        if (bd->shaded)
          rem->prop.shaded = (100 + bd->shade.dir);
        else
          rem->prop.shaded = (50 + bd->shade.dir);
     }
   if (rem->apply & E_REMEMBER_APPLY_ZONE)
     {
        rem->prop.zone = bd->zone->num;
        rem->prop.head = bd->zone->container->manager->num;
     }
   if (rem->apply & E_REMEMBER_APPLY_SKIP_WINLIST)
     rem->prop.skip_winlist = bd->user_skip_winlist;
   if (rem->apply & E_REMEMBER_APPLY_STICKY)
     rem->prop.sticky = bd->sticky;
   if (rem->apply & E_REMEMBER_APPLY_SKIP_PAGER)
     rem->prop.skip_pager = bd->client.netwm.state.skip_pager;
   if (rem->apply & E_REMEMBER_APPLY_SKIP_TASKBAR)
     rem->prop.skip_taskbar = bd->client.netwm.state.skip_taskbar;
   if (rem->apply & E_REMEMBER_APPLY_ICON_PREF)
     rem->prop.icon_preference = bd->icon_preference;
   if (rem->apply & E_REMEMBER_APPLY_DESKTOP)
     e_desk_xy_get(bd->desk, &rem->prop.desk_x, &rem->prop.desk_y);
   if (rem->apply & E_REMEMBER_APPLY_FULLSCREEN)
     rem->prop.fullscreen = bd->fullscreen;
   if (rem->apply & E_REMEMBER_APPLY_OFFER_RESISTANCE)
     rem->prop.offer_resistance = bd->offer_resistance;
   rem->no_reopen = bd->internal_no_reopen;
   {
      E_Event_Remember_Update *ev;

      ev = malloc(sizeof(E_Event_Remember_Update));
      if (!ev) return;
      ev->border = bd;
      ecore_event_add(E_EVENT_REMEMBER_UPDATE, ev, NULL, NULL);
   }
}

/* local subsystem functions */
static E_Remember *
_e_remember_find(E_Border *bd, int check_usable)
{
   Eina_List *l = NULL;
   E_Remember *rem;

#if REMEMBER_SIMPLE
   EINA_LIST_FOREACH(e_config->remembers, l, rem)
     {
        int required_matches;
        int matches;
        const char *title = "";

        matches = 0;
        required_matches = 0;
        if (rem->match & E_REMEMBER_MATCH_NAME) required_matches++;
        if (rem->match & E_REMEMBER_MATCH_CLASS) required_matches++;
        if (rem->match & E_REMEMBER_MATCH_TITLE) required_matches++;
        if (rem->match & E_REMEMBER_MATCH_ROLE) required_matches++;
        if (rem->match & E_REMEMBER_MATCH_TYPE) required_matches++;
        if (rem->match & E_REMEMBER_MATCH_TRANSIENT) required_matches++;

        if (bd->client.netwm.name) title = bd->client.netwm.name;
        else title = bd->client.icccm.title;

        if ((rem->match & E_REMEMBER_MATCH_NAME) &&
            ((!e_util_strcmp(rem->name, bd->client.icccm.name)) ||
             (e_util_both_str_empty(rem->name, bd->client.icccm.name))))
          matches++;
        if ((rem->match & E_REMEMBER_MATCH_CLASS) &&
            ((!e_util_strcmp(rem->class, bd->client.icccm.class)) ||
             (e_util_both_str_empty(rem->class, bd->client.icccm.class))))
          matches++;
        if ((rem->match & E_REMEMBER_MATCH_TITLE) &&
            ((!e_util_strcmp(rem->title, title)) ||
             (e_util_both_str_empty(rem->title, title))))
          matches++;
        if ((rem->match & E_REMEMBER_MATCH_ROLE) &&
            ((!e_util_strcmp(rem->role, bd->client.icccm.window_role)) ||
             (e_util_both_str_empty(rem->role, bd->client.icccm.window_role))))
          matches++;
        if ((rem->match & E_REMEMBER_MATCH_TYPE) &&
            (rem->type == bd->client.netwm.type))
          matches++;
        if ((rem->match & E_REMEMBER_MATCH_TRANSIENT) &&
            (((rem->transient) && (bd->client.icccm.transient_for != 0)) ||
             ((!rem->transient) && (bd->client.icccm.transient_for == 0))))
          matches++;
        if (matches >= required_matches)
          return rem;
     }
   return NULL;
#endif
#if REMEMBER_HIERARCHY
   /* This search method finds the best possible match available and is
    * based on the fact that the list is sorted, with those remembers
    * with the most possible matches at the start of the list. This
    * means, as soon as a valid match is found, it is a match
    * within the set of best possible matches. */
   EINA_LIST_FOREACH(e_config->remembers, l, rem)
     {
        const char *title = "";

        if ((check_usable) && (!e_remember_usable_get(rem)))
          continue;

        if (bd->client.netwm.name) title = bd->client.netwm.name;
        else title = bd->client.icccm.title;

        /* For each type of match, check whether the match is
         * required, and if it is, check whether there's a match. If
         * it fails, then go to the next remember */
        if (rem->match & E_REMEMBER_MATCH_NAME &&
            !e_util_glob_match(bd->client.icccm.name, rem->name))
          continue;
        if (rem->match & E_REMEMBER_MATCH_CLASS &&
            !e_util_glob_match(bd->client.icccm.class, rem->class))
          continue;
        if (rem->match & E_REMEMBER_MATCH_TITLE &&
            !e_util_glob_match(title, rem->title))
          continue;
        if (rem->match & E_REMEMBER_MATCH_ROLE &&
            e_util_strcmp(rem->role, bd->client.icccm.window_role) &&
            !e_util_both_str_empty(rem->role, bd->client.icccm.window_role))
          continue;
        if (rem->match & E_REMEMBER_MATCH_TYPE &&
            rem->type != (int)bd->client.netwm.type)
          continue;
        if (rem->match & E_REMEMBER_MATCH_TRANSIENT &&
            !(rem->transient && bd->client.icccm.transient_for != 0) &&
            !(!rem->transient) && (bd->client.icccm.transient_for == 0))
          continue;

        return rem;
     }

   return NULL;
#endif
}

static void
_e_remember_free(E_Remember *rem)
{
   e_config->remembers = eina_list_remove(e_config->remembers, rem);
   if (rem->name) eina_stringshare_del(rem->name);
   if (rem->class) eina_stringshare_del(rem->class);
   if (rem->title) eina_stringshare_del(rem->title);
   if (rem->role) eina_stringshare_del(rem->role);
   if (rem->prop.border) eina_stringshare_del(rem->prop.border);
   if (rem->prop.command) eina_stringshare_del(rem->prop.command);
   if (rem->prop.desktop_file) eina_stringshare_del(rem->prop.desktop_file);
   free(rem);
}

static void
_e_remember_cb_hook_eval_post_new_border(void *data __UNUSED__, void *border)
{
   E_Border *bd;

   bd = border;

   // remember only when window was modified
   // if (!bd->new_client) return;

   if ((bd->internal) && (!bd->remember) &&
       (e_config->remember_internal_windows) &&
       (!bd->internal_no_remember) &&
       (bd->client.icccm.class && bd->client.icccm.class[0]))
     {
        E_Remember *rem;

        if (!strncmp(bd->client.icccm.class, "e_fwin", 6))
          {
             if (!(e_config->remember_internal_windows & E_REMEMBER_INTERNAL_FM_WINS))
               return;
          }
        else
          {
             if (!(e_config->remember_internal_windows & E_REMEMBER_INTERNAL_DIALOGS))
               return;
          }

        rem = e_remember_new();
        if (!rem) return;

        e_remember_default_match_set(rem, bd);

        rem->apply = E_REMEMBER_APPLY_POS | E_REMEMBER_APPLY_SIZE | E_REMEMBER_APPLY_BORDER;

        e_remember_use(rem);
        e_remember_update(bd);
        bd->remember = rem;
     }
}

static void
_e_remember_cb_hook_pre_post_fetch(void *data __UNUSED__, void *border)
{
   E_Border *bd = border;
   E_Remember *rem = NULL;
   int temporary = 0;

   if ((!bd->new_client) || (bd->internal_no_remember)) return;

   if (!bd->remember)
     {
        rem = e_remember_find_usable(bd);
        if (rem)
          {
             bd->remember = rem;
             e_remember_use(rem);
          }
     }

   if (bd->internal && remembers && bd->client.icccm.class && bd->client.icccm.class[0])
     {
        Eina_List *l;
        EINA_LIST_FOREACH(remembers->list, l, rem)
          {
             if (rem->class && !strcmp(rem->class, bd->client.icccm.class))
               break;
          }
        if (rem)
          {
             temporary = 1;
             remembers->list = eina_list_remove(remembers->list, rem);
             if (!remembers->list)
               e_config_domain_save("e_remember_restart",
                                    e_remember_list_edd, remembers);
          }
        else rem = bd->remember;
     }

   if (!rem)
     return;

   if (rem->apply & E_REMEMBER_APPLY_ZONE)
     {
        E_Zone *zone;

        zone = e_container_zone_number_get(bd->zone->container, rem->prop.zone);
        if (zone)
          e_border_zone_set(bd, zone);
     }
   if (rem->apply & E_REMEMBER_APPLY_DESKTOP)
     {
        E_Desk *desk;

        desk = e_desk_at_xy_get(bd->zone, rem->prop.desk_x, rem->prop.desk_y);
        if (desk)
          {
             e_border_desk_set(bd, desk);
             if (e_config->desk_auto_switch)
               e_desk_show(desk);
          }
     }
   if (rem->apply & E_REMEMBER_APPLY_SIZE)
     {
        bd->client.w = rem->prop.w;
        bd->client.h = rem->prop.h;
        /* we can trust internal windows */
        if (bd->internal)
          {
             if (bd->zone->w != rem->prop.res_x)
               {
                  if (bd->client.w > (bd->zone->w - 64))
                    bd->client.w = bd->zone->w - 64;
               }
             if (bd->zone->h != rem->prop.res_y)
               {
                  if (bd->client.h > (bd->zone->h - 64))
                    bd->client.h = bd->zone->h - 64;
               }
             if (bd->client.icccm.min_w > bd->client.w)
               bd->client.w = bd->client.icccm.min_w;
             if (bd->client.icccm.max_w < bd->client.w)
               bd->client.w = bd->client.icccm.max_w;
             if (bd->client.icccm.min_h > bd->client.h)
               bd->client.h = bd->client.icccm.min_h;
             if (bd->client.icccm.max_h < bd->client.h)
               bd->client.h = bd->client.icccm.max_h;
          }
        bd->w = bd->client.w + bd->client_inset.l + bd->client_inset.r;
        bd->h = bd->client.h + bd->client_inset.t + bd->client_inset.b;
        if (rem->prop.maximize)
          {
             bd->saved.x = rem->prop.pos_x;
             bd->saved.y = rem->prop.pos_y;
             bd->saved.w = bd->w;
             bd->saved.h = bd->h;
             bd->maximized = rem->prop.maximize | e_config->maximize_policy;
          }
        bd->changes.size = 1;
        bd->changes.shape = 1;
        BD_CHANGED(bd);
     }
   if ((rem->apply & E_REMEMBER_APPLY_POS) && (!bd->re_manage))
     {
        bd->x = rem->prop.pos_x;
        bd->y = rem->prop.pos_y;
        if (bd->zone->w != rem->prop.res_x)
          {
             int px;

             px = bd->x + (bd->w / 2);
             if (px < ((rem->prop.res_x * 1) / 3))
               {
                  if (bd->zone->w >= (rem->prop.res_x / 3))
                    bd->x = rem->prop.pos_x;
                  else
                    bd->x = ((rem->prop.pos_x - 0) * bd->zone->w) /
                      (rem->prop.res_x / 3);
               }
             else if (px < ((rem->prop.res_x * 2) / 3))
               {
                  if (bd->zone->w >= (rem->prop.res_x / 3))
                    bd->x = (bd->zone->w / 2) +
                      (px - (rem->prop.res_x / 2)) -
                      (bd->w / 2);
                  else
                    bd->x = (bd->zone->w / 2) +
                      (((px - (rem->prop.res_x / 2)) * bd->zone->w) /
                       (rem->prop.res_x / 3)) -
                      (bd->w / 2);
               }
             else
               {
                  if (bd->zone->w >= (rem->prop.res_x / 3))
                    bd->x = bd->zone->w +
                      rem->prop.pos_x - rem->prop.res_x +
                      (rem->prop.w - bd->client.w);
                  else
                    bd->x = bd->zone->w +
                      (((rem->prop.pos_x - rem->prop.res_x) * bd->zone->w) /
                       (rem->prop.res_x / 3)) +
                      (rem->prop.w - bd->client.w);
               }
             if ((rem->prop.pos_x >= 0) && (bd->x < 0))
               bd->x = 0;
             else if (((rem->prop.pos_x + rem->prop.w) < rem->prop.res_x) &&
                      ((bd->x + bd->w) > bd->zone->w))
               bd->x = bd->zone->w - bd->w;
          }
        if (bd->zone->h != rem->prop.res_y)
          {
             int py;

             py = bd->y + (bd->h / 2);
             if (py < ((rem->prop.res_y * 1) / 3))
               {
                  if (bd->zone->h >= (rem->prop.res_y / 3))
                    bd->y = rem->prop.pos_y;
                  else
                    bd->y = ((rem->prop.pos_y - 0) * bd->zone->h) /
                      (rem->prop.res_y / 3);
               }
             else if (py < ((rem->prop.res_y * 2) / 3))
               {
                  if (bd->zone->h >= (rem->prop.res_y / 3))
                    bd->y = (bd->zone->h / 2) +
                      (py - (rem->prop.res_y / 2)) -
                      (bd->h / 2);
                  else
                    bd->y = (bd->zone->h / 2) +
                      (((py - (rem->prop.res_y / 2)) * bd->zone->h) /
                       (rem->prop.res_y / 3)) -
                      (bd->h / 2);
               }
             else
               {
                  if (bd->zone->h >= (rem->prop.res_y / 3))
                    bd->y = bd->zone->h +
                      rem->prop.pos_y - rem->prop.res_y +
                      (rem->prop.h - bd->client.h);
                  else
                    bd->y = bd->zone->h +
                      (((rem->prop.pos_y - rem->prop.res_y) * bd->zone->h) /
                       (rem->prop.res_y / 3)) +
                      (rem->prop.h - bd->client.h);
               }
             if ((rem->prop.pos_y >= 0) && (bd->y < 0))
               bd->y = 0;
             else if (((rem->prop.pos_y + rem->prop.h) < rem->prop.res_y) &&
                      ((bd->y + bd->h) > bd->zone->h))
               bd->y = bd->zone->h - bd->h;
          }
        //		  if (bd->zone->w != rem->prop.res_x)
        //		    bd->x = (rem->prop.pos_x * bd->zone->w) / rem->prop.res_x;
        //		  if (bd->zone->h != rem->prop.res_y)
        //		    bd->y = (rem->prop.pos_y * bd->zone->h) / rem->prop.res_y;
        if (
            /* upper left */
            (!E_INSIDE(bd->x, bd->y, 0, 0, bd->zone->w, bd->zone->h)) &&
            /* upper right */
            (!E_INSIDE(bd->x + bd->w, bd->y, 0, 0, bd->zone->w, bd->zone->h)) &&
            /* lower left */
            (!E_INSIDE(bd->x, bd->y + bd->h, 0, 0, bd->zone->w, bd->zone->h)) &&
            /* lower right */
            (!E_INSIDE(bd->x + bd->w, bd->y + bd->h, 0, 0, bd->zone->w, bd->zone->h))
           )
          {
             e_border_center_pos_get(bd, &bd->x, &bd->y);
             rem->prop.pos_x = bd->x, rem->prop.pos_y = bd->y;
          }
        bd->x += bd->zone->x;
        bd->y += bd->zone->y;
        bd->placed = 1;
        bd->changes.pos = 1;
        BD_CHANGED(bd);
     }
   if (rem->apply & E_REMEMBER_APPLY_LAYER)
     {
        e_border_layer_set(bd, rem->prop.layer);
     }
   if (rem->apply & E_REMEMBER_APPLY_BORDER)
     {
        if (rem->prop.border)
          {
             if (bd->bordername) eina_stringshare_del(bd->bordername);
             if (rem->prop.border) bd->bordername = eina_stringshare_add(rem->prop.border);
             else bd->bordername = NULL;
             bd->client.border.changed = 1;
          }
     }
   if (rem->apply & E_REMEMBER_APPLY_FULLSCREEN)
     {
        if (rem->prop.fullscreen)
          e_border_fullscreen(bd, e_config->fullscreen_policy);
     }
   if (rem->apply & E_REMEMBER_APPLY_STICKY)
     {
        if (rem->prop.sticky) e_border_stick(bd);
     }
   if (rem->apply & E_REMEMBER_APPLY_SHADE)
     {
        if (rem->prop.shaded >= 100)
          e_border_shade(bd, rem->prop.shaded - 100);
        else if (rem->prop.shaded >= 50)
          e_border_unshade(bd, rem->prop.shaded - 50);
     }
   if (rem->apply & E_REMEMBER_APPLY_LOCKS)
     {
        bd->lock_user_location = rem->prop.lock_user_location;
        bd->lock_client_location = rem->prop.lock_client_location;
        bd->lock_user_size = rem->prop.lock_user_size;
        bd->lock_client_size = rem->prop.lock_client_size;
        bd->lock_user_stacking = rem->prop.lock_user_stacking;
        bd->lock_client_stacking = rem->prop.lock_client_stacking;
        bd->lock_user_iconify = rem->prop.lock_user_iconify;
        bd->lock_client_iconify = rem->prop.lock_client_iconify;
        bd->lock_user_desk = rem->prop.lock_user_desk;
        bd->lock_client_desk = rem->prop.lock_client_desk;
        bd->lock_user_sticky = rem->prop.lock_user_sticky;
        bd->lock_client_sticky = rem->prop.lock_client_sticky;
        bd->lock_user_shade = rem->prop.lock_user_shade;
        bd->lock_client_shade = rem->prop.lock_client_shade;
        bd->lock_user_maximize = rem->prop.lock_user_maximize;
        bd->lock_client_maximize = rem->prop.lock_client_maximize;
        bd->lock_user_fullscreen = rem->prop.lock_user_fullscreen;
        bd->lock_client_fullscreen = rem->prop.lock_client_fullscreen;
        bd->lock_border = rem->prop.lock_border;
        bd->lock_close = rem->prop.lock_close;
        bd->lock_focus_in = rem->prop.lock_focus_in;
        bd->lock_focus_out = rem->prop.lock_focus_out;
        bd->lock_life = rem->prop.lock_life;
     }
   if (rem->apply & E_REMEMBER_APPLY_SKIP_WINLIST)
     bd->user_skip_winlist = rem->prop.skip_winlist;
   if (rem->apply & E_REMEMBER_APPLY_SKIP_PAGER)
     bd->client.netwm.state.skip_pager = rem->prop.skip_pager;
   if (rem->apply & E_REMEMBER_APPLY_SKIP_TASKBAR)
     bd->client.netwm.state.skip_taskbar = rem->prop.skip_taskbar;
   if (rem->apply & E_REMEMBER_APPLY_ICON_PREF)
     bd->icon_preference = rem->prop.icon_preference;
   if (rem->apply & E_REMEMBER_APPLY_OFFER_RESISTANCE)
     bd->offer_resistance = rem->prop.offer_resistance;
   if (rem->apply & E_REMEMBER_SET_FOCUS_ON_START)
     bd->want_focus = 1;

   if (temporary)
     _e_remember_free(rem);
}

static void
_e_remember_init_edd(void)
{
   e_remember_edd = E_CONFIG_DD_NEW("E_Remember", E_Remember);
#undef T
#undef D
#define T E_Remember
#define D e_remember_edd
   E_CONFIG_VAL(D, T, match, INT);
   E_CONFIG_VAL(D, T, no_reopen, INT);
   E_CONFIG_VAL(D, T, apply_first_only, UCHAR);
   E_CONFIG_VAL(D, T, keep_settings, UCHAR);
   E_CONFIG_VAL(D, T, name, STR);
   E_CONFIG_VAL(D, T, class, STR);
   E_CONFIG_VAL(D, T, title, STR);
   E_CONFIG_VAL(D, T, role, STR);
   E_CONFIG_VAL(D, T, type, INT);
   E_CONFIG_VAL(D, T, transient, UCHAR);
   E_CONFIG_VAL(D, T, apply, INT);
   E_CONFIG_VAL(D, T, max_score, INT);
   E_CONFIG_VAL(D, T, prop.pos_x, INT);
   E_CONFIG_VAL(D, T, prop.pos_y, INT);
   E_CONFIG_VAL(D, T, prop.res_x, INT);
   E_CONFIG_VAL(D, T, prop.res_y, INT);
   E_CONFIG_VAL(D, T, prop.pos_w, INT);
   E_CONFIG_VAL(D, T, prop.pos_h, INT);
   E_CONFIG_VAL(D, T, prop.w, INT);
   E_CONFIG_VAL(D, T, prop.h, INT);
   E_CONFIG_VAL(D, T, prop.layer, INT);
   E_CONFIG_VAL(D, T, prop.maximize, UINT);
   E_CONFIG_VAL(D, T, prop.lock_user_location, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_location, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_size, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_size, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_stacking, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_stacking, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_iconify, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_iconify, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_desk, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_desk, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_sticky, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_sticky, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_shade, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_shade, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_maximize, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_maximize, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_user_fullscreen, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_client_fullscreen, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_border, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_close, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_focus_in, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_focus_out, UCHAR);
   E_CONFIG_VAL(D, T, prop.lock_life, UCHAR);
   E_CONFIG_VAL(D, T, prop.border, STR);
   E_CONFIG_VAL(D, T, prop.sticky, UCHAR);
   E_CONFIG_VAL(D, T, prop.shaded, UCHAR);
   E_CONFIG_VAL(D, T, prop.skip_winlist, UCHAR);
   E_CONFIG_VAL(D, T, prop.skip_pager, UCHAR);
   E_CONFIG_VAL(D, T, prop.skip_taskbar, UCHAR);
   E_CONFIG_VAL(D, T, prop.fullscreen, UCHAR);
   E_CONFIG_VAL(D, T, prop.desk_x, INT);
   E_CONFIG_VAL(D, T, prop.desk_y, INT);
   E_CONFIG_VAL(D, T, prop.zone, INT);
   E_CONFIG_VAL(D, T, prop.head, INT);
   E_CONFIG_VAL(D, T, prop.command, STR);
   E_CONFIG_VAL(D, T, prop.icon_preference, UCHAR);
   E_CONFIG_VAL(D, T, prop.desktop_file, STR);
   E_CONFIG_VAL(D, T, prop.offer_resistance, UCHAR);
#undef T
#undef D
   e_remember_list_edd = E_CONFIG_DD_NEW("E_Remember_List", E_Remember_List);
#undef T
#undef D
#define T E_Remember_List
#define D e_remember_list_edd
   E_CONFIG_LIST(D, T, list, e_remember_edd);
#undef T
#undef D
}

